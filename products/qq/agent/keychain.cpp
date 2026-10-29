// Changed for QOR OS: on Linux the key is kept in the Secret Service, over D-Bus, as the creator's own key for a
// provider and not a QOR key (QOR OS's QOR-0011).

#include "keychain.h"

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <wincred.h>
#elif defined(Q_OS_LINUX)
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QEventLoop>
#include <QMap>
#include <QTimer>
#endif

namespace qq::keychain {

QString target()
{
    const QString set = qEnvironmentVariable("QQ_KEYCHAIN_TARGET");
    return set.isEmpty() ? QStringLiteral("QQ Studio/Anthropic API key") : set;
}

#ifdef Q_OS_WIN

QString store(const QString &secret)
{
    const std::wstring name = target().toStdWString();
    const QByteArray blob = secret.toUtf8();
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(name.c_str());
    credential.CredentialBlobSize = DWORD(blob.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(blob.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;  // this user, on this computer; not carried to others
    if (!CredWriteW(&credential, 0))
        return QStringLiteral("The key could not be kept in Windows Credential Manager (error %1).").arg(GetLastError());
    return {};
}

QString load()
{
    const std::wstring name = target().toStdWString();
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(name.c_str(), CRED_TYPE_GENERIC, 0, &credential))
        return {};
    const QString secret = QString::fromUtf8(reinterpret_cast<const char *>(credential->CredentialBlob),
                                             int(credential->CredentialBlobSize));
    SecureZeroMemory(credential->CredentialBlob, credential->CredentialBlobSize);
    CredFree(credential);
    return secret;
}

bool has()
{
    const std::wstring name = target().toStdWString();
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(name.c_str(), CRED_TYPE_GENERIC, 0, &credential))
        return false;
    SecureZeroMemory(credential->CredentialBlob, credential->CredentialBlobSize);
    CredFree(credential);
    return true;
}

bool remove()
{
    const std::wstring name = target().toStdWString();
    return CredDeleteW(name.c_str(), CRED_TYPE_GENERIC, 0);
}

#elif defined(Q_OS_LINUX)

// The Secret Service (freedesktop.org's specification) is the desktop's keyring: GNOME Keyring, KWallet and the like
// keep secrets for this user and unlock with their login. The key is an item in the keyring's default collection,
// found by its attributes. It crosses the session bus in a "plain" session, as it does for every program that uses
// the service without encryption; the bus is this user's own.

namespace {

const QString kService = QStringLiteral("org.freedesktop.secrets");
const QString kServicePath = QStringLiteral("/org/freedesktop/secrets");
const QString kServiceInterface = QStringLiteral("org.freedesktop.Secret.Service");
const QString kCollectionInterface = QStringLiteral("org.freedesktop.Secret.Collection");
const QString kItemInterface = QStringLiteral("org.freedesktop.Secret.Item");
const QString kSessionInterface = QStringLiteral("org.freedesktop.Secret.Session");
const QString kPromptInterface = QStringLiteral("org.freedesktop.Secret.Prompt");
constexpr int kPromptWaitMs = 5 * 60 * 1000;  // long enough to type a keyring's password

/// The specification's Secret: (session, parameters, value, content type), signature (oayays).
struct Secret {
    QDBusObjectPath session;
    QByteArray parameters;
    QByteArray value;
    QString contentType;
};

QDBusArgument &operator<<(QDBusArgument &argument, const Secret &secret)
{
    argument.beginStructure();
    argument << secret.session << secret.parameters << secret.value << secret.contentType;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Secret &secret)
{
    argument.beginStructure();
    argument >> secret.session >> secret.parameters >> secret.value >> secret.contentType;
    argument.endStructure();
    return argument;
}

using Attributes = QMap<QString, QString>;

bool isNone(const QDBusObjectPath &path)
{
    return path.path().isEmpty() || path.path() == QLatin1String("/");
}

/// One conversation with the Secret Service; `why` says what went wrong, if anything did.
class Keyring
{
public:
    Keyring() : m_bus(QDBusConnection::sessionBus())
    {
        static const bool registered = [] {
            qDBusRegisterMetaType<Secret>();
            qDBusRegisterMetaType<Attributes>();
            return true;
        }();
        Q_UNUSED(registered);
        if (!m_bus.isConnected())
            why = QStringLiteral("There is no session bus, so no keyring to keep the key in.");
    }

    ~Keyring()
    {
        if (!isNone(m_session))
            call(m_session.path(), kSessionInterface, QStringLiteral("Close"), {});
    }

    QString why;

