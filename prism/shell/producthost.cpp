// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "producthost.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QLoggingCategory>
#include <QtCore/QProcess>
#include <QtCore/QRandomGenerator>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QtEndian>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include <algorithm>
#include <array>
#include <utility>

#if defined(Q_OS_LINUX)
#include <csignal>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#endif

using namespace Qt::StringLiterals;

namespace qor {

Q_LOGGING_CATEGORY(lcProducts, "qor.prism.products")

namespace {

// Random bytes as lowercase hex, from the operating system's generator.
QByteArray randomHex(int bytes)
{
    std::array<quint32, 8> words {};
    const int count = std::min<int>(int(words.size()), bytes / 4);
    QRandomGenerator::system()->fillRange(words.data(), count);
    QByteArray raw(reinterpret_cast<const char *>(words.data()), count * 4);
    words.fill(0);
    QByteArray hex = raw.toHex();
    raw.fill('\0');
    return hex;
}

// Compares two secrets in time that depends on their length only, which is
// not secret: how much of a guess was right cannot be measured.
bool sameSecret(const QByteArray &offered, const QByteArray &expected)
{
    if (expected.isEmpty() || offered.size() != expected.size())
        return false;
    unsigned char difference = 0;
    for (qsizetype i = 0; i < expected.size(); ++i)
        difference |= static_cast<unsigned char>(offered.at(i) ^ expected.at(i));
    return difference == 0;
}

// The process on the other end of a connection, as the operating system
// reports it, and only if it runs as this user. On Windows the pipe itself
// admits only this user (QLocalServer::UserAccessOption).
std::optional<qint64> peerProcess(QLocalSocket *socket)
{
#if defined(Q_OS_LINUX)
    ucred credentials {};
    socklen_t length = sizeof credentials;
    if (::getsockopt(int(socket->socketDescriptor()), SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0
        || credentials.uid != ::getuid())
        return std::nullopt;
    return qint64(credentials.pid);
#elif defined(Q_OS_WIN)
    ULONG pid = 0;
    if (!::GetNamedPipeClientProcessId(reinterpret_cast<HANDLE>(socket->socketDescriptor()), &pid))
        return std::nullopt;
    return qint64(pid);
#else
    Q_UNUSED(socket);
    return std::nullopt;
#endif
}

// Commands a newer one of the same type makes moot while not yet sent.
bool replaceable(const QString &type)
{
    return type == u"place"_s || type == u"theme"_s || type == u"player"_s || type == u"prefs"_s || type == u"play"_s;
}

QJsonObject prefsCommand(bool reducedMotion)
{
    return { { u"type"_s, u"prefs"_s }, { u"prefs"_s, QJsonObject { { u"reduced_motion"_s, reducedMotion } } } };
}

QJsonObject themeCommand(const QVariantMap &colors)
{
    return { { u"type"_s, u"theme"_s }, { u"colors"_s, QJsonObject::fromVariantMap(colors) } };
}

// Ends a product that has not ended by itself within `graceMs`: asked first,
// then made to.
void endLater(QProcess *process, int graceMs = 5000)
{
    if (!process)
        return;
    QTimer::singleShot(graceMs, process, [process] {
        if (process->state() == QProcess::NotRunning)
            return;
        process->terminate();
        QTimer::singleShot(3000, process, [process] {
            if (process->state() != QProcess::NotRunning)
                process->kill();
        });
    });
}

} // namespace

// ── HostSession ─────────────────────────────────────────────────────────────

HostSession::HostSession(const ProductRecord &record, QObject *parent)
    : QObject(parent), m_record(record), m_token(randomHex(32))
{
    m_deadline.setSingleShot(true);
    connect(&m_deadline, &QTimer::timeout, this, [this] {
        if (!m_welcomed)
            end(u"the product did not connect, or say hello, in time"_s);
    });
    m_wait.setSingleShot(true);
    connect(&m_wait, &QTimer::timeout, this, [this] { answer(QJsonValue::Null); });
}

HostSession::~HostSession() { forgetToken(); }

bool HostSession::listen()
{
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    m_server->setMaxPendingConnections(1);
#if defined(Q_OS_UNIX)
    // A folder only this user can enter (QTemporaryDir makes it 0700), in the
    // session's runtime folder.
    QString base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (base.isEmpty())
        base = QDir::tempPath();
    m_folder = std::make_unique<QTemporaryDir>(base + u"/qor-host-XXXXXX"_s);
    if (!m_folder->isValid())
        return false;
    m_channel = m_folder->filePath(u"channel"_s);
#else
    m_channel = u"qor-host-"_s + QString::fromLatin1(randomHex(16));
#endif
    connect(m_server, &QLocalServer::newConnection, this, &HostSession::accept);
    return m_server->listen(m_channel);
}

void HostSession::expect(qint64 pid, int lifetimeMs)
{
    m_expected = pid;
    m_lifetimeMs = lifetimeMs;
    m_issued.start();
    m_deadline.start(lifetimeMs);
}

void HostSession::accept()
{
    QLocalSocket *socket = m_server->nextPendingConnection();
    // One connection, whoever it is from: the channel then closes to everyone.
    m_server->close();
    m_folder.reset();
    if (!socket)
        return;
    socket->setParent(this);
    const std::optional<qint64> peer = peerProcess(socket);
    if (m_ended || !m_expected || !peer || *peer != *m_expected) {
        socket->abort();
        socket->deleteLater();
        end(u"a connection that was not from the product Prism started"_s);
        return;
    }
    m_socket = socket;
    connect(socket, &QLocalSocket::readyRead, this, &HostSession::readFrames);
    connect(socket, &QLocalSocket::disconnected, this, [this] {
        end(m_welcomed ? u"the product closed its channel"_s : u"the product closed the channel before saying hello"_s);
    });
    readFrames();
}

void HostSession::readFrames()
{
    if (!m_socket || m_ended)
        return;
    m_buffer += m_socket->readAll();
    if (m_buffer.size() > 2 * (FrameLimit + 4)) {
        end(u"the product sent more than it was asked"_s);
        return;
    }
    // One request at a time: while a long poll is held, what follows it waits.
    while (!m_held && !m_ended && m_buffer.size() >= 4) {
        const quint32 length = qFromBigEndian<quint32>(m_buffer.constData());
        if (length > FrameLimit) {
            // A peer breaking the protocol: nothing more is read from it.
            end(u"a frame over 1 MiB"_s);
            return;
        }
        if (m_buffer.size() < 4 + qsizetype(length))
            return;
        const QByteArray body = m_buffer.mid(4, length);
        m_buffer.remove(0, 4 + qsizetype(length));
        handle(body);
    }
}

void HostSession::handle(const QByteArray &body)
{
    QJsonParseError error {};
    const QJsonDocument document = QJsonDocument::fromJson(body, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        refuse(u"bad_request"_s, u"a frame that is not a JSON object"_s, u"an unreadable frame"_s);
        return;
    }
    if (m_welcomed)
        request(document.object());
    else
        hello(document.object());
}

void HostSession::hello(const QJsonObject &frame)
{
    if (frame.value(u"type"_s).toString() != u"hello"_s) {
        refuse(u"bad_request"_s, u"the first frame must be a hello"_s, u"the first frame was not a hello"_s);
        return;
    }
    // Compared in constant time, and once, whatever the outcome.
    const bool tokenMatches = sameSecret(frame.value(u"token"_s).toString().toLatin1(), m_token);
    const bool inTime = m_issued.isValid() && m_issued.elapsed() <= m_lifetimeMs;
    forgetToken();
    const QString notStarted = u"this connection was not the one Prism started"_s;
    if (!tokenMatches || !inTime) {
        refuse(u"not_authorised"_s, notStarted, u"the token was wrong, or used too late"_s);
        return;
    }
    if (frame.value(u"product"_s).toString() != m_record.id) {
        refuse(u"not_authorised"_s, notStarted, u"a hello from another product than the one started"_s);
        return;
    }
    // The newest version both speak, if there is one.
    const qint64 chosen = std::min<qint64>(frame.value(u"protocol"_s).toInteger(), Protocol);
    if (chosen < std::max<qint64>(frame.value(u"min_protocol"_s).toInteger(), Protocol)) {
        refuse(u"protocol_mismatch"_s, u"Prism speaks the QOR host protocol %1"_s.arg(Protocol),
               u"the product asked for a protocol Prism does not speak"_s);
        return;
    }
    m_deadline.stop();
    m_welcomed = true;
    QByteArray session = randomHex(16);
    write({ { u"type"_s, u"welcome"_s },
            { u"protocol"_s, chosen },
            { u"capabilities"_s, QJsonArray { m_record.method } },
            { u"session"_s, QString::fromLatin1(session) } });
    session.fill('\0');
    qCInfo(lcProducts, "%s connected", qPrintable(m_record.name));
    emit welcomedChanged();
}

void HostSession::request(const QJsonObject &frame)
{
    const QString type = frame.value(u"type"_s).toString();
    if (type == u"hello"_s) {
        refuse(u"bad_request"_s, u"a session has one hello"_s, u"a second hello"_s);
        return;
    }
    const QJsonValue id = frame.value(u"id"_s);
    if (type != u"request"_s || !id.isDouble() || !frame.value(u"method"_s).isString()) {
        refuse(u"bad_request"_s, u"a frame that is not a request"_s, u"a frame that was not a request"_s);
        return;
    }
    const QString method = frame.value(u"method"_s).toString();
    if (method != m_record.method) {
        write({ { u"type"_s, u"error"_s },
                { u"id"_s, id },
                { u"code"_s, u"unsupported_method"_s },
                { u"message"_s, u"Prism does not offer \"%1\""_s.arg(method.left(64)) } });
        return;
    }
    // What QOR Media reports on its long poll (where playback stopped, a
    // file's length) is for the Library: each object with a type, 64 at most,
    // as the product keeps them.
    const QJsonArray reports = frame.value(u"params"_s).toObject().value(u"reports"_s).toArray();
    for (qsizetype i = 0; i < std::min<qsizetype>(reports.size(), 64); ++i) {
        const QJsonObject report = reports.at(i).toObject();
        if (report.value(u"type"_s).isString())
            emit reported(report);
    }
    m_held = id.toInteger();
    if (!m_commands.isEmpty())
        answer(m_commands.takeFirst());
    else
        m_wait.start(m_record.waitMs);
}

void HostSession::answer(const QJsonValue &command)
{
    if (!m_held)
        return;
    const qint64 id = *std::exchange(m_held, std::nullopt);
    m_wait.stop();
    write({ { u"type"_s, u"result"_s }, { u"id"_s, id }, { u"result"_s, QJsonObject { { u"command"_s, command } } } });
    // Anything the product sent while its poll was held.
    QMetaObject::invokeMethod(this, &HostSession::readFrames, Qt::QueuedConnection);
}

void HostSession::send(const QJsonObject &command)
{
    if (m_ended)
        return;
    const QString type = command.value(u"type"_s).toString();
    if (replaceable(type))
        m_commands.removeIf([&type](const QJsonObject &older) { return older.value(u"type"_s).toString() == type; });
    m_commands.append(command);
    if (m_held)
        answer(m_commands.takeFirst());
}

QJsonArray HostSession::waiting() const
{
    QJsonArray commands;
    for (const QJsonObject &command : m_commands)
        commands.append(command);
    return commands;
}

// At once, whatever is still to be written: the product needs only to see its
// channel close. (Closing gracefully waits for the event loop to write it,
// which never comes while Prism is ending.)
void HostSession::close() { end(u"Prism closed the channel"_s, false); }

void HostSession::write(const QJsonObject &frame)
{
    if (!m_socket || m_socket->state() != QLocalSocket::ConnectedState)
        return;
    const QByteArray body = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    if (body.size() > FrameLimit)
        return;
    QByteArray length(4, '\0');
    qToBigEndian(quint32(body.size()), length.data());
    m_socket->write(length + body);
    m_socket->flush();
}

void HostSession::refuse(const QString &code, const QString &message, const QString &reason)
{
    // The product is told why; the session ends whether or not it listens.
    write({ { u"type"_s, u"error"_s }, { u"id"_s, QJsonValue::Null }, { u"code"_s, code }, { u"message"_s, message } });
    end(reason);
}

void HostSession::end(const QString &reason, bool gracefully)
{
    if (m_ended)
        return;
    m_ended = true;
    forgetToken();
    m_deadline.stop();
    m_wait.stop();
    m_held.reset();
    m_commands.clear();
    if (m_server)
        m_server->close();
    m_folder.reset();
    // Gracefully, what is still to be written (a refusal) goes before the
    // channel closes.
    if (m_socket) {
        if (gracefully)
            m_socket->disconnectFromServer();
        else
            m_socket->abort();
    }
    qCInfo(lcProducts, "%s: %s", qPrintable(m_record.name), qPrintable(reason));
    emit ended(reason);
}

void HostSession::forgetToken()
{
    m_token.fill('\0');
    m_token.clear();
}

// ── ProductHost ─────────────────────────────────────────────────────────────

ProductHost::ProductHost(QObject *parent) : ProductHost(defaultRecords(), parent) { }

ProductHost::ProductHost(const QList<ProductRecord> &records, QObject *parent) : QObject(parent), m_records(records) { }

ProductHost::~ProductHost()
{
    // Every product ends with Prism: its channel closes, and it quits. One
    // with no channel is asked to.
    const QHash<QString, Run> runs = std::exchange(m_runs, {});
    for (const Run &run : runs) {
        if (run.process)
            run.process->disconnect(this);
        if (run.session) {
            run.session->disconnect(this);
            run.session->close();
        } else if (run.process) {
            run.process->terminate();
        }
    }
    for (const Run &run : runs) {
        if (run.process && !run.process->waitForFinished(2000)) {
            run.process->kill();
            run.process->waitForFinished(1000);
        }
    }
}

QList<ProductRecord> ProductHost::defaultRecords()
{
    ProductRecord arqade;
    arqade.id = u"arqade"_s;
    arqade.name = u"ARQADE"_s;
    arqade.variable = u"QOR_ARQADE"_s;
    arqade.method = u"arqade.next"_s;
    arqade.waitMs = 3000;
    arqade.commands = { u"place"_s, u"hide"_s, u"theme"_s, u"player"_s, u"prefs"_s, u"play"_s, u"quit"_s };
    arqade.glyph = u"gamepad-2"_s;
    arqade.size = QSize(1360, 860);
    arqade.launchable = true;
    // Its games read the controller themselves, and its home answers it as well: the shell's would press their keys
    // a second time, and its East would be Esc, which puts a game away.
    arqade.takesController = true;
    arqade.keywords = u"arcade games play orbital void runner synapse rift survivor flux reversi arenas"_s;
    arqade.description = u"Demiurge's arcade: its games, rebuilt in QQ."_s;
    arqade.hue = 300;
    arqade.chroma = 0.17;

    ProductRecord media;
    media.id = u"qor-media"_s;
    media.name = u"QOR Media"_s;
    media.variable = u"QOR_MEDIA"_s;
    media.method = u"media.next"_s;
    media.waitMs = 25000;
    media.commands = { u"open"_s, u"place"_s, u"hide"_s, u"theme"_s };
    media.glyph = u"film"_s;
    media.size = QSize(1280, 760);
    media.launchable = false;
    media.keywords = u"player viewer editor video audio music pictures photos pdf documents"_s;
    media.description = u"Plays, shows and edits what the Library holds."_s;
    media.hue = 205;
    media.chroma = 0.12;

    ProductRecord studio;
    studio.id = u"qq-studio"_s;
    studio.name = u"QQ Studio"_s;
    studio.variable = u"QOR_QQ_STUDIO"_s;
    studio.fileArgument = u"--project"_s;
    studio.glyph = u"layers"_s;
    studio.launchable = true;
    studio.keywords = u"qq engine editor scenes worlds games make create build 3d agent"_s;
    studio.description = u"QQ's editor: make scenes and games, and play them."_s;
    studio.hue = 45;
    studio.chroma = 0.16;

    ProductRecord player;
    player.id = u"qq-player"_s;
    player.name = u"QQ Player"_s;
    player.variable = u"QOR_QQ_PLAYER"_s;
    player.fileArgument = u"--"_s;
    player.glyph = u"play"_s;
    player.launchable = false;
    player.takesController = true;
    player.keywords = u"qq games play"_s;
    player.description = u"Plays a QQ game."_s;
    player.hue = 140;
    player.chroma = 0.15;

    return { arqade, media, studio, player };
}

void ProductHost::setDisplay(const QString &display)
{
    if (m_display == display)
        return;
    m_display = display;
    emit displayChanged();
    emit productsChanged();
}

void ProductHost::setReducedMotion(bool reduced)
{
    if (m_reducedMotion == reduced)
        return;
    m_reducedMotion = reduced;
    for (auto it = m_runs.cbegin(); it != m_runs.cend(); ++it) {
        const ProductRecord *r = record(it.key());
        if (it->session && r && r->commands.contains(u"prefs"_s))
            it->session->send(prefsCommand(reduced));
    }
    emit reducedMotionChanged();
}

void ProductHost::setTheme(const QVariantMap &theme)
{
    if (m_theme == theme)
        return;
    m_theme = theme;
    for (auto it = m_runs.cbegin(); it != m_runs.cend(); ++it) {
        const ProductRecord *r = record(it.key());
        if (it->session && r && r->commands.contains(u"theme"_s))
            it->session->send(themeCommand(m_theme));
    }
    emit themeChanged();
}

const ProductRecord *ProductHost::record(const QString &id) const
{
    const auto found = std::find_if(m_records.cbegin(), m_records.cend(), [&id](const auto &r) { return r.id == id; });
    return found == m_records.cend() ? nullptr : &*found;
}

QVariantMap ProductHost::product(const QString &id) const
{
    const ProductRecord *r = record(id);
    if (!r)
        return {};
    return { { u"id"_s, r->id },
             { u"name"_s, r->name },
             { u"glyph"_s, r->glyph },
             { u"width"_s, r->size.width() },
             { u"height"_s, r->size.height() },
             { u"launchable"_s, r->launchable },
             { u"placed"_s, r->commands.contains(u"place"_s) },
             { u"takesController"_s, r->takesController },
             { u"keywords"_s, r->keywords },
             { u"description"_s, r->description },
             { u"hue"_s, r->hue },
             { u"chroma"_s, r->chroma },
             { u"available"_s, !m_display.isEmpty() && !programFor(id).isEmpty() },
             { u"running"_s, m_runs.contains(id) } };
}

QVariantList ProductHost::products() const
{
    QVariantList list;
    for (const ProductRecord &r : m_records)
        list.append(product(r.id));
    return list;
}

QString ProductHost::programFor(const QString &id) const
{
    const ProductRecord *r = record(id);
    if (!r)
        return {};
    const auto runnable = [](const QString &path) {
        const QFileInfo info(path);
        return info.isFile() && info.isExecutable();
    };
    if (const QString named = qEnvironmentVariable(r->variable.toLatin1().constData()); !named.isEmpty())
        return runnable(named) ? QFileInfo(named).absoluteFilePath() : QString();
#if defined(Q_OS_WIN)
    const QString beside = QCoreApplication::applicationDirPath() + u'/' + r->id + u".exe"_s;
#else
    const QString beside = QCoreApplication::applicationDirPath() + u'/' + r->id;
#endif
    return runnable(beside) ? beside : QString();
}

QString ProductHost::productOf(qint64 pid) const
{
    for (auto it = m_runs.cbegin(); it != m_runs.cend(); ++it)
        if (it->process && pid > 0 && it->process->processId() == pid)
            return it.key();
    return {};
}

HostSession *ProductHost::session(const QString &id) const
{
    const auto it = m_runs.constFind(id);
    return it == m_runs.cend() ? nullptr : it->session.data();
}

QJsonObject ProductHost::rectJson(const QRect &rect)
{
    return { { u"x"_s, rect.x() }, { u"y"_s, rect.y() }, { u"w"_s, rect.width() }, { u"h"_s, rect.height() } };
}

bool ProductHost::place(const QString &id, const QRect &rect, const QString &activationToken)
{
    const ProductRecord *r = record(id);
    if (!r || !r->commands.contains(u"place"_s))
        return false;
    if (rect.isEmpty()) {
        qCWarning(lcProducts, "%s cannot be shown in an empty rectangle", qPrintable(r->name));
        return false;
    }
    if (!start(*r, {}, activationToken))
        return false;
    session(id)->send({ { u"type"_s, u"place"_s }, { u"rect"_s, rectJson(rect) }, { u"parent"_s, QJsonValue::Null } });
    return true;
}

bool ProductHost::open(const QString &id, const QString &path, const QString &kind, const QRect &rect,
                       const QString &activationToken, double position)
{
    const ProductRecord *r = record(id);
    if (!r || !r->commands.contains(u"open"_s) || path.isEmpty())
        return false;
    if (rect.isEmpty()) {
        qCWarning(lcProducts, "%s cannot be shown in an empty rectangle", qPrintable(r->name));
        return false;
    }
    if (!start(*r, {}, activationToken))
        return false;
    session(id)->send({ { u"type"_s, u"open"_s },
                        { u"path"_s, path },
                        { u"kind"_s, kind },
                        { u"position"_s, position >= 0 ? QJsonValue(position) : QJsonValue(QJsonValue::Null) },
                        { u"rect"_s, rectJson(rect) },
                        { u"parent"_s, QJsonValue::Null } });
    return true;
}

bool ProductHost::play(const QString &id, const QString &game, const QRect &rect, const QString &activationToken)
{
    const ProductRecord *r = record(id);
    if (!r || !r->commands.contains(u"play"_s) || game.isEmpty())
        return false;
    if (!place(id, rect, activationToken))
        return false;
    session(id)->send({ { u"type"_s, u"play"_s }, { u"game"_s, game } });
    return true;
}

bool ProductHost::launch(const QString &id, const QString &file, const QString &activationToken)
{
    const ProductRecord *r = record(id);
    if (!r || !r->method.isEmpty() || (!file.isEmpty() && r->fileArgument.isEmpty()))
        return false;
    if (const auto it = m_runs.constFind(id); it != m_runs.cend() && it->file != file) {
        qCInfo(lcProducts, "%s already runs on another file", qPrintable(r->name));
        return false;
    }
    return start(*r, file, activationToken);
}

void ProductHost::stop(const QString &id)
{
    const auto it = m_runs.find(id);
    const ProductRecord *r = record(id);
    if (it == m_runs.end() || !r)
        return;
    it->stopping = true;
    if (it->session) {
        if (it->session->welcomed() && r->commands.contains(u"quit"_s))
            it->session->send({ { u"type"_s, u"quit"_s } });
        else
            it->session->close();
    }
    // With no channel, nothing else asks it to end.
    endLater(it->process, it->session ? 5000 : 0);
}

bool ProductHost::start(const ProductRecord &record, const QString &file, const QString &activationToken)
{
    if (m_runs.contains(record.id))
        return true;
    const QString program = programFor(record.id);
    if (m_display.isEmpty() || program.isEmpty()) {
        qCInfo(lcProducts, "%s cannot be started: %s", qPrintable(record.name),
               m_display.isEmpty() ? "there is no display for it" : "its program is not installed");
        return false;
    }

    HostSession *session = nullptr;
    if (!record.method.isEmpty()) {
        session = new HostSession(record, this);
        if (!session->listen()) {
            qCWarning(lcProducts, "%s: its channel could not be opened", qPrintable(record.name));
            delete session;
            return false;
        }
    }
    auto *process = new QProcess(this);
    process->setProgram(program);
    if (session)
        process->setArguments({ u"--launcher-channel"_s, session->channel() });
    else if (!file.isEmpty())
        process->setArguments({ record.fileArgument, file });
    if (!session)
        process->setStandardInputFile(QProcess::nullDevice());
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(u"WAYLAND_DISPLAY"_s, m_display);
    environment.insert(u"QT_QPA_PLATFORM"_s, u"wayland"_s);
    environment.insert(u"QOR_REDUCED_MOTION"_s, m_reducedMotion ? u"1"_s : u"0"_s);
    // The colours it wears, for a product with no `theme` to follow (QQ).
    if (m_theme.isEmpty())
        environment.remove(u"QOR_THEME_COLORS"_s);
    else
        environment.insert(
            u"QOR_THEME_COLORS"_s,
            QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(m_theme)).toJson(QJsonDocument::Compact)));
    // A token Prism was started with was for its own window.
    environment.remove(u"XDG_ACTIVATION_TOKEN"_s);
    if (!activationToken.isEmpty())
        environment.insert(u"XDG_ACTIVATION_TOKEN"_s, activationToken);
    process->setProcessEnvironment(environment);
    // Its output goes where Prism's goes.
    process->setProcessChannelMode(QProcess::ForwardedChannels);
#if defined(Q_OS_LINUX)
    // It ends with Prism, however Prism ends.
    process->setChildProcessModifier([] { ::prctl(PR_SET_PDEATHSIG, SIGTERM); });
#endif

