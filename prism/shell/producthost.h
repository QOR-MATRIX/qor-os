// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <optional>

class QLocalServer;
class QLocalSocket;
class QProcess;
class QTemporaryDir;

namespace qor {

// Prism's record of a product it hosts: how a person knows it, where its
// program is, and what it speaks. A product is always named and laid out from
// this record, never from what it says about itself.
struct ProductRecord
{
    QString id;              // what its hello must say: "arqade"
    QString name;            // what a person reads: "ARQADE"
    QString variable;        // the environment variable that may name its program: "QOR_ARQADE"
    QString method;          // its long poll, the one method Prism offers it: "arqade.next"; empty for a
                             // product Prism only starts (QQ), which is served no channel
    int waitMs = 3000;       // how long the long poll waits for a command
    QStringList commands;    // the commands it understands
    QString fileArgument;    // what comes before a file it is started on: "--project", or "--" for a first argument
    QString glyph;           // its icon in the Shelf and Beam
    QSize size;              // the content size it opens at, where there is room; empty: the size it asks for
    bool launchable = false; // opened by itself (ARQADE), or only to show a file (QOR Media)
    bool takesController = false; // a game, which plays with the controller while its window has the focus
                                  // (the QQ Player): the shell's controller interface steps aside
    QString keywords;
    QString description;
    int hue = 0;
    double chroma = 0;
};

// One run of a product, as Prism serves its channel: Prism's side of the QOR
// host protocol, version 1 (demiurge-chain's ADR-046; its launcher speaks the
// same, `tools/qor-launcher/src-tauri/src/host/`).
//
// - listen() opens a channel only this user can open: a local socket in a
//   folder of its own (Linux), or a named pipe (Windows). It takes one
//   connection, and only from the process expect() names, as the operating
//   system reports it; then it closes to everyone.
// - The first frame must be a hello carrying the run's token within
//   `TokenLifetimeMs` of expect(). The token is compared in constant time and
//   used once. Prism agrees version 1 and offers one method, the record's
//   long poll.
// - Each long poll is answered with the oldest command waiting, or held until
//   a command comes or the record's wait ends, then answered with none. A
//   newer place, theme, player, prefs or play replaces one not yet sent.
//
// Frames are a 4-byte big-endian length and a JSON object, at most 1 MiB. The
// token and the session's id are never logged.
class HostSession : public QObject
{
    Q_OBJECT

public:
    static constexpr int Protocol = 1;
    static constexpr qint64 FrameLimit = 1 << 20;
    static constexpr int TokenLifetimeMs = 30000;

    explicit HostSession(const ProductRecord &record, QObject *parent = nullptr);
    ~HostSession() override;

    // Opens the channel. False if it could not be.
    bool listen();
    // What the product is told to connect to: a name, not a secret.
    QString channel() const { return m_channel; }
    // The one-time token, for the product's stdin: 32 random bytes as hex.
    QByteArray token() const { return m_token; }
    // The process that may connect, from now on, for the token's lifetime.
    void expect(qint64 pid, int lifetimeMs = TokenLifetimeMs);

    // Queues a command for the product's next long poll.
    void send(const QJsonObject &command);
    // Closes the channel: the product sees it lost, and quits.
    void close();

    bool welcomed() const { return m_welcomed; }
    // Commands waiting, oldest first (for the tests).
    QJsonArray waiting() const;

signals:
    void welcomedChanged();
    // The session is over: the product closed its channel, broke the
    // protocol, or never said hello in time. `reason` is for the log, and
    // never holds the token or the session's id.
    void ended(const QString &reason);

private:
    void accept();
    void readFrames();
    void handle(const QByteArray &body);
    void hello(const QJsonObject &frame);
    void request(const QJsonObject &frame);
    void answer(const QJsonValue &command);
    void write(const QJsonObject &frame);
    void refuse(const QString &code, const QString &message, const QString &reason);
    void end(const QString &reason, bool gracefully = true);
    void forgetToken();