    /// The items kept under this target, unlocked and locked.
    QList<QDBusObjectPath> find(QList<QDBusObjectPath> *locked = nullptr)
    {
        const QList<QVariant> out = call(kServicePath, kServiceInterface, QStringLiteral("SearchItems"),
                                         {QVariant::fromValue(attributes())});
        if (out.size() < 2)
            return {};
        if (locked)
            *locked = qdbus_cast<QList<QDBusObjectPath>>(out.at(1));
        return qdbus_cast<QList<QDBusObjectPath>>(out.at(0));
    }

    /// Unlocks `objects`, through the keyring's prompt if it shows one. True if every one is unlocked.
    bool unlock(const QList<QDBusObjectPath> &objects)
    {
        if (objects.isEmpty())
            return true;
        QList<QVariant> out =
            call(kServicePath, kServiceInterface, QStringLiteral("Unlock"), {QVariant::fromValue(objects)});
        if (out.size() < 2)
            return false;
        if (!isNone(qdbus_cast<QDBusObjectPath>(out.at(1)))) {
            if (!prompt(qdbus_cast<QDBusObjectPath>(out.at(1))))
                return false;
            // Asked again: what the prompt unlocked comes back unlocked, with no prompt.
            out = call(kServicePath, kServiceInterface, QStringLiteral("Unlock"), {QVariant::fromValue(objects)});
            if (out.size() < 2 || !isNone(qdbus_cast<QDBusObjectPath>(out.at(1))))
                return fail(QStringLiteral("The keyring stayed locked."));
        }
        if (qdbus_cast<QList<QDBusObjectPath>>(out.at(0)).size() < objects.size())
            return fail(QStringLiteral("The keyring stayed locked."));
        return true;
    }

    /// The keyring's default collection, unlocked: made first, through the keyring's prompt, if there is none (as
    /// libsecret does). None if it cannot be made or stays locked.
    QDBusObjectPath collection()
    {
        QDBusObjectPath path = alias();
        if (isNone(path) && why.isEmpty()) {
            const QVariantMap properties{
                {QStringLiteral("org.freedesktop.Secret.Collection.Label"), QStringLiteral("Login")}};
            const QList<QVariant> out = call(kServicePath, kServiceInterface, QStringLiteral("CreateCollection"),
                                             {properties, QStringLiteral("default")});
            if (out.size() >= 2 && !isNone(qdbus_cast<QDBusObjectPath>(out.at(1))))
                prompt(qdbus_cast<QDBusObjectPath>(out.at(1)));
            path = alias();
        }
        if (isNone(path)) {
            fail(QStringLiteral("The keyring has no default collection to keep the key in."));
            return {};
        }
        return unlock({path}) ? path : QDBusObjectPath();
    }

    QDBusObjectPath alias()
    {
        const QList<QVariant> out =
            call(kServicePath, kServiceInterface, QStringLiteral("ReadAlias"), {QStringLiteral("default")});
        return out.isEmpty() ? QDBusObjectPath() : qdbus_cast<QDBusObjectPath>(out.at(0));
    }

    /// A plain session for secrets to cross the bus in.
    QDBusObjectPath session()
    {
        if (isNone(m_session)) {
            const QList<QVariant> out = call(kServicePath, kServiceInterface, QStringLiteral("OpenSession"),
                                             {QStringLiteral("plain"), QVariant::fromValue(QDBusVariant(QString()))});
            if (out.size() >= 2)
                m_session = qdbus_cast<QDBusObjectPath>(out.at(1));
        }
        return m_session;
    }

    QList<QVariant> call(const QString &path, const QString &interface, const QString &method,
                         const QList<QVariant> &arguments)
    {
        m_failed = true;
        if (!m_bus.isConnected())
            return {};
        QDBusMessage message = QDBusMessage::createMethodCall(kService, path, interface, method);
        message.setArguments(arguments);
        const QDBusMessage reply = m_bus.call(message);
        m_failed = reply.type() == QDBusMessage::ErrorMessage;
        if (m_failed) {
            if (why.isEmpty()) {
                why = reply.errorName() == QLatin1String("org.freedesktop.DBus.Error.ServiceUnknown")
                          ? QStringLiteral("There is no keyring (a Secret Service) on this system to keep the key in.")
                          : QStringLiteral("The keyring said no (%1).").arg(reply.errorName());
            }
            return {};
        }
        return reply.arguments();
    }

