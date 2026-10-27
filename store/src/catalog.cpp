// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "catalog.h"

#include "signature.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>

using namespace Qt::StringLiterals;

namespace qor::store {

QVariantMap CatalogItem::toVariant() const
{
    return {
        { u"id"_s, id },
        { u"kind"_s, kind },
        { u"name"_s, name },
        { u"developer"_s, developer },
        { u"version"_s, version },
        { u"summary"_s, summary },
        { u"description"_s, description },
        { u"whatsNew"_s, whatsNew },
        { u"released"_s, released },
        { u"tags"_s, tags },
        { u"featured"_s, featured },
        { u"price"_s, price },
        { u"preview"_s, preview },
        { u"size"_s, packageSize },
    };
}

const CatalogItem *Catalog::item(const QString &id) const
{
    for (const CatalogItem &candidate : items)
        if (candidate.id == id)
            return &candidate;
    return nullptr;
}

bool isValidItemId(const QString &id)
{
    static const QRegularExpression pattern(u"^[a-z][a-z0-9]*(\\.[a-z0-9][a-z0-9-]*){2,}$"_s);
    return id.size() <= 128 && pattern.match(id).hasMatch();
}

bool isValidVersion(const QString &version)
{
    static const QRegularExpression pattern(u"^\\d{1,5}\\.\\d{1,5}\\.\\d{1,5}$"_s);
    return pattern.match(version).hasMatch();
}

int compareVersions(const QString &a, const QString &b)
{
    const QStringList left = a.split(u'.');
    const QStringList right = b.split(u'.');
    for (qsizetype i = 0; i < std::max(left.size(), right.size()); ++i) {
        const int x = i < left.size() ? left.at(i).toInt() : 0;
        const int y = i < right.size() ? right.at(i).toInt() : 0;
        if (x != y)
            return x < y ? -1 : 1;
    }
    return 0;
}

CatalogResult parseCatalog(const QByteArray &json, const QByteArray &signature, const QByteArray &storeKey)
{
    CatalogResult result;
    if (!verifySignature(storeKey, json, signature)) {
        result.error = u"The catalogue's store signature does not match."_s;
        return result;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (!document.isObject()) {
        result.error = u"The catalogue is not valid JSON: %1"_s.arg(parseError.errorString());
        return result;
    }
    const QJsonObject root = document.object();
    if (root.value(u"format"_s).toInt() != 1) {
        result.error = u"Unsupported catalogue format."_s;
        return result;
    }
    Catalog &catalog = result.catalog;
    catalog.name = root.value(u"store"_s).toString();

    const QJsonObject developers = root.value(u"developers"_s).toObject();
    for (auto it = developers.begin(); it != developers.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        Developer developer { it.key(), entry.value(u"name"_s).toString(), entry.value(u"verified"_s).toBool(),
                              QByteArray::fromBase64(entry.value(u"key"_s).toString().toLatin1()) };
        if (developer.key.size() != 65 || developer.name.isEmpty()) {
            result.error = u"Developer %1 has no valid key."_s.arg(developer.id);
            return result;
        }
        catalog.developers.insert(developer.id, developer);
    }

    for (const QJsonValue &value : root.value(u"items"_s).toArray()) {
        const QJsonObject entry = value.toObject();
        CatalogItem item;
        item.id = entry.value(u"id"_s).toString();
        item.kind = entry.value(u"kind"_s).toString();
        item.name = entry.value(u"name"_s).toString();
        item.developer = entry.value(u"developer"_s).toString();
        item.version = entry.value(u"version"_s).toString();
        item.summary = entry.value(u"summary"_s).toString();
        item.description = entry.value(u"description"_s).toString();
        item.whatsNew = entry.value(u"whatsNew"_s).toString();
        item.released = entry.value(u"released"_s).toString();
        for (const QJsonValue &tag : entry.value(u"tags"_s).toArray())
            item.tags.append(tag.toString());
        item.featured = entry.value(u"featured"_s).toBool();
        item.price = entry.value(u"price"_s).toDouble();
        item.preview = entry.value(u"preview"_s).toObject().toVariantMap();
        const QJsonObject package = entry.value(u"package"_s).toObject();
        item.packageFile = package.value(u"file"_s).toString();
        item.packageSize = package.value(u"size"_s).toInteger();
        item.packageSha256 = QByteArray::fromHex(package.value(u"sha256"_s).toString().toLatin1());

        const bool known = item.kind == u"theme"_s || item.kind == u"widget"_s;
        if (!isValidItemId(item.id) || !known || item.name.isEmpty() || !isValidVersion(item.version)
            || !catalog.developers.contains(item.developer) || item.packageSha256.size() != 32
            || item.packageFile.isEmpty() || item.packageFile.contains(u".."_s) || item.packageFile.startsWith(u'/')
            || catalog.item(item.id)) {
            result.error = u"Catalogue entry %1 is not valid."_s.arg(item.id.isEmpty() ? u"?"_s : item.id);
            return result;
        }
        catalog.items.append(item);
    }
    result.ok = true;
    return result;
}

} // namespace qor::store
