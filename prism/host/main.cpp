// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// qtos-shell — runs the Prism shell in a window on any desktop OS.
//
// The shell is the same QML that Prism (the Wayland compositor) hosts on a
// device; here it manages simulated app windows, and talks to the simulated
// QTOS services over Qt Remote Objects. See docs/BUILDING.md.

#include "framestats.h"
#include "gpupreference.h"
#include "kit.h"
#include "services.h"
#include "simservices.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <QtQuickControls2/QQuickStyle>

#include <cstdio>
#include <memory>

using namespace Qt::StringLiterals;

Q_IMPORT_QML_PLUGIN(QTOS_KitPlugin)
Q_IMPORT_QML_PLUGIN(LumenPlugin)
Q_IMPORT_QML_PLUGIN(QTOS_ServicesPlugin)
Q_IMPORT_QML_PLUGIN(QTOS_AppsPlugin)
Q_IMPORT_QML_PLUGIN(QTOS_ShellPlugin)

Q_LOGGING_CATEGORY(lcHost, "qtos.shell.host")

namespace {

struct HostOptions
{
    QVariantMap qml; // forwarded to Main.qml as `options`
    QString servicesMode = u"embedded"_s;
    QUrl servicesUrl;
    QString scenario;
    QString screenshotPath;
    int screenshotDelayMs = 3000;
    int benchmarkSeconds = 0;
    QString gpu = u"high-performance"_s;
    QString benchmarkOut;
    bool ephemeral = false;
};

HostOptions parseOptions(const QGuiApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(u"Prism shell for QTOS, running in simulation."_s);
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption services(u"services"_s, u"Service backend: embedded (default) or external."_s, u"mode"_s,
                                      u"embedded"_s);
    const QCommandLineOption servicesUrl(u"services-url"_s, u"Qt Remote Objects endpoint of the services host."_s,
                                         u"url"_s);
    const QCommandLineOption scenario(u"scenario"_s, u"Simulation scenario JSON file."_s, u"file"_s);
    const QCommandLineOption ephemeral(u"ephemeral"_s, u"Do not persist preferences (implied by --screenshot)."_s);
    const QCommandLineOption unlocked(u"unlocked"_s, u"Start on the desktop instead of the lock screen."_s);
    const QCommandLineOption open(u"open"_s, u"Comma-separated apps to open at start (e.g. files,music)."_s, u"apps"_s);
    const QCommandLineOption surface(u"surface"_s, u"Shell surface to show: beam, control-center, overview."_s,
                                     u"name"_s);
    const QCommandLineOption query(u"query"_s, u"Initial Beam query."_s, u"text"_s);
    const QCommandLineOption theme(u"theme"_s, u"Force appearance: light or dark."_s, u"theme"_s);
    const QCommandLineOption accent(u"accent"_s, u"Force accent hue in degrees."_s, u"hue"_s);
    const QCommandLineOption size(u"size"_s, u"Window size, e.g. 1600x1000."_s, u"WxH"_s);
    const QCommandLineOption fullscreen(u"fullscreen"_s, u"Start full screen."_s);
    const QCommandLineOption screenshot(u"screenshot"_s, u"Save a screenshot to this path and exit."_s, u"file"_s);
    const QCommandLineOption delay(u"delay"_s, u"Milliseconds to wait before the screenshot."_s, u"ms"_s, u"3000"_s);
    const QCommandLineOption benchmark(
        u"benchmark"_s, u"Run a scripted animation tour for N seconds, report frame pacing as JSON, exit."_s,
        u"seconds"_s);
    const QCommandLineOption benchmarkOut(u"benchmark-out"_s, u"Write the benchmark JSON to this file."_s, u"file"_s);
    const QCommandLineOption gpu(u"gpu"_s, u"GPU preference: high-performance (default), low-power or default."_s,
                                 u"preference"_s, u"high-performance"_s);
    parser.addOptions({ services, servicesUrl, scenario, ephemeral, unlocked, open, surface, query, theme, accent, size,
                        fullscreen, screenshot, delay, benchmark, benchmarkOut, gpu });
    parser.process(app);

    HostOptions options;
    options.servicesMode = parser.value(services);
    options.servicesUrl = parser.isSet(servicesUrl) ? QUrl(parser.value(servicesUrl)) : QUrl();
    options.scenario = parser.value(scenario);
    options.screenshotPath = parser.value(screenshot);
    options.screenshotDelayMs = parser.value(delay).toInt();
    options.benchmarkSeconds = parser.value(benchmark).toInt();
    options.benchmarkOut = parser.value(benchmarkOut);
    options.gpu = parser.value(gpu);
    options.ephemeral = parser.isSet(ephemeral) || parser.isSet(screenshot) || options.benchmarkSeconds > 0;

    QVariantMap &qml = options.qml;
    qml.insert(u"unlocked"_s, parser.isSet(unlocked) || options.benchmarkSeconds > 0);
    qml.insert(u"apps"_s, parser.value(open).split(u',', Qt::SkipEmptyParts));
    qml.insert(u"surface"_s, parser.value(surface));
    qml.insert(u"query"_s, parser.value(query));
    qml.insert(u"theme"_s, parser.value(theme));
    qml.insert(u"accent"_s, parser.isSet(accent) ? QVariant(parser.value(accent).toDouble()) : QVariant());
    qml.insert(u"fullscreen"_s, parser.isSet(fullscreen));
    qml.insert(u"screenshot"_s, parser.isSet(screenshot));
    qml.insert(u"qtVersion"_s, QString::fromLatin1(qVersion()));
    qml.insert(u"version"_s, QStringLiteral(QTOS_VERSION_STRING));
    if (parser.isSet(size)) {
        const QStringList parts = parser.value(size).split(u'x');
        if (parts.size() == 2) {
            qml.insert(u"width"_s, parts[0].toInt());
            qml.insert(u"height"_s, parts[1].toInt());
        }
    }
    return options;
}

// Drives a deterministic tour of shell surfaces and reports frame pacing.
void runBenchmark(QQuickWindow *window, const HostOptions &options)
{
    auto *stats = new qtos::FrameStats(window);
    stats->setWindow(window);
    QObject *desktop = window->findChild<QObject *>(u"desktop"_s);

    struct Step
    {
        const char *method;
        QVariant argument;
    };
    static const QList<Step> script = {
        { "setSurface", u"overview"_s },
        { "setSurface", QString() },
        { "setSurface", u"beam"_s },
        { "setSurface", QString() },
        { "setSurface", u"control-center"_s },
        { "setSurface", QString() },
        { "switchSpace", 1 },
        { "switchSpace", 0 },
    };

    auto *driver = new QTimer(window);
    driver->setInterval(750);
    QObject::connect(driver, &QTimer::timeout, window, [desktop, driver] {
        const int index = driver->property("step").toInt();
        const Step &step = script.at(index % script.size());
        if (step.argument.typeId() == QMetaType::Int)
            QMetaObject::invokeMethod(desktop, step.method, Q_ARG(int, step.argument.toInt()));
        else
            QMetaObject::invokeMethod(desktop, step.method, Q_ARG(QString, step.argument.toString()));
        driver->setProperty("step", index + 1);
    });

    constexpr int warmupMs = 2500;
    QTimer::singleShot(warmupMs, window, [stats, driver] {
        stats->reset();
        driver->start();
    });
    QTimer::singleShot(warmupMs + options.benchmarkSeconds * 1000, window, [window, stats, options] {
        const char *api = "unknown";
        switch (window->rendererInterface()->graphicsApi()) {
        case QSGRendererInterface::Direct3D11: api = "Direct3D 11"; break;
        case QSGRendererInterface::Direct3D12: api = "Direct3D 12"; break;
        case QSGRendererInterface::Vulkan: api = "Vulkan"; break;
        case QSGRendererInterface::Metal: api = "Metal"; break;
        case QSGRendererInterface::OpenGL: api = "OpenGL"; break;
        default: break;
        }
        const double avg = stats->average();
        const QJsonObject result {
            { u"frames"_s, stats->framesRecorded() },
            { u"averageMs"_s, avg },
            { u"averageFps"_s, avg > 0 ? 1000.0 / avg : 0.0 },
            { u"p50Ms"_s, stats->percentile(50) },
            { u"p95Ms"_s, stats->percentile(95) },
            { u"p99Ms"_s, stats->percentile(99) },
            { u"maxMs"_s, stats->maximum() },
            { u"graphicsApi"_s, QString::fromLatin1(api) },
            { u"size"_s, QString(QString::number(window->width()) + u'x' + QString::number(window->height())) },
            { u"devicePixelRatio"_s, window->devicePixelRatio() },
            { u"seconds"_s, options.benchmarkSeconds },
        };
        const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
        if (!options.benchmarkOut.isEmpty()) {
            QFile file(options.benchmarkOut);
            if (file.open(QIODevice::WriteOnly))
                file.write(json);
        }
        std::fputs(json.constData(), stdout);
        std::fflush(stdout);
        QCoreApplication::exit(0);
    });
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(u"QOR-MATRIX"_s);
    QGuiApplication::setOrganizationDomain(u"qor-matrix.dev"_s);
    QGuiApplication::setApplicationName(u"QTOS Shell"_s);
    QGuiApplication::setApplicationDisplayName(u"QTOS"_s);
    QGuiApplication::setApplicationVersion(QStringLiteral(QTOS_VERSION_STRING));

    const HostOptions options = parseOptions(app);
    qCInfo(lcHost) << "GPU:" << qtos::host::applyGpuPreference(options.gpu);

    qtos::Kit::initialize();
    QQuickStyle::setStyle(u"Lumen"_s);

    // ── Services ────────────────────────────────────────────────────────────
    std::unique_ptr<qtos::sim::SimServices> simulator;
    QUrl servicesUrl = options.servicesUrl.isEmpty() ? qtos::Services::defaultUrl() : options.servicesUrl;
    if (options.servicesMode != u"external"_s) {
        simulator = std::make_unique<qtos::sim::SimServices>();
        qtos::sim::SimOptions simOptions;
        simOptions.url = options.servicesUrl;
        simOptions.scenarioPath = options.scenario;
        simOptions.persistent = !options.ephemeral;
        if (!simulator->start(simOptions)) {
            qCCritical(lcHost) << "Could not start the simulated services.";
            return 1;
        }
        servicesUrl = simulator->url();
    }
    if (!qtos::Services::instance()->connectTo(servicesUrl))
        qCWarning(lcHost) << "Continuing with partially available services.";

    // ── Shell ───────────────────────────────────────────────────────────────
    QQmlApplicationEngine engine;
    engine.setInitialProperties({ { u"options"_s, options.qml } });
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(2); },
        Qt::QueuedConnection);
    engine.loadFromModule("QTOS.Shell", "Main");

    if (engine.rootObjects().isEmpty())
        return 2;

    if (options.benchmarkSeconds > 0)
        runBenchmark(qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst()), options);

    if (!options.screenshotPath.isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        QTimer::singleShot(options.screenshotDelayMs, &app, [window, path = options.screenshotPath] {
            const QImage image = window ? window->grabWindow() : QImage();
            // Quality applies to lossy formats (.webp, .jpg); PNG stays lossless.
            const bool saved = !image.isNull() && image.save(path, nullptr, 92);
            if (saved)
                qCInfo(lcHost) << "Saved screenshot" << QDir::toNativeSeparators(path) << image.size();
            else
                qCCritical(lcHost) << "Failed to save screenshot" << path;
            QCoreApplication::exit(saved ? 0 : 3);
        });
    }

    return app.exec();
}