    /// Shows the keyring's prompt (to unlock it, or to confirm) and waits for it to be answered.
    bool prompt(const QDBusObjectPath &path)
    {
        QEventLoop loop;
        // The answer comes as a signal; connected before the prompt is shown, so it cannot be missed.
        if (!m_bus.connect(kService, path.path(), kPromptInterface, QStringLiteral("Completed"), &loop, SLOT(quit())))
            return fail(QStringLiteral("The keyring's prompt could not be followed."));
        QTimer::singleShot(kPromptWaitMs, &loop, &QEventLoop::quit);
        call(path.path(), kPromptInterface, QStringLiteral("Prompt"), {QString()});  // no window of ours to sit over
        const bool shown = !m_failed;
        if (shown)
            loop.exec();
        m_bus.disconnect(kService, path.path(), kPromptInterface, QStringLiteral("Completed"), &loop, SLOT(quit()));
        return shown;
    }

    bool fail(const QString &message)
    {
        if (why.isEmpty())
            why = message;
        return false;
    }

    static Attributes attributes()
    {
        return {{QStringLiteral("application"), QStringLiteral("QQ Studio")}, {QStringLiteral("target"), target()}};
    }

private:
    QDBusConnection m_bus;
    QDBusObjectPath m_session;
    bool m_failed = false;  // whether the last call was answered with an error, or not made
};

}  // namespace

QString store(const QString &secret)
{
    Keyring keyring;
    const QDBusObjectPath collection = keyring.collection();
    const QDBusObjectPath session = isNone(collection) ? QDBusObjectPath() : keyring.session();
    if (isNone(session))
        return keyring.why.isEmpty() ? QStringLiteral("The key could not be kept in the keyring.") : keyring.why;
    Secret kept{session, {}, secret.toUtf8(), QStringLiteral("text/plain; charset=utf8")};
    const QVariantMap properties{
        {QStringLiteral("org.freedesktop.Secret.Item.Label"), target()},
        {QStringLiteral("org.freedesktop.Secret.Item.Attributes"), QVariant::fromValue(Keyring::attributes())}};
    const QList<QVariant> out = keyring.call(collection.path(), kCollectionInterface, QStringLiteral("CreateItem"),
                                             {properties, QVariant::fromValue(kept), true});
    kept.value.fill('\0');
    if (out.size() < 2)
        return keyring.why.isEmpty() ? QStringLiteral("The key could not be kept in the keyring.") : keyring.why;
    const QDBusObjectPath confirm = qdbus_cast<QDBusObjectPath>(out.at(1));
    if (!isNone(confirm) && (!keyring.prompt(confirm) || keyring.find().isEmpty()))
        return keyring.why.isEmpty() ? QStringLiteral("The key was not kept: the keyring's prompt was dismissed.")
                                     : keyring.why;
    return {};
}

QString load()
{
    Keyring keyring;
    QList<QDBusObjectPath> locked;
    QList<QDBusObjectPath> items = keyring.find(&locked);
    if (items.isEmpty() && !locked.isEmpty() && keyring.unlock(locked))
        items = locked;
    const QDBusObjectPath session = items.isEmpty() ? QDBusObjectPath() : keyring.session();
    if (isNone(session))
        return {};
    const QList<QVariant> out =
        keyring.call(items.first().path(), kItemInterface, QStringLiteral("GetSecret"), {QVariant::fromValue(session)});
    if (out.isEmpty())
        return {};
    Secret kept = qdbus_cast<Secret>(out.at(0));
    const QString secret = QString::fromUtf8(kept.value);
    kept.value.fill('\0');
    return secret;
}

bool has()
{
    Keyring keyring;
    QList<QDBusObjectPath> locked;
    return !keyring.find(&locked).isEmpty() || !locked.isEmpty();
}

bool remove()
{
    Keyring keyring;
    QList<QDBusObjectPath> locked;
    QList<QDBusObjectPath> items = keyring.find(&locked);
    if (!locked.isEmpty() && keyring.unlock(locked))
        items += locked;
    bool removed = false;
    for (const QDBusObjectPath &item : std::as_const(items)) {
        const QList<QVariant> out = keyring.call(item.path(), kItemInterface, QStringLiteral("Delete"), {});
        if (out.isEmpty())
            continue;
        const QDBusObjectPath confirm = qdbus_cast<QDBusObjectPath>(out.at(0));
        if (isNone(confirm) || keyring.prompt(confirm))
            removed = true;
    }
    return removed && keyring.find().isEmpty();
}

#else

QString store(const QString &)
{
    return QStringLiteral("There is no keychain for QQ Studio on this system yet.");
}
QString load() { return {}; }
bool has() { return false; }
bool remove() { return false; }

#endif

}  // namespace qq::keychain
