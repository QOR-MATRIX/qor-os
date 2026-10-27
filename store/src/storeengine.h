// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "catalog.h"

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QUrl>

#include <functional>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
QT_END_NAMESPACE

namespace qor::store {

// The QOR OS Store client for one user: fetches the signed catalogue, and
// downloads, verifies, installs, updates and removes packages under
// `dataDir` (items/<id>/ and installed.json).
//
// Nothing is installed unless the catalogue carries the store's signature,
// the package matches the digest the catalogue lists for it, the package's
// manifest carries its developer's signature, and every file matches the
// manifest. Files land in a temporary folder that replaces the old version
// only once all of them are written.
class StoreEngine : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Loading, Ready, Failed };

    struct Installed
    {
        QString id;
        QString kind;
        QString name;
        QString version;
        QString developer;
        QString installedAt;
    };

    StoreEngine(const QString &dataDir, const QByteArray &storeKey, QObject *parent = nullptr);
    ~StoreEngine() override;

    QUrl catalogUrl() const { return m_catalogUrl; }
    void setCatalogUrl(const QUrl &url);
    // Fetches the catalogue (catalog.json and its signature, catalog.json.sig).
    void refresh();

    State state() const { return m_state; }
    QString error() const { return m_error; }
    const Catalog &catalog() const { return m_catalog; }
    const QMap<QString, Installed> &installed() const { return m_installed; }
    QString dataDir() const { return m_dataDir; }
    QString installDirectory(const QString &id) const;

    bool isBusy(const QString &id) const { return m_jobs.contains(id); }
    double progress(const QString &id) const { return m_progress.value(id, 0); }

    // Installs the catalogue's version of `id` (an update when an older one
    // is installed). Ends with finished().
    bool install(const QString &id);
    bool remove(const QString &id);

Q_SIGNALS:
    void stateChanged();
    void catalogChanged();
    void installedChanged();
    void progressChanged(const QString &id);
    void finished(const QString &id, bool ok, const QString &error);

private:
    using Done = std::function<void(bool ok, const QByteArray &data, const QString &error)>;
    void fetch(const QString &job, const QUrl &url, qint64 maximumSize, const Done &done);
    void setState(State state, const QString &error = {});
    void complete(const QString &id, const QByteArray &data);
    void fail(const QString &id, const QString &error);
    void loadInstalled();
    bool saveInstalled() const;

    QString m_dataDir;
    QByteArray m_storeKey;
    QUrl m_catalogUrl;
    State m_state = State::Idle;
    QString m_error;
    Catalog m_catalog;
    QMap<QString, Installed> m_installed;
    QNetworkAccessManager *m_network = nullptr;
    QHash<QString, QPointer<QNetworkReply>> m_jobs;
    QHash<QString, double> m_progress;
    int m_generation = 0;
};

} // namespace qor::store
