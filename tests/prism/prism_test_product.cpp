// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// prism-test-arqade, prism-test-media, prism-test-qq-studio and
// prism-test-qq-player: stand in for ARQADE, QOR Media, QQ Studio and the QQ
// Player in Prism's product test (tests/prism/products.e2e.json), as Wayland
// clients that show a plain window where those show theirs.
//
// ARQADE and QOR Media speak a product's side of the host protocol as those
// do:
//
//   - ARQADE stays hidden until `place`, then shows a window of the
//     rectangle's size (its Dock with no parent window). `prefs` and the game
//     `play` asks for show in its title.
//   - QOR Media shows when it is asked to `open` a file, sized by the
//     rectangle that comes with it, and names the file in its title.
//
// Each quits on `quit`, when its window is closed, or when its channel is
// lost, as they do.
//
// QQ Studio and the QQ Player read their arguments as those do and show at
// once, at a size of their own: the Player at its 1280 x 720, naming its
// scene in its title; the Studio at more than a 1920 x 1200 screen holds, so
// that Prism must fit it. A channel they were given shows in their title.
// Each quits when its window is closed.

#include "productside.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QFileInfo>
#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>
#include <QtGui/QRasterWindow>

namespace {

class Window : public QRasterWindow
{
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(QRect(QPoint(), size()), QColor(0x16, 0x12, 0x2a));
    }
};

// QQ Studio or the QQ Player, which Prism only starts.
int startedOnly(QGuiApplication &app, const QString &id, const QString &name)
{
    QCommandLineParser parser;
    const QCommandLineOption channel(QStringLiteral("launcher-channel"), QString(), QStringLiteral("name"));
    const QCommandLineOption project(QStringLiteral("project"), QString(), QStringLiteral("folder"));
    parser.addOptions({ channel, project });
    parser.addPositionalArgument(QStringLiteral("file"), QString());
    parser.process(app);

    Window window;
    const QString file = parser.isSet(project) ? parser.value(project) : parser.positionalArguments().value(0);
    QString title = file.isEmpty() ? QStringLiteral("Test %1").arg(name)
                                   : QStringLiteral("%1 — Test %2").arg(QFileInfo(file).fileName(), name);
    if (parser.isSet(channel))
        title += QStringLiteral(" · launcher channel");
    window.setTitle(title);
    window.resize(id == QStringLiteral("qq-player") ? QSize(1280, 720) : QSize(2000, 1300));
    window.show();
    return QGuiApplication::exec();
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    const QString id = QStringLiteral(PRODUCT_ID);
    const QString name = QStringLiteral(PRODUCT_NAME);
    if (id.startsWith(QStringLiteral("qq-")))
        return startedOnly(app, id, name);
    const QString method =
        id == QStringLiteral("arqade") ? QStringLiteral("arqade.next") : QStringLiteral("media.next");

    Window window;
    bool lessMotion = false;
    QString file;
    QString game;
    const auto retitle = [&] {
        QString title = file.isEmpty() ? QStringLiteral("Test %1").arg(name)
                                       : QStringLiteral("%1 — Test %2").arg(QFileInfo(file).fileName(), name);
        if (!game.isEmpty())
            title += QStringLiteral(" · playing %1").arg(game);
        if (lessMotion)
            title += QStringLiteral(" · less motion");
        window.setTitle(title);
    };
    retitle();
    // The rectangle is in physical pixels, as the launcher measures it.
    const auto place = [&](const QJsonObject &rect) {
        const qreal ratio = window.devicePixelRatio() > 0 ? window.devicePixelRatio() : 1;
        window.resize(qRound(rect.value(QStringLiteral("w")).toInt() / ratio),
                      qRound(rect.value(QStringLiteral("h")).toInt() / ratio));
    };

    ProductSide side;
    side.welcomed = [&](const QJsonObject &) { side.ask(method); };
    side.answered = [&](const QJsonValue &answer) {
        const QJsonObject command = answer.toObject();
        const QString type = command.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("quit")) {
            QCoreApplication::quit();
            return;
        }
        if (type == QStringLiteral("place")) {
            place(command.value(QStringLiteral("rect")).toObject());
            window.show();
        } else if (type == QStringLiteral("open")) {
            place(command.value(QStringLiteral("rect")).toObject());
            file = command.value(QStringLiteral("path")).toString();
            retitle();
            window.show();
        } else if (type == QStringLiteral("hide")) {
            window.hide();
        } else if (type == QStringLiteral("play")) {
            game = command.value(QStringLiteral("game")).toString();
            retitle();
        } else if (type == QStringLiteral("prefs")) {
            lessMotion =
                command.value(QStringLiteral("prefs")).toObject().value(QStringLiteral("reduced_motion")).toBool();
            retitle();
        }
        side.ask(method);
    };
    side.lost = [] { QCoreApplication::quit(); };
    side.connectTo(ProductSide::channelIn(app.arguments()),
                   { { QStringLiteral("type"), QStringLiteral("hello") },
                     { QStringLiteral("protocol"), 1 },
                     { QStringLiteral("min_protocol"), 1 },
                     { QStringLiteral("product"), id },
                     { QStringLiteral("version"), QStringLiteral("0.0.0") },
                     { QStringLiteral("token"), QString::fromLatin1(ProductSide::readToken()) } });
    return QGuiApplication::exec();
}
