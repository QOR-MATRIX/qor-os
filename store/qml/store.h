// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace qtos::store {

class StoreEngine;

// QML singleton `Store`: the QTOS Store for the shell and its apps.
//
// The catalogue comes from QTOS_STORE_URL (a catalog.json URL) or, by
// default, the signed first-party catalogue built into QTOS. Packages install
// per user under QTOS_STORE_DIR, by default the app data folder's "store".
//
// Items are plain maps: the catalogue entry plus `developerName`, `verified`,
// `installed`, `installedVersion`, `updateAvailable`, `busy` and `progress`.
class Store : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool ready READ isReady NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool loading READ isLoading NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY itemsChanged FINAL)
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged FINAL)
    Q_PROPERTY(QVariantList library READ library NOTIFY itemsChanged FINAL)
    Q_PROPERTY(int updateCount READ updateCount NOTIFY itemsChanged FINAL)
    Q_PROPERTY(QVariantList themes READ themes NOTIFY installedChanged FINAL)
    Q_PROPERTY(QVariantList widgets READ widgets NOTIFY installedChanged FINAL)

public:
    explicit Store(QObject *parent = nullptr);

    // The process-wide store client (created on first use).
    static StoreEngine *engine();
    // The QTOS Store's public key: the root of trust for every install.
    static QByteArray storeKey();

    bool isReady() const;
    bool isLoading() const;
    QString error() const;
    QString name() const;
    QVariantList items() const;
    QVariantList library() const;
    int updateCount() const;
    QVariantList themes() const;
    QVariantList widgets() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool install(const QString &id);
    Q_INVOKABLE bool remove(const QString &id);
    Q_INVOKABLE QVariantMap item(const QString &id) const;
    // Items whose name, summary or tags contain every word of `query`,
    // optionally of one kind.
    Q_INVOKABLE QVariantList search(const QString &query, const QString &kind = {}) const;
    Q_INVOKABLE QVariantMap theme(const QString &id) const;
    Q_INVOKABLE QVariantMap widget(const QString &id) const;

Q_SIGNALS:
    void stateChanged();
    void itemsChanged();
    void installedChanged();
    void finished(const QString &id, bool ok, const QString &message);

private:
    QVariantMap describe(const QString &id) const;
    void invalidate();
    void loadInstalledContent() const;

    mutable bool m_itemsValid = false;
    mutable QVariantList m_items;
    mutable bool m_contentValid = false;
    mutable QVariantList m_themes;
    mutable QVariantList m_widgets;
};

} // namespace qtos::store
