// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// What keeps QOR Media to this computer. A hostile file can name a picture anywhere: Markdown's ![](https://...) loads as
// the preview opens, and a track's or a PDF's title read as rich text would too. On Windows a file://server/share URL
// also makes the system log in to that server, which can give away the person's password hash.

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlNetworkAccessManagerFactory>

namespace qormedia {

class LocalOnlyNetwork : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoing) override
    {
        const QString scheme = request.url().scheme();
        if (scheme == QLatin1String("qrc") || scheme == QLatin1String("data"))
            return QNetworkAccessManager::createRequest(op, request, outgoing);
        return QNetworkAccessManager::createRequest(op, QNetworkRequest(QUrl(QStringLiteral("qrc:/qor-media-refused"))), nullptr);
    }
};

class LocalOnlyNetworkFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override { return new LocalOnlyNetwork(parent); }
};

class NoOtherComputers : public QQmlAbstractUrlInterceptor
{
public:
    QUrl intercept(const QUrl &url, DataType) override
    {
        if (url.scheme() == QLatin1String("file") && !url.host().isEmpty())
            return QUrl(QStringLiteral("qrc:/qor-media-refused"));
        return url;
    }
};

}  // namespace qormedia
