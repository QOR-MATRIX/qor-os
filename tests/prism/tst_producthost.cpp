// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Prism's side of the QOR host protocol (prism/shell/producthost.h): first
// with the test as the product, on a channel in this process, then with a real
// process ProductHost starts (tst_producthost_product, which has no window).

#include "producthost.h"
#include "productside.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QTemporaryDir>
#include <QtNetwork/QLocalSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <memory>

using namespace Qt::StringLiterals;
using qor::HostSession;
using qor::ProductHost;
using qor::ProductRecord;

namespace {

// Everything logged while the tests run, at every level, to look for secrets.
QMutex logMutex;
QStringList logged;
QtMessageHandler previousHandler = nullptr;

void capture(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    {
        const QMutexLocker locker(&logMutex);
        logged.append(message);
    }
    if (previousHandler)
        previousHandler(type, context, message);
}

ProductRecord testRecord(int waitMs = 200)
{
    ProductRecord record;
    record.id = u"test-product"_s;
    record.name = u"Test Product"_s;
    record.variable = u"QOR_TEST_PRODUCT_PROGRAM"_s;
    record.method = u"test.next"_s;
    record.waitMs = waitMs;
    record.commands = { u"place"_s, u"open"_s, u"prefs"_s, u"quit"_s };
    record.glyph = u"app-window"_s;
    record.size = QSize(800, 600);
    record.launchable = true;
    return record;
}

QJsonObject hello(const QByteArray &token, const QString &product = u"test-product"_s, int protocol = 1,
                  int minimum = 1)
{
    return { { u"type"_s, u"hello"_s }, { u"protocol"_s, protocol },  { u"min_protocol"_s, minimum },
             { u"product"_s, product }, { u"version"_s, u"0.0.0"_s }, { u"token"_s, QString::fromLatin1(token) } };
}

QJsonObject place(int x, int y, int w, int h)
{
    return { { u"type"_s, u"place"_s },
             { u"rect"_s, QJsonObject { { u"x"_s, x }, { u"y"_s, y }, { u"w"_s, w }, { u"h"_s, h } } },
             { u"parent"_s, QJsonValue::Null } };
}

// The test as the product.
struct Product
{
    ProductSide side;
    QList<QJsonObject> welcomes;
    QList<QJsonObject> refusals;
    QList<QJsonValue> answers;
    bool lost = false;

    Product()
    {
        side.welcomed = [this](const QJsonObject &frame) { welcomes.append(frame); };
        side.refused = [this](const QJsonObject &frame) { refusals.append(frame); };
        side.answered = [this](const QJsonValue &command) { answers.append(command); };
        side.lost = [this] { lost = true; };
    }
};

// A session in this process, which this process may connect to.
std::unique_ptr<HostSession> openSession(const ProductRecord &record = testRecord())
{
    auto session = std::make_unique<HostSession>(record);
    if (!session->listen())
        return nullptr;
    session->expect(QCoreApplication::applicationPid());
    return session;
}

// What tst_producthost_product wrote, by key, in order.
QList<QJsonObject> readLog(const QString &path)
{
    QList<QJsonObject> entries;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly))
        for (const QByteArray &line : file.readAll().split('\n'))
            if (!line.trimmed().isEmpty())
                entries.append(QJsonDocument::fromJson(line).object());
    return entries;
}

QList<QJsonValue> entriesOf(const QString &path, const QString &key)
{
    QList<QJsonValue> values;
    for (const QJsonObject &entry : readLog(path))
        if (entry.contains(key))
            values.append(entry.value(key));
    return values;
}

} // namespace

