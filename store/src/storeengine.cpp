// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "storeengine.h"

#include "package.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSaveFile>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcStore, "qor.store")

namespace qor::store {

namespace {

constexpr qint64 kMaximumCatalogSize = 4 * 1024 * 1024;
constexpr int kTransferTimeoutMs = 30000;

} // namespace

StoreEngine::StoreEngine(const QString &dataDir, const QByteArray &storeKey, QObject *parent)
    : QObject(parent), m_dataDir(dataDir), m_storeKey(storeKey), m_network(new QNetworkAccessManager(this))
{
    m_network->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_network->setTransferTimeout(kTransferTimeoutMs);
    QDir().mkpath(m_dataDir + u"/items"_s);
    loadInstalled();
}

StoreEngine::~StoreEngine()
{
    for (const QPointer<QNetworkReply> &reply : std::as_const(m_jobs))
        if (reply)
            reply->abort();
}

void StoreEngine::setCatalogUrl(const QUrl &url) { m_catalogUrl = url; }

QString StoreEngine::installDirectory(const QString &id) const { return m_dataDir + u"/items/"_s + id; }

void StoreEngine::setState(State state, const QString &error)
{
    m_error = error;
    if (m_state == state && error.isEmpty())
        return;
    m_state = state;
    Q_EMIT stateChanged();
}

// Local catalogues (qrc:, file:) are read directly; anything else goes
// through the network. Either way the result arrives asynchronously.
void StoreEngine::fetch(const QString &job, const QUrl &url, qint64 maximumSize, const Done &done)
{
    if (url.scheme() == u"qrc"_s || url.isLocalFile()) {
        const QString path = url.scheme() == u"qrc"_s ? u":"_s + url.path() : url.toLocalFile();
        QTimer::singleShot(0, this, [path, maximumSize, done] {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                return done(false, {}, u"%1 cannot be read."_s.arg(path));
            if (file.size() > maximumSize)
                return done(false, {}, u"%1 is larger than expected."_s.arg(path));
            done(true, file.readAll(), {});
        });
        return;
    }
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    QNetworkReply *reply = m_network->get(request);
    if (!job.isEmpty()) {
        m_jobs.insert(job, reply);
        connect(reply, &QNetworkReply::downloadProgress, this, [this, job](qint64 received, qint64 total) {
            m_progress.insert(job, total > 0 ? 0.9 * double(received) / double(total) : 0);
            Q_EMIT progressChanged(job);
        });
    }
    connect(reply, &QNetworkReply::readyRead, reply, [reply, maximumSize] {
        if (reply->bytesAvailable() > maximumSize)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [reply, done] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return done(false, {}, reply->errorString());
        done(true, reply->readAll(), {});
    });
}

void StoreEngine::refresh()
{
    if (m_catalogUrl.isEmpty()) {
        setState(State::Failed, u"No store catalogue is configured."_s);
        return;
    }
    const int generation = ++m_generation;
    setState(State::Loading);
    const QUrl signatureUrl(m_catalogUrl.toString() + u".sig"_s);
    fetch({}, m_catalogUrl, kMaximumCatalogSize,
          [this, generation, signatureUrl](bool ok, const QByteArray &json, const QString &error) {
              if (generation != m_generation)
                  return;
              if (!ok)
                  return setState(State::Failed, error);
              fetch({}, signatureUrl, 1024,
                    [this, generation, json](bool fetched, const QByteArray &signature, const QString &fetchError) {
                        if (generation != m_generation)
                            return;
                        if (!fetched)
                            return setState(State::Failed, fetchError);
                        CatalogResult result = parseCatalog(json, signature, m_storeKey);
                        if (!result.ok) {
                            qCWarning(lcStore) << "Catalogue refused:" << result.error;
                            return setState(State::Failed, result.error);
                        }
                        m_catalog = std::move(result.catalog);
                        qCInfo(lcStore) << "Catalogue" << m_catalog.name << "with" << m_catalog.items.size() << "items";
                        Q_EMIT catalogChanged();
                        setState(State::Ready);
                    });
          });
}

bool StoreEngine::install(const QString &id)
{
    const CatalogItem *item = m_catalog.item(id);
    if (!item || isBusy(id))
        return false;
    m_jobs.insert(id, nullptr);
    m_progress.insert(id, 0);
    Q_EMIT progressChanged(id);
    const QUrl url = m_catalogUrl.resolved(QUrl(item->packageFile));
    fetch(id, url, item->packageSize + 1024, [this, id](bool ok, const QByteArray &data, const QString &error) {
        if (!ok)
            return fail(id, error);
        complete(id, data);
    });
    return true;
}

void StoreEngine::complete(const QString &id, const QByteArray &data)
{
    const CatalogItem *item = m_catalog.item(id);
    if (!item)
        return fail(id, u"The item is no longer in the catalogue."_s);
    if (QCryptographicHash::hash(data, QCryptographicHash::Sha256) != item->packageSha256)
        return fail(id, u"The download does not match the catalogue."_s);
    const Developer developer = m_catalog.developers.value(item->developer);
    const PackageResult package = openPackage(data, developer.key);
    if (!package.ok)
        return fail(id, package.error);
    const PackageManifest &manifest = package.manifest;
    if (manifest.id != item->id || manifest.version != item->version || manifest.kind != item->kind
        || manifest.developer != item->developer)
        return fail(id, u"The package is not the one the catalogue lists."_s);

    // Write everything beside the old version, then swap.
    const QString target = installDirectory(id);
    const QString staging = m_dataDir + u"/items/.staging-"_s + QUuid::createUuid().toString(QUuid::Id128);
    for (auto it = package.contents.cbegin(); it != package.contents.cend(); ++it) {
        const QString path = staging + u'/' + it.key();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(it.value()) != it.value().size() || !file.commit()) {
            QDir(staging).removeRecursively();
            return fail(id, u"%1 could not be written."_s.arg(it.key()));
        }
    }
    const QString previous = target + u".old-"_s + QUuid::createUuid().toString(QUuid::Id128);
    if (QFileInfo::exists(target) && !QDir().rename(target, previous)) {
        QDir(staging).removeRecursively();
        return fail(id, u"The installed version is in use."_s);
    }
    if (!QDir().rename(staging, target)) {
        QDir().rename(previous, target);
        QDir(staging).removeRecursively();
        return fail(id, u"The package could not be installed."_s);
    }
    QDir(previous).removeRecursively();

