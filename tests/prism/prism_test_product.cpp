// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// prism-test-arqade and prism-test-media: stand in for ARQADE and QOR Media
// in Prism's product test (tests/prism/products.e2e.json), as Wayland clients
// that speak a product's side of the host protocol as those do and show a
// plain window where they show theirs:
//
//   - ARQADE stays hidden until `place`, then shows a window of the
//     rectangle's size (its Dock with no parent window). `prefs` shows in its
//     title.
//   - QOR Media shows when it is asked to `open` a file, sized by the
//     rectangle that comes with it, and names the file in its title.
//
// Each quits on `quit`, when its window is closed, or when its channel is
// lost, as they do.

#include "productside.h"

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

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    const QString id = QStringLiteral(PRODUCT_ID);
    const QString name = QStringLiteral(PRODUCT_NAME);
    const QString method =
        id == QStringLiteral("arqade") ? QStringLiteral("arqade.next") : QStringLiteral("media.next");

    Window window;
    bool lessMotion = false;
    QString file;
    const auto retitle = [&] {
        QString title = file.isEmpty() ? QStringLiteral("Test %1").arg(name)
                                       : QStringLiteral("%1 — Test %2").arg(QFileInfo(file).fileName(), name);
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
