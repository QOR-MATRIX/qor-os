// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// tst_producthost_product: stands in for a product in tst_producthost. Started
// by ProductHost, it says hello as QOR_TEST_PRODUCT_ID with the token from its
// stdin (with QOR_TEST_PRODUCT_MODE=wrong-token, with another), keeps the long
// poll QOR_TEST_PRODUCT_METHOD waiting, and writes what it sees to
// QOR_TEST_PRODUCT_LOG, one JSON object a line: its arguments and the
// environment Prism gave it, the welcome or the refusal, each command, and
// the channel lost. It quits on `quit`, or when the channel is lost. It has no
// window.

#include "productside.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    QFile log(environment.value(QStringLiteral("QOR_TEST_PRODUCT_LOG")));
    if (!log.open(QIODevice::WriteOnly | QIODevice::Append))
        return 2;
    const auto note = [&log](const QString &key, const QJsonValue &value) {
        log.write(QJsonDocument(QJsonObject { { key, value } }).toJson(QJsonDocument::Compact) + '\n');
        log.flush();
    };

    QJsonObject given;
    for (const char *name : { "WAYLAND_DISPLAY", "QT_QPA_PLATFORM", "QOR_REDUCED_MOTION", "XDG_ACTIVATION_TOKEN" }) {
        const QString key = QString::fromLatin1(name);
        given.insert(key, environment.contains(key) ? QJsonValue(environment.value(key)) : QJsonValue());
    }
    const QStringList arguments = app.arguments().mid(1);
    note(QStringLiteral("started"),
         QJsonObject { { QStringLiteral("pid"), QCoreApplication::applicationPid() },
                       { QStringLiteral("arguments"), QJsonArray::fromStringList(arguments) },
                       { QStringLiteral("environment"), given } });

    QByteArray token = ProductSide::readToken();
    note(QStringLiteral("tokenLength"), int(token.size()));
    if (environment.value(QStringLiteral("QOR_TEST_PRODUCT_MODE")) == QStringLiteral("wrong-token"))
        token = QByteArray(token.size(), '0');

    const QString method = environment.value(QStringLiteral("QOR_TEST_PRODUCT_METHOD"));
    ProductSide side;
    side.welcomed = [&](const QJsonObject &frame) {
        note(QStringLiteral("welcome"), frame);
        side.ask(method);
    };
    side.refused = [&](const QJsonObject &frame) { note(QStringLiteral("refused"), frame); };
    side.answered = [&](const QJsonValue &command) {
        if (command.isObject()) {
            note(QStringLiteral("command"), command);
            if (command.toObject().value(QStringLiteral("type")).toString() == QStringLiteral("quit")) {
                QCoreApplication::quit();
                return;
            }
        }
        side.ask(method);
    };
    side.lost = [&] {
        note(QStringLiteral("lost"), true);
        QCoreApplication::quit();
    };
    side.connectTo(ProductSide::channelIn(arguments),
                   { { QStringLiteral("type"), QStringLiteral("hello") },
                     { QStringLiteral("protocol"), 1 },
                     { QStringLiteral("min_protocol"), 1 },
                     { QStringLiteral("product"), environment.value(QStringLiteral("QOR_TEST_PRODUCT_ID")) },
                     { QStringLiteral("version"), QStringLiteral("0.0.0") },
                     { QStringLiteral("token"), QString::fromLatin1(token) } });
    return QCoreApplication::exec();
}
