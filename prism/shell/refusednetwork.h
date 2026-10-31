// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Nothing the shell draws comes from the network. Qt's image loading would fetch
// any picture named to it, and a shell string, a window's title or a
// notification, can name one if a Text ever reads it as rich text: so the shell's
// engine has no network to fetch it with. The Store's downloads have a network
// manager of their own, checked against signatures, and are not affected.
#pragma once

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtQml/QQmlNetworkAccessManagerFactory>

namespace qor {

class RefusedNetwork : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoing) override
    {
        if (request.url().scheme() == QLatin1String("qrc") || request.url().scheme() == QLatin1String("data"))
            return QNetworkAccessManager::createRequest(op, request, outgoing);
        return QNetworkAccessManager::createRequest(op, QNetworkRequest(QUrl(QStringLiteral("qrc:/qor-refused"))),
                                                    nullptr);
    }
};

class RefusedNetworkFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override { return new RefusedNetwork(parent); }
};

} // namespace qor
