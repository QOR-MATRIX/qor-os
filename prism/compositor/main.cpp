// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// prism — the QOR OS Wayland compositor (Linux).
//
// Hosts the QOR.Shell desktop inside a Qt Wayland Compositor scene. Run it
// nested for development:
//
//   prism --unlocked            # inside an existing Wayland/X11 session or WSLg
//   WAYLAND_DISPLAY=wayland-1 weston-terminal   # launch a client into Prism
//
// Prism prints the socket name it listens on (the first free wayland-N, or
// --socket).
//
// Status: experimental, see docs/decisions/qtos/QTOS-0003-compositor-strategy.md and
// docs/decisions/QOR-0005-every-display-at-its-own-refresh.md.

#include "backend.h"
#include "kit.h"
#include "producthost.h"
#include "services.h"
#include "shellbenchmark.h"
#include "shellscript.h"
#include "simservices.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>

#include <algorithm>
#include <memory>

using namespace Qt::StringLiterals;

Q_IMPORT_QML_PLUGIN(QOR_KitPlugin)
Q_IMPORT_QML_PLUGIN(LumenPlugin)
Q_IMPORT_QML_PLUGIN(QOR_ServicesPlugin)
Q_IMPORT_QML_PLUGIN(QOR_AppsPlugin)
Q_IMPORT_QML_PLUGIN(QOR_ShellPlugin)