class tst_ProductHost : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_log;

    // A host whose one product is tst_producthost_product, writing to m_log.
    std::unique_ptr<ProductHost> host(const QByteArray &mode = {})
    {
        QFile::remove(m_log);
        qputenv("QOR_TEST_PRODUCT_PROGRAM", QOR_TEST_PRODUCT);
        qputenv("QOR_TEST_PRODUCT_LOG", m_log.toUtf8());
        qputenv("QOR_TEST_PRODUCT_ID", "test-product");
        qputenv("QOR_TEST_PRODUCT_METHOD", "test.next");
        qputenv("QOR_TEST_PRODUCT_MODE", mode);
        auto made = std::make_unique<ProductHost>(QList<ProductRecord> { testRecord() });
        made->setDisplay(u"wayland-test"_s);
        return made;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_log = m_dir.filePath(u"product.log"_s);
        previousHandler = qInstallMessageHandler(capture);
        QLoggingCategory::setFilterRules(u"*.debug=true"_s);
    }

    // ── The handshake ──────────────────────────────────────────────────────

    void a_good_hello_is_welcomed_with_its_one_method()
    {
        auto session = openSession();
        QVERIFY(session);
        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(product.welcomes.size(), 1);
        const QJsonObject welcome = product.welcomes.first();
        QCOMPARE(welcome.value(u"protocol"_s).toInt(), 1);
        QCOMPARE(welcome.value(u"capabilities"_s).toArray(), QJsonArray { u"test.next"_s });
        QCOMPARE(welcome.value(u"session"_s).toString().size(), 32);
        QVERIFY(session->welcomed());
        // Used once, and forgotten.
        QVERIFY(session->token().isEmpty());
    }

    void a_wrong_token_is_refused()
    {
        auto session = openSession();
        QSignalSpy ended(session.get(), &HostSession::ended);
        Product product;
        product.side.connectTo(session->channel(), hello(QByteArray(64, '0')));
        QTRY_COMPARE(product.refusals.size(), 1);
        QCOMPARE(product.refusals.first().value(u"code"_s).toString(), u"not_authorised"_s);
        QVERIFY(product.refusals.first().value(u"id"_s).isNull());
        QTRY_VERIFY(product.lost);
        QVERIFY(!session->welcomed());
        QCOMPARE(ended.size(), 1);
        QVERIFY(session->token().isEmpty());
    }

    void a_hello_from_another_product_is_refused()
    {
        auto session = openSession();
        Product product;
        product.side.connectTo(session->channel(), hello(session->token(), u"another"_s));
        QTRY_COMPARE(product.refusals.size(), 1);
        QCOMPARE(product.refusals.first().value(u"code"_s).toString(), u"not_authorised"_s);
        QTRY_VERIFY(product.lost);
    }

    void the_first_frame_must_be_a_hello()
    {
        auto session = openSession();
        Product product;
        product.side.connectTo(session->channel(),
                               { { u"type"_s, u"request"_s }, { u"id"_s, 1 }, { u"method"_s, u"test.next"_s } });
        QTRY_COMPARE(product.refusals.size(), 1);
        QCOMPARE(product.refusals.first().value(u"code"_s).toString(), u"bad_request"_s);
        QTRY_VERIFY(product.lost);
    }

    void a_second_hello_is_refused()
    {
        auto session = openSession();
        const QByteArray token = session->token();
        Product product;
        product.side.connectTo(session->channel(), hello(token));
        QTRY_COMPARE(product.welcomes.size(), 1);
        product.side.send(hello(token));
        QTRY_COMPARE(product.refusals.size(), 1);
        QCOMPARE(product.refusals.first().value(u"code"_s).toString(), u"bad_request"_s);
        QTRY_VERIFY(product.lost);
    }

    void the_newest_protocol_both_speak_is_chosen_data()
    {
        QTest::addColumn<int>("protocol");
        QTest::addColumn<int>("minimum");
        QTest::addColumn<bool>("welcomed");
        QTest::newRow("the same") << 1 << 1 << true;
        QTest::newRow("a newer product that still speaks 1") << 2 << 1 << true;
        QTest::newRow("a product too new") << 3 << 2 << false;
        QTest::newRow("no version said") << 0 << 0 << false;
    }

    void the_newest_protocol_both_speak_is_chosen()
    {
        QFETCH(int, protocol);
        QFETCH(int, minimum);
        QFETCH(bool, welcomed);
        auto session = openSession();
        Product product;
        product.side.connectTo(session->channel(), hello(session->token(), u"test-product"_s, protocol, minimum));
        if (welcomed) {
            QTRY_COMPARE(product.welcomes.size(), 1);
            QCOMPARE(product.welcomes.first().value(u"protocol"_s).toInt(), 1);
        } else {
            QTRY_COMPARE(product.refusals.size(), 1);
            QCOMPARE(product.refusals.first().value(u"code"_s).toString(), u"protocol_mismatch"_s);
            QTRY_VERIFY(product.lost);
        }
    }

    void a_product_that_never_says_hello_is_dropped()
    {
        auto session = std::make_unique<HostSession>(testRecord());
        QVERIFY(session->listen());
        session->expect(QCoreApplication::applicationPid(), 300);
        QSignalSpy ended(session.get(), &HostSession::ended);
        QLocalSocket quiet;
        quiet.connectToServer(session->channel());
        QVERIFY(quiet.waitForConnected(5000));
        QTRY_COMPARE(ended.size(), 1);
        QVERIFY(ended.first().first().toString().contains(u"in time"_s));
        QTRY_COMPARE(quiet.state(), QLocalSocket::UnconnectedState);
    }

    // ── Who may connect ────────────────────────────────────────────────────

    void only_the_process_prism_started_may_connect()
    {
        auto session = std::make_unique<HostSession>(testRecord());
        QVERIFY(session->listen());
        // Another process than this one.
        session->expect(QCoreApplication::applicationPid() + 1);
        QSignalSpy ended(session.get(), &HostSession::ended);
        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(ended.size(), 1);
        QVERIFY(ended.first().first().toString().contains(u"not from the product"_s));
        QTRY_VERIFY(product.lost);
        QVERIFY(product.welcomes.isEmpty());
    }

    void the_channel_takes_one_connection()
    {
        auto session = openSession();
        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(product.welcomes.size(), 1);
        QLocalSocket second;
        second.connectToServer(session->channel());
        QVERIFY(!second.waitForConnected(1000));
    }

    // ── Requests ───────────────────────────────────────────────────────────

    void a_method_not_offered_is_answered_unsupported()
    {
        auto session = openSession();
        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(product.welcomes.size(), 1);
        product.side.ask(u"test.publish"_s);
        QTRY_COMPARE(product.refusals.size(), 1);
        QCOMPARE(product.refusals.first().value(u"code"_s).toString(), u"unsupported_method"_s);
        QCOMPARE(product.refusals.first().value(u"id"_s).toInt(), 1);
        // The session goes on.
        product.side.ask(u"test.next"_s);
        session->send(place(0, 0, 640, 480));
        QTRY_COMPARE(product.answers.size(), 1);
        QVERIFY(!product.lost);
    }

    void a_frame_over_a_mebibyte_ends_the_session()
    {
        auto session = openSession();
        QSignalSpy ended(session.get(), &HostSession::ended);
        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(product.welcomes.size(), 1);
        QByteArray length(4, '\0');
        qToBigEndian(quint32(2 << 20), length.data());
        product.side.writeRaw(length);
        QTRY_COMPARE(ended.size(), 1);
        QTRY_VERIFY(product.lost);
    }

    void the_long_poll_answers_commands_in_order()
    {
        auto session = openSession();
        const QJsonObject open { { u"type"_s, u"open"_s }, { u"path"_s, u"/films/a.mkv"_s } };
        // Sent before the product is there: they wait for it.
        session->send(place(0, 0, 100, 100));
        session->send(open);
        session->send(place(10, 20, 640, 480));
        // A newer place replaced the one not yet sent.
        QCOMPARE(session->waiting(), (QJsonArray { open, place(10, 20, 640, 480) }));

        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(product.welcomes.size(), 1);
        product.side.ask(u"test.next"_s);
        QTRY_COMPARE(product.answers.size(), 1);
        product.side.ask(u"test.next"_s);
        QTRY_COMPARE(product.answers.size(), 2);
        QCOMPARE(product.answers.at(0).toObject(), open);
        QCOMPARE(product.answers.at(1).toObject(), place(10, 20, 640, 480));
        QVERIFY(session->waiting().isEmpty());
    }

    void the_long_poll_answers_nothing_after_its_wait()
    {
        auto session = openSession(testRecord(150));
        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(product.welcomes.size(), 1);
        QElapsedTimer clock;
        clock.start();
        product.side.ask(u"test.next"_s);
        QTRY_COMPARE(product.answers.size(), 1);
        QVERIFY(product.answers.first().isNull());
        QVERIFY(clock.elapsed() >= 140);
    }

    void a_command_answers_a_held_poll_at_once()
    {
        auto session = openSession(testRecord(60000));
        Product product;
        product.side.connectTo(session->channel(), hello(session->token()));
        QTRY_COMPARE(product.welcomes.size(), 1);
        product.side.ask(u"test.next"_s);
        QTest::qWait(100);
        QVERIFY(product.answers.isEmpty());
        session->send(place(0, 0, 640, 480));
        QTRY_COMPARE_WITH_TIMEOUT(product.answers.size(), 1, 2000);
        QCOMPARE(product.answers.first().toObject(), place(0, 0, 640, 480));
    }

    // ── ProductHost, with a process of its own ─────────────────────────────

    void nothing_is_offered_without_a_display_or_a_program()
    {
        qputenv("QOR_TEST_PRODUCT_PROGRAM", QOR_TEST_PRODUCT);
        ProductHost host(QList<ProductRecord> { testRecord() });
        QCOMPARE(host.product(u"test-product"_s).value(u"available"_s).toBool(), false);
        QVERIFY(!host.place(u"test-product"_s, QRect(0, 0, 800, 600)));
        host.setDisplay(u"wayland-test"_s);
        QCOMPARE(host.product(u"test-product"_s).value(u"available"_s).toBool(), true);
        QCOMPARE(host.product(u"test-product"_s).value(u"name"_s).toString(), u"Test Product"_s);
        QVERIFY(host.product(u"another"_s).isEmpty());

        qputenv("QOR_TEST_PRODUCT_PROGRAM", m_dir.filePath(u"nothing-here"_s).toUtf8());
        QCOMPARE(host.product(u"test-product"_s).value(u"available"_s).toBool(), false);
        QVERIFY(!host.place(u"test-product"_s, QRect(0, 0, 800, 600)));
        QCOMPARE(host.productOf(QCoreApplication::applicationPid()), QString());
    }

    void a_product_starts_with_its_token_on_stdin_only()
    {
        auto host = this->host();
        QSignalSpy connected(host.get(), &ProductHost::connected);
        QSignalSpy exited(host.get(), &ProductHost::exited);
        QVERIFY(host->place(u"test-product"_s, QRect(10, 20, 800, 600), u"activation-1"_s));
        const QByteArray token = host->session(u"test-product"_s)->token();
        QCOMPARE(token.size(), 64);
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, 10000);

        const QJsonObject started = entriesOf(m_log, u"started"_s).value(0).toObject();
        const QJsonArray arguments = started.value(u"arguments"_s).toArray();
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(arguments.at(0).toString(), u"--launcher-channel"_s);
        QCOMPARE(arguments.at(1).toString(), host->session(u"test-product"_s)->channel());
        const QJsonObject environment = started.value(u"environment"_s).toObject();
        QCOMPARE(environment.value(u"WAYLAND_DISPLAY"_s).toString(), u"wayland-test"_s);
        QCOMPARE(environment.value(u"QT_QPA_PLATFORM"_s).toString(), u"wayland"_s);
        QCOMPARE(environment.value(u"QOR_REDUCED_MOTION"_s).toString(), u"0"_s);
        QCOMPARE(environment.value(u"XDG_ACTIVATION_TOKEN"_s).toString(), u"activation-1"_s);
        QCOMPARE(entriesOf(m_log, u"tokenLength"_s).value(0).toInt(), 64);
        const QByteArray seen = QJsonDocument(started).toJson();
        QVERIFY(!seen.contains(token));

        // Its windows are known by its process.
        const qint64 pid = started.value(u"pid"_s).toInteger();
        QCOMPARE(host->productOf(pid), u"test-product"_s);
        QCOMPARE(host->product(u"test-product"_s).value(u"running"_s).toBool(), true);

        // Its Motion setting first, then where to show.
        QTRY_COMPARE(entriesOf(m_log, u"command"_s).size(), 2);
        const QList<QJsonValue> commands = entriesOf(m_log, u"command"_s);
        QCOMPARE(commands.at(0).toObject(),
                 (QJsonObject { { u"type"_s, u"prefs"_s },
                                { u"prefs"_s, QJsonObject { { u"reduced_motion"_s, false } } } }));
        QCOMPARE(commands.at(1).toObject(), place(10, 20, 800, 600));

        host->stop(u"test-product"_s);
        QTRY_COMPARE_WITH_TIMEOUT(exited.size(), 1, 10000);
        QCOMPARE(exited.first().at(0).toString(), u"test-product"_s);
        QCOMPARE(exited.first().at(1).toBool(), false);
        QCOMPARE(host->productOf(pid), QString());
        QCOMPARE(host->product(u"test-product"_s).value(u"running"_s).toBool(), false);
    }

    void the_motion_setting_reaches_a_running_product()
    {
        auto host = this->host();
        QSignalSpy connected(host.get(), &ProductHost::connected);
        QVERIFY(host->place(u"test-product"_s, QRect(0, 0, 800, 600)));
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, 10000);
        QTRY_COMPARE(entriesOf(m_log, u"command"_s).size(), 2);
        host->setReducedMotion(true);
        QTRY_COMPARE(entriesOf(m_log, u"command"_s).size(), 3);
        QCOMPARE(entriesOf(m_log, u"command"_s).at(2).toObject().value(u"prefs"_s).toObject(),
                 (QJsonObject { { u"reduced_motion"_s, true } }));
    }

    void a_product_ends_with_prism()
    {
        auto host = this->host();
        QSignalSpy connected(host.get(), &ProductHost::connected);
        QVERIFY(host->place(u"test-product"_s, QRect(0, 0, 800, 600)));
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, 10000);
        host.reset();
        // It saw its channel close, and quit.
        QCOMPARE(entriesOf(m_log, u"lost"_s).size(), 1);
    }

    void a_process_with_the_wrong_token_is_ended()
    {
        auto host = this->host("wrong-token");
        QSignalSpy exited(host.get(), &ProductHost::exited);
        QVERIFY(host->place(u"test-product"_s, QRect(0, 0, 800, 600)));
        QTRY_COMPARE_WITH_TIMEOUT(exited.size(), 1, 15000);
        QCOMPARE(exited.first().at(1).toBool(), true);
        QCOMPARE(entriesOf(m_log, u"refused"_s).value(0).toObject().value(u"code"_s).toString(), u"not_authorised"_s);
        QVERIFY(entriesOf(m_log, u"command"_s).isEmpty());
    }

    void nothing_secret_reaches_the_log()
    {
        {
            const QMutexLocker locker(&logMutex);
            logged.clear();
        }
        auto host = this->host();
        QSignalSpy connected(host.get(), &ProductHost::connected);
        QSignalSpy exited(host.get(), &ProductHost::exited);
        QVERIFY(host->place(u"test-product"_s, QRect(0, 0, 800, 600)));
        const QByteArray token = host->session(u"test-product"_s)->token();
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, 10000);
        const QString sessionId = entriesOf(m_log, u"welcome"_s).value(0).toObject().value(u"session"_s).toString();
        QCOMPARE(sessionId.size(), 32);
        host->stop(u"test-product"_s);
        QTRY_COMPARE_WITH_TIMEOUT(exited.size(), 1, 10000);

        const QMutexLocker locker(&logMutex);
        QVERIFY(!logged.isEmpty());
        for (const QString &message : std::as_const(logged)) {
            QVERIFY2(!message.contains(QString::fromLatin1(token)), qPrintable(message));
            QVERIFY2(!message.contains(sessionId), qPrintable(message));
        }
    }
};

QTEST_GUILESS_MAIN(tst_ProductHost)
#include "tst_producthost.moc"
