// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// qtos-simd — hosts the simulated QTOS services in their own process, the way
// the qtos-*d daemons run on a device. Start it, then run the shell with
// `qtos-shell --services external` and drive it with `qtosctl`.

#include "simservices.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(u"QOR-MATRIX"_s);
    QCoreApplication::setApplicationName(u"QTOS Simulator"_s);
    QCoreApplication::setApplicationVersion(QStringLiteral(QTOS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Hosts the simulated QTOS system services over Qt Remote Objects."_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption urlOption(u"url"_s, u"Endpoint to host the services on."_s, u"url"_s);
    const QCommandLineOption scenarioOption(u"scenario"_s, u"Scenario JSON file to load."_s, u"file"_s);
    const QCommandLineOption ephemeralOption(u"ephemeral"_s, u"Do not persist user preferences."_s);
    parser.addOptions({ urlOption, scenarioOption, ephemeralOption });
    parser.process(app);

    qtos::sim::SimOptions options;
    options.url = parser.isSet(urlOption) ? QUrl(parser.value(urlOption)) : QUrl();
    options.scenarioPath = parser.value(scenarioOption);
    options.persistent = !parser.isSet(ephemeralOption);

    qtos::sim::SimServices services;
    if (!services.start(options))
        return 1;
    return app.exec();
}
