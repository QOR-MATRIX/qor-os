// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

namespace qor::store {

// A publisher, as the store vouches for it. `key` is the developer's ECDSA
// P-256 public key (65-byte uncompressed point): every package from this
// developer carries a manifest signed with it.
struct Developer
{
    QString id;
    QString name;
    bool verified = false;
    QByteArray key;
};

// One listing in the catalogue.
struct CatalogItem
{
    QString id;   // reverse-DNS, e.g. dev.qorsync.theme.ember
    QString kind; // "theme" | "widget"
    QString name;
    QString developer; // a Developer id
    QString version;   // major.minor.patch
    QString summary;
    QString description;
    QString whatsNew;
    QString released; // ISO date
    QStringList tags;
    bool featured = false;
    double price = 0;    // in USD; 0 is free (paid items come with the developer program)
    QVariantMap preview; // shown before download (a theme's colours, a widget's size and glyph)
    QString packageFile; // relative to the catalogue URL
    qint64 packageSize = 0;
    QByteArray packageSha256; // raw digest

    QVariantMap toVariant() const;
};

struct Catalog
{
    QString name;
    QHash<QString, Developer> developers;
    QList<CatalogItem> items;

    const CatalogItem *item(const QString &id) const;
};

struct CatalogResult
{
    bool ok = false;
    QString error;
    Catalog catalog;
};

// Checks the store's signature over the catalogue's exact bytes, then
// parses and validates it. Nothing in an unsigned or altered catalogue is
// trusted, not even its developer keys.
CatalogResult parseCatalog(const QByteArray &json, const QByteArray &signature, const QByteArray &storeKey);

// Compares dotted versions numerically: <0, 0 or >0.
int compareVersions(const QString &a, const QString &b);

bool isValidItemId(const QString &id);
bool isValidVersion(const QString &version);

} // namespace qor::store