int main(int argc, char *argv[])
{
    // The compositor's own window must not try to connect to itself.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM") && !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
        qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    // Before the audio engine or interface sounds touch Qt Multimedia.
    qor::audio::configureBackend();

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(u"QOR-MATRIX"_s);
    QGuiApplication::setApplicationName(u"Prism"_s);
    QGuiApplication::setApplicationVersion(QStringLiteral(QOR_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Prism, the QOR OS Wayland compositor."_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption services(u"services"_s, u"embedded (simulated, default) or external."_s, u"mode"_s,
                                      u"embedded"_s);
    const QCommandLineOption unlocked(u"unlocked"_s, u"Start on the desktop instead of the lock screen."_s);
    const QCommandLineOption socket(u"socket"_s, u"Wayland socket name for clients (default: first free wayland-N)."_s,
                                    u"name"_s);
    const QCommandLineOption size(u"size"_s, u"Output window size, e.g. 1600x1000."_s, u"WxH"_s);
    const QCommandLineOption spreadClients(u"spread-clients"_s,
                                           u"Open each new client on the output with the fewest windows."_s);
    const QCommandLineOption outputs(
        u"outputs"_s, u"Nested: drive this many outputs, side by side, each a window of --size."_s, u"n"_s, u"1"_s);
    const QCommandLineOption surface(u"surface"_s, u"Shell surface to show: beam, control-center, overview."_s,
                                     u"name"_s);
    const QCommandLineOption screenshot(u"screenshot"_s, u"Save a screenshot of the output to this path and exit."_s,
                                        u"file"_s);
    const QCommandLineOption delay(u"delay"_s, u"Milliseconds to wait before the screenshot."_s, u"ms"_s, u"5000"_s);
    const QCommandLineOption benchmark(
        u"benchmark"_s,
        u"Run the shell tour for N seconds, report per-output frame pacing and input latency as JSON, exit."_s,
        u"seconds"_s);
    const QCommandLineOption benchmarkOut(u"benchmark-out"_s, u"Write the benchmark JSON to this file."_s, u"file"_s);
    const QCommandLineOption script(u"script"_s, u"Run a JSON UI script against the shell, then exit."_s, u"file"_s);
    parser.addOptions({ services, unlocked, socket, size, outputs, spreadClients, surface, screenshot, delay, benchmark,
                        benchmarkOut, script });
    parser.process(app);

    qor::Kit::initialize();
    QQuickStyle::setStyle(u"Lumen"_s);

    // Automated runs install from the Store into a folder of their own, gone
    // with the run.
    std::unique_ptr<QTemporaryDir> automationStore;
    if ((parser.isSet(screenshot) || parser.isSet(benchmark) || parser.isSet(script))
        && qEnvironmentVariableIsEmpty("QOR_STORE_DIR")) {
        automationStore = std::make_unique<QTemporaryDir>();
        qputenv("QOR_STORE_DIR", automationStore->path().toUtf8());
    }

    std::unique_ptr<qor::sim::SimServices> simulator;
    QUrl servicesUrl = qor::Services::defaultUrl();
    if (parser.value(services) != u"external"_s) {
        simulator = std::make_unique<qor::sim::SimServices>();
        qor::sim::SimOptions simOptions;
        // Automated runs never make sound, keep no preferences, and get a
        // private endpoint so they never meet a session someone is using.
        const bool automated = parser.isSet(screenshot) || parser.isSet(benchmark) || parser.isSet(script);
        simOptions.silent = automated;
        if (automated && qEnvironmentVariableIsEmpty("QOR_GAMEPADS"))
            qputenv("QOR_GAMEPADS", "virtual"); // controllers on the desk never join in
        if (automated && qEnvironmentVariableIsEmpty("QOR_TOUCHPAD"))
            qputenv("QOR_TOUCHPAD", "off");
        if (automated) {
            simOptions.persistent = false;
            simOptions.url = QUrl(qor::Services::defaultUrl().scheme() + u":qor-services-"_s
                                  + QString::number(QCoreApplication::applicationPid()));
        }
        if (!simulator->start(simOptions))
            return 1;
        servicesUrl = simulator->url();
    }
    qor::Services::instance()->connectTo(servicesUrl);

    QQmlApplicationEngine engine;
    QVariantMap options;
    const int benchmarkSeconds = parser.value(benchmark).toInt();
    options.insert(u"unlocked"_s, parser.isSet(unlocked) || benchmarkSeconds > 0);
    options.insert(u"surface"_s, parser.value(surface));
    int width = 1600;
    int height = 1000;
    if (const QStringList parts = parser.value(size).split(u'x'); parts.size() == 2) {
        width = parts[0].toInt();
        height = parts[1].toInt();
    }
    options.insert(u"width"_s, width);
    options.insert(u"height"_s, height);
    // On bare metal (Qt's eglfs, linuxfb or vkkhrdisplay) every screen is an
    // output, fullscreen. Nested, --outputs windows of the host.
    const QString platform = QGuiApplication::platformName();
    const bool bareMetal = platform.startsWith(u"eglfs"_s) || platform == u"linuxfb"_s || platform == u"vkkhrdisplay"_s;
    QVariantList outputList;
    if (bareMetal) {
        const QList<QScreen *> screens = QGuiApplication::screens();
        for (qsizetype i = 0; i < screens.size(); ++i) {
            const QSize screenSize = screens.at(i)->size();
            outputList.append(QVariantMap { { u"width"_s, screenSize.width() },
                                            { u"height"_s, screenSize.height() },
                                            { u"screen"_s, int(i) },
                                            { u"fullScreen"_s, true } });
            qInfo("Output %lld: %s, %dx%d at %.1f Hz", qlonglong(i), qPrintable(screens.at(i)->name()),
                  screenSize.width(), screenSize.height(), screens.at(i)->refreshRate());
        }
    } else {
        const int count = std::clamp(parser.value(outputs).toInt(), 1, 4);
        for (int i = 0; i < count; ++i)
            outputList.append(QVariantMap { { u"width"_s, width }, { u"height"_s, height } });
    }
    options.insert(u"outputs"_s, outputList);
    options.insert(u"spreadClients"_s, parser.isSet(spreadClients));
    engine.setInitialProperties({ { u"options"_s, options }, { u"socketName"_s, parser.value(socket).toUtf8() } });
    engine.loadFromModule("QOR.Prism", "Compositor");
    if (engine.rootObjects().isEmpty())
        return 2;

    const QByteArray socketName = engine.rootObjects().constFirst()->property("socketName").toByteArray();
    qInfo("Prism is running. Start clients with WAYLAND_DISPLAY=%s", socketName.constData());
    // The products Prism hosts open their windows here.
    if (auto *products = engine.singletonInstance<qor::ProductHost *>("QOR.Shell", "ProductHost"))
        products->setDisplay(QString::fromUtf8(socketName));

    // One QQuickWindow per WaylandOutput, the main display's (the Desktop's)
    // first.
    QList<QQuickWindow *> outputWindows;
    for (QWindow *window : QGuiApplication::topLevelWindows())
        if (auto *quick = qobject_cast<QQuickWindow *>(window); quick && quick->isVisible())
            outputWindows.append(quick);
    std::stable_sort(outputWindows.begin(), outputWindows.end(), [](QQuickWindow *a, QQuickWindow *b) {
        const auto main = [](QQuickWindow *w) { return w->findChild<QObject *>(u"desktop"_s) != nullptr; };
        return main(a) && !main(b);
    });

    if (benchmarkSeconds > 0)
        qor::runShellBenchmark(outputWindows, { u"prism"_s, benchmarkSeconds, parser.value(benchmarkOut) });

    if (parser.isSet(script))
        qor::runShellScript(outputWindows.value(0), parser.value(script));

    if (parser.isSet(screenshot)) {
        QTimer::singleShot(parser.value(delay).toInt(), &app, [path = parser.value(screenshot), outputWindows] {
            const bool saved = !outputWindows.isEmpty() && outputWindows.first()->grabWindow().save(path);
            if (saved)
                qInfo("Saved %s", qPrintable(path));
            else
                qWarning("Could not save a screenshot to %s", qPrintable(path));
            QCoreApplication::exit(saved ? 0 : 3);
        });
    }
    return app.exec();
}
