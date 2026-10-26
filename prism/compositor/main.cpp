// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// prism — the QTOS Wayland compositor (Linux).
//
// Hosts the QTOS.Shell desktop inside a Qt Wayland Compositor scene. Run it
// nested for development:
//
//   prism --unlocked            # inside an existing Wayland/X11 session or WSLg
//   WAYLAND_DISPLAY=wayland-1 weston-terminal   # launch a client into Prism
//
// Prism prints the socket name it listens on (the first free wayland-N, or
// --socket).
//
// Status: experimental, see docs/adr/0003-compositor-strategy.md.

#include "backend.h"
#include "kit.h"
#include "services.h"
#include "shellbenchmark.h"
#include "shellscript.h"
#include "simservices.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>

#include <algorithm>
#include <memory>

using namespace Qt::StringLiterals;

Q_IMPORT_QML_PLUGIN(QTOS_KitPlugin)
Q_IMPORT_QML_PLUGIN(LumenPlugin)
Q_IMPORT_QML_PLUGIN(QTOS_ServicesPlugin)
Q_IMPORT_QML_PLUGIN(QTOS_AppsPlugin)
Q_IMPORT_QML_PLUGIN(QTOS_ShellPlugin)

int main(int argc, char *argv[])
{
    // The compositor's own window must not try to connect to itself.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM") && !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
        qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    // Before the audio engine or interface sounds touch Qt Multimedia.
    qtos::audio::configureBackend();

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(u"QOR-MATRIX"_s);
    QGuiApplication::setApplicationName(u"Prism"_s);
    QGuiApplication::setApplicationVersion(QStringLiteral(QTOS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Prism, the QTOS Wayland compositor."_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption services(u"services"_s, u"embedded (simulated, default) or external."_s, u"mode"_s,
                                      u"embedded"_s);
    const QCommandLineOption unlocked(u"unlocked"_s, u"Start on the desktop instead of the lock screen."_s);
    const QCommandLineOption socket(u"socket"_s, u"Wayland socket name for clients (default: first free wayland-N)."_s,
                                    u"name"_s);
    const QCommandLineOption size(u"size"_s, u"Output window size, e.g. 1600x1000."_s, u"WxH"_s);
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
    parser.addOptions(
        { services, unlocked, socket, size, surface, screenshot, delay, benchmark, benchmarkOut, script });
    parser.process(app);

    qtos::Kit::initialize();
    QQuickStyle::setStyle(u"Lumen"_s);

    std::unique_ptr<qtos::sim::SimServices> simulator;
    QUrl servicesUrl = qtos::Services::defaultUrl();
    if (parser.value(services) != u"external"_s) {
        simulator = std::make_unique<qtos::sim::SimServices>();
        qtos::sim::SimOptions simOptions;
        // Automated runs never make sound, keep no preferences, and get a
        // private endpoint so they never meet a session someone is using.
        const bool automated = parser.isSet(screenshot) || parser.isSet(benchmark) || parser.isSet(script);
        simOptions.silent = automated;
        if (automated && qEnvironmentVariableIsEmpty("QTOS_GAMEPADS"))
            qputenv("QTOS_GAMEPADS", "virtual"); // controllers on the desk never join in
        if (automated && qEnvironmentVariableIsEmpty("QTOS_TOUCHPAD"))
            qputenv("QTOS_TOUCHPAD", "off");
        if (automated) {
            simOptions.persistent = false;
            simOptions.url = QUrl(qtos::Services::defaultUrl().scheme() + u":qtos-services-"_s
                                  + QString::number(QCoreApplication::applicationPid()));
        }
        if (!simulator->start(simOptions))
            return 1;
        servicesUrl = simulator->url();
    }
    qtos::Services::instance()->connectTo(servicesUrl);

    QQmlApplicationEngine engine;
    QVariantMap options;
    const int benchmarkSeconds = parser.value(benchmark).toInt();
    options.insert(u"unlocked"_s, parser.isSet(unlocked) || benchmarkSeconds > 0);
    options.insert(u"surface"_s, parser.value(surface));
    if (const QStringList parts = parser.value(size).split(u'x'); parts.size() == 2) {
        options.insert(u"width"_s, parts[0].toInt());
        options.insert(u"height"_s, parts[1].toInt());
    }
    engine.setInitialProperties({ { u"options"_s, options }, { u"socketName"_s, parser.value(socket).toUtf8() } });
    engine.loadFromModule("QTOS.Prism", "Compositor");
    if (engine.rootObjects().isEmpty())
        return 2;

    const QByteArray socketName = engine.rootObjects().constFirst()->property("socketName").toByteArray();
    qInfo("Prism is running. Start clients with WAYLAND_DISPLAY=%s", socketName.constData());

    if (benchmarkSeconds > 0) {
        // One QQuickWindow per WaylandOutput: measure them all.
        QList<QQuickWindow *> outputs;
        for (QWindow *window : QGuiApplication::topLevelWindows())
            if (auto *quick = qobject_cast<QQuickWindow *>(window); quick && quick->isVisible())
                outputs.append(quick);
        qtos::runShellBenchmark(outputs, { u"prism"_s, benchmarkSeconds, parser.value(benchmarkOut) });
    }

    if (parser.isSet(script)) {
        QQuickWindow *output = nullptr;
        for (QWindow *window : QGuiApplication::topLevelWindows())
            if (auto *quick = qobject_cast<QQuickWindow *>(window); quick && quick->isVisible() && !output)
                output = quick;
        qtos::runShellScript(output, parser.value(script));
    }

    if (parser.isSet(screenshot)) {
        QTimer::singleShot(parser.value(delay).toInt(), &app, [path = parser.value(screenshot)] {
            const QWindowList windows = QGuiApplication::topLevelWindows();
            auto it = std::find_if(windows.cbegin(), windows.cend(),
                                   [](QWindow *w) { return w->isVisible() && qobject_cast<QQuickWindow *>(w); });
            const bool saved = it != windows.cend() && static_cast<QQuickWindow *>(*it)->grabWindow().save(path);
            if (saved)
                qInfo("Saved %s", qPrintable(path));
            else
                qWarning("Could not save a screenshot to %s", qPrintable(path));
            QCoreApplication::exit(saved ? 0 : 3);
        });
    }
    return app.exec();
}