    m_installed.insert(id, { id, item->kind, item->name, item->version, item->developer,
                             QDateTime::currentDateTimeUtc().toString(Qt::ISODate) });
    saveInstalled();
    m_jobs.remove(id);
    m_progress.remove(id);
    qCInfo(lcStore) << "Installed" << id << item->version;
    Q_EMIT installedChanged();
    Q_EMIT progressChanged(id);
    Q_EMIT finished(id, true, {});
}

void StoreEngine::fail(const QString &id, const QString &error)
{
    qCWarning(lcStore) << "Install of" << id << "failed:" << error;
    m_jobs.remove(id);
    m_progress.remove(id);
    Q_EMIT progressChanged(id);
    Q_EMIT finished(id, false, error);
}

bool StoreEngine::remove(const QString &id)
{
    if (!m_installed.contains(id) || isBusy(id))
        return false;
    QDir(installDirectory(id)).removeRecursively();
    m_installed.remove(id);
    saveInstalled();
    qCInfo(lcStore) << "Removed" << id;
    Q_EMIT installedChanged();
    return true;
}

void StoreEngine::loadInstalled()
{
    QFile file(m_dataDir + u"/installed.json"_s);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject items = QJsonDocument::fromJson(file.readAll()).object().value(u"items"_s).toObject();
    for (auto it = items.begin(); it != items.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        Installed installed { it.key(),
                              entry.value(u"kind"_s).toString(),
                              entry.value(u"name"_s).toString(),
                              entry.value(u"version"_s).toString(),
                              entry.value(u"developer"_s).toString(),
                              entry.value(u"installedAt"_s).toString() };
        // Only what is actually on disk.
        if (isValidItemId(installed.id) && QFileInfo(installDirectory(installed.id)).isDir())
            m_installed.insert(installed.id, installed);
    }
}

bool StoreEngine::saveInstalled() const
{
    QJsonObject items;
    for (const Installed &installed : m_installed)
        items.insert(installed.id, QJsonObject { { u"kind"_s, installed.kind },
                                                 { u"name"_s, installed.name },
                                                 { u"version"_s, installed.version },
                                                 { u"developer"_s, installed.developer },
                                                 { u"installedAt"_s, installed.installedAt } });
    QSaveFile file(m_dataDir + u"/installed.json"_s);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(
        QJsonDocument(QJsonObject { { u"format"_s, 1 }, { u"items"_s, items } }).toJson(QJsonDocument::Indented));
    return file.commit();
}

} // namespace qor::store
