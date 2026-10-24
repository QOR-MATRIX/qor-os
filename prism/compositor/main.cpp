// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// prism — the QTOS Wayland compositor (Linux).
//
// Hosts the QTOS.Shell desktop inside a Qt Wayland Compositor scene. Run it
// nested for development:
//
//   prism                       # inside an existing Wayland/X11 session or WSLg
//   WAYLAND_DISPLAY=wayland-1 kwrite   # launch a client into Prism
//
// Status: experimental, see docs/adr/0003-compositor-strategy.md.

#include "kit.h"
#include "services.h"
#include "simservices.h"

#include <QtCore/QCommandLineParser>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuickControls2/QQuickStyle>

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
    parser.addOptions({ services, unlocked });
    parser.process(app);

    qtos::Kit::initialize();
    QQuickStyle::setStyle(u"Lumen"_s);

    std::unique_ptr<qtos::sim::SimServices> simulator;
    QUrl servicesUrl = qtos::Services::defaultUrl();
    if (parser.value(services) != u"external"_s) {
        simulator = std::make_unique<qtos::sim::SimServices>();
        if (!simulator->start({}))
            return 1;
        servicesUrl = simulator->url();
    }
    qtos::Services::instance()->connectTo(servicesUrl);

    QQmlApplicationEngine engine;
    QVariantMap options;
    options.insert(u"unlocked"_s, parser.isSet(unlocked));
    engine.setInitialProperties({ { u"options"_s, options } });
    engine.loadFromModule("QTOS.Prism", "Compositor");
    if (engine.rootObjects().isEmpty())
        return 2;
    return app.exec();
}
