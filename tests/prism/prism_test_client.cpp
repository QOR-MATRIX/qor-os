// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// prism-test-client: a Wayland client with a known layout, for Prism's
// end-to-end tests (tools/dev/prism-e2e.py).
//
// The "Menu" button sits at (20, 20) in the window and opens a QMenu, an
// xdg_popup, directly below it, so a test can click it and check where Prism
// places the popup. Pressing "Move" (160, 20) or "Resize" (300, 20) starts a
// client-initiated move or bottom-right resize (xdg_toplevel.move/resize), as
// client-side decorated apps do from their own title bar and borders.
// "Activate later" (440, 20) asks for focus 1.5 s after it is clicked
// (xdg_activation_v1), long enough for a test to focus another window first.
// Title, app id and size limits are set from the command line.

#include <QtCore/QCommandLineParser>
#include <QtCore/QTimer>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

using namespace Qt::StringLiterals;

namespace {

QSize parseSize(const QString &text)
{
    const QStringList parts = text.split(u'x');
    return parts.size() == 2 ? QSize(parts[0].toInt(), parts[1].toInt()) : QSize();
}

class Window : public QWidget
{
public:
    Window()
    {
        auto *button = new QPushButton(u"Menu"_s, this);
        button->setGeometry(20, 20, 120, 32);
        auto *menu = new QMenu(this);
        menu->addAction(u"First"_s);
        menu->addAction(u"Second"_s);
        menu->addAction(u"Third"_s);
        connect(button, &QPushButton::clicked, this,
                [button, menu] { menu->popup(button->mapToGlobal(QPoint(0, button->height()))); });
        auto *move = new QPushButton(u"Move"_s, this);
        move->setGeometry(160, 20, 120, 32);
        connect(move, &QPushButton::pressed, this, [this] { windowHandle()->startSystemMove(); });
        auto *resize = new QPushButton(u"Resize"_s, this);
        resize->setGeometry(300, 20, 120, 32);
        connect(resize, &QPushButton::pressed, this,
                [this] { windowHandle()->startSystemResize(Qt::RightEdge | Qt::BottomEdge); });
        auto *activate = new QPushButton(u"Activate later"_s, this);
        activate->setGeometry(440, 20, 140, 32);
        connect(activate, &QPushButton::clicked, this,
                [this] { QTimer::singleShot(1500, this, [this] { activateWindow(); }); });
        m_size = new QLabel(this);
        m_size->setGeometry(20, 70, 300, 24);
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        m_size->setText(u"%1 × %2"_s.arg(event->size().width()).arg(event->size().height()));
    }

private:
    QLabel *m_size;
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption title(u"title"_s, u"Window title."_s, u"text"_s, u"Prism Test Client"_s);
    const QCommandLineOption appId(u"app-id"_s, u"Wayland app id (desktop file name)."_s, u"id"_s);
    const QCommandLineOption size(u"size"_s, u"Initial size, e.g. 640x420."_s, u"WxH"_s, u"640x420"_s);
    const QCommandLineOption minimum(u"min"_s, u"Minimum size."_s, u"WxH"_s);
    const QCommandLineOption maximum(u"max"_s, u"Maximum size."_s, u"WxH"_s);
    parser.addOptions({ title, appId, size, minimum, maximum });
    parser.process(app);

    if (parser.isSet(appId))
        QGuiApplication::setDesktopFileName(parser.value(appId));

    Window window;
    window.setWindowTitle(parser.value(title));
    window.resize(parseSize(parser.value(size)));
    if (parser.isSet(minimum))
        window.setMinimumSize(parseSize(parser.value(minimum)));
    if (parser.isSet(maximum))
        window.setMaximumSize(parseSize(parser.value(maximum)));
    window.show();
    return app.exec();
}