    ProductRecord m_record;
    std::unique_ptr<QTemporaryDir> m_folder;
    QLocalServer *m_server = nullptr;
    QLocalSocket *m_socket = nullptr;
    QString m_channel;
    QByteArray m_token;
    std::optional<qint64> m_expected;
    QElapsedTimer m_issued;
    int m_lifetimeMs = TokenLifetimeMs;
    QTimer m_deadline;
    QTimer m_wait;
    QByteArray m_buffer;
    bool m_welcomed = false;
    bool m_ended = false;
    // The long poll being held, by its request id.
    std::optional<qint64> m_held;
    QList<QJsonObject> m_commands;
};

// QML singleton `ProductHost`: Prism hosts QOR OS's products, ARQADE, QOR
// Media, QQ Studio and the QQ Player, as demiurge-chain's QOR Launcher does on
// Windows (QOR-0010).
//
// It starts a product as a Wayland client of Prism (WAYLAND_DISPLAY is
// `display`). ARQADE and QOR Media get `--launcher-channel <name>` and their
// run's token on stdin, never in their arguments or environment, and Prism
// serves their channel (HostSession). On Windows the launcher docks their
// windows inside its own; on Wayland a window stays a toplevel, so `place`
// carries the content rectangle Prism planned for it and no parent, and Prism
// lays out the window when it arrives: productOf() tells which product a
// client is by its process.
//
// QQ Studio and the QQ Player are only started, with no channel: QQ Studio's
// is for publishing, which Prism does not offer before the vault service
// (Phase 2), and the QQ Player has none. Their windows open at the size they
// ask for.
//
// A product ends with Prism: its channel closes and it quits, or, with no
// channel, it is asked to end. On Linux it is also sent SIGTERM if Prism dies
// first.
class ProductHost : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // The Wayland socket the products open their windows on: Prism's. Empty
    // where nothing can host them (qor-shell), so none is available.
    Q_PROPERTY(QString display READ display WRITE setDisplay NOTIFY displayChanged)
    // The person's Motion setting: passed to every product as it starts
    // (QOR_REDUCED_MOTION), and to those that follow `prefs` while they run.
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY reducedMotionChanged)
    // Every product: product(id) for each.
    Q_PROPERTY(QVariantList products READ products NOTIFY productsChanged)

public:
    explicit ProductHost(QObject *parent = nullptr);
    explicit ProductHost(const QList<ProductRecord> &records, QObject *parent = nullptr);
    ~ProductHost() override;

    // ARQADE, QOR Media, QQ Studio and the QQ Player.
    static QList<ProductRecord> defaultRecords();

    QString display() const { return m_display; }
    void setDisplay(const QString &display);
    bool reducedMotion() const { return m_reducedMotion; }
    void setReducedMotion(bool reduced);
    QVariantList products() const;

    // { id, name, glyph, width, height, launchable, placed, takesController,
    //   keywords, description, hue, chroma, available, running }, or an empty
    // map for no product. `placed`: Prism tells it where to show (place);
    // otherwise it is only started, and width and height are 0.
    Q_INVOKABLE QVariantMap product(const QString &id) const;
    // The product's program, if it can be started: the file its variable
    // names, else `<id>` beside Prism's own program.
    Q_INVOKABLE QString programFor(const QString &id) const;
    // The product a Wayland client's process is, or "".
    Q_INVOKABLE QString productOf(qint64 pid) const;

    // Shows the product with its content at `rect` of the output, in physical
    // pixels, starting it first if it is not running. `activationToken` (from
    // xdg-activation) lets its first window take focus. False if it cannot be
    // started.
    Q_INVOKABLE bool place(const QString &id, const QRect &rect, const QString &activationToken = QString());
    // QOR Media: opens `path` (of `kind`: video, audio, picture, document)
    // with its content at `rect`, starting it first if need be.
    Q_INVOKABLE bool open(const QString &id, const QString &path, const QString &kind, const QRect &rect,
                          const QString &activationToken = QString());
    // Starts a product Prism only starts (QQ Studio, the QQ Player), on `file`
    // if one is given: a project, a scene. True if it now runs on that file;
    // false if it cannot be started, or already runs on another.
    Q_INVOKABLE bool launch(const QString &id, const QString &file = QString(),
                            const QString &activationToken = QString());
    // Asks a running product to quit, and ends it if it has not within a few
    // seconds. One with no channel is asked at once. Not a failure (exited).
    Q_INVOKABLE void stop(const QString &id);

    // The running product's session (for the tests).
    HostSession *session(const QString &id) const;

signals:
    void displayChanged();
    void reducedMotionChanged();
    void productsChanged();
    // The product said hello and was welcomed.
    void connected(const QString &id);
    // The product's process ended: `failed` when it crashed, exited with an
    // error, or never connected.
    void exited(const QString &id, bool failed);

private:
    struct Run
    {
        QPointer<QProcess> process;
        QPointer<HostSession> session; // none for a product with no channel
        QString file;                  // what it was started on (launch)
        bool stopping = false;         // stop() was asked: its end is no failure
    };

    const ProductRecord *record(const QString &id) const;
    bool start(const ProductRecord &record, const QString &file, const QString &activationToken);
    void finish(const QString &id, bool failed);
    static QJsonObject rectJson(const QRect &rect);

    QList<ProductRecord> m_records;
    QHash<QString, Run> m_runs;
    QString m_display;
    bool m_reducedMotion = false;
};

} // namespace qor
