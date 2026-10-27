// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// qor-simd — hosts the simulated QOR OS services in their own process, the way
// the qor-*d daemons run on a device. Start it, then run the shell with
// `qor-shell --services external` and drive it with `qorctl`.

#include "carryover.h"
#include "simservices.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(u"QOR-MATRIX"_s);
    QCoreApplication::setApplicationName(u"QOR OS Simulator"_s);
    QCoreApplication::setApplicationVersion(QStringLiteral(QOR_VERSION_STRING));
    qor::sim::carryOverFrom(u"QTOS Simulator"_s);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Hosts the simulated QOR OS system services over Qt Remote Objects."_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption urlOption(u"url"_s, u"Endpoint to host the services on."_s, u"url"_s);
    const QCommandLineOption scenarioOption(u"scenario"_s, u"Scenario JSON file to load."_s, u"file"_s);
    const QCommandLineOption ephemeralOption(u"ephemeral"_s, u"Do not persist user preferences."_s);
    const QCommandLineOption silentOption(u"silent"_s, u"Render audio without an output device."_s);
    parser.addOptions({ urlOption, scenarioOption, ephemeralOption, silentOption });
    parser.process(app);

    qor::sim::SimOptions options;
    options.url = parser.isSet(urlOption) ? QUrl(parser.value(urlOption)) : QUrl();
    options.scenarioPath = parser.value(scenarioOption);
    options.persistent = !parser.isSet(ephemeralOption);
    options.silent = parser.isSet(silentOption);

    qor::sim::SimServices services;
    if (!services.start(options))
        return 1;
    return app.exec();
}