    const QString id = record.id;
    connect(process, &QProcess::finished, this, [this, id](int code, QProcess::ExitStatus status) {
        finish(id, status != QProcess::NormalExit || code != 0);
    });
    connect(process, &QProcess::errorOccurred, this, [this, id](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            qCWarning(lcProducts, "%s could not be started", qPrintable(id));
            finish(id, true);
        }
    });
    if (session) {
        connect(process, &QProcess::started, session, [process, session] {
            // The process Prism started is the one that may connect. It
            // connects once it has read its token, so this comes first.
            session->expect(process->processId());
            // The token on its stdin, which then closes: never an argument or
            // a variable another process could read.
            QByteArray line = session->token() + '\n';
            process->write(line);
            line.fill('\0');
            process->closeWriteChannel();
        });
        connect(session, &HostSession::welcomedChanged, this, [this, id] {
            emit connected(id);
            emit productsChanged();
        });
        connect(session, &HostSession::reported, this,
                [this, id](const QJsonObject &report) { emit reported(id, report.toVariantMap()); });
        // Without its channel a product has nothing to do: it quits by itself
        // once welcomed, and is ended if it does not.
        connect(session, &HostSession::ended, process, [process] { endLater(process); });
        if (record.commands.contains(u"prefs"_s))
            session->send(prefsCommand(m_reducedMotion));
        if (record.commands.contains(u"theme"_s) && !m_theme.isEmpty())
            session->send(themeCommand(m_theme));
    }

    m_runs.insert(id, { process, session, file });
    qCInfo(lcProducts, "Starting %s", qPrintable(record.name));
    process->start();
    emit productsChanged();
    return true;
}

void ProductHost::finish(const QString &id, bool failed)
{
    const auto it = m_runs.find(id);
    if (it == m_runs.end())
        return;
    const Run run = *it;
    m_runs.erase(it);
    const ProductRecord *r = record(id);
    // One with a channel that never said hello did not start properly.
    const bool started = (r && r->method.isEmpty()) || (run.session && run.session->welcomed());
    if (run.session) {
        run.session->close();
        run.session->deleteLater();
    }
    if (run.process) {
        run.process->disconnect(this);
        run.process->deleteLater();
    }
    qCInfo(lcProducts, "%s has ended", qPrintable(r ? r->name : id));
    emit productsChanged();
    emit exited(id, !run.stopping && (failed || !started));
}

} // namespace qor
