// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "store.h"

#include "catalog.h"
#include "package.h"
#include "storeengine.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>

#include <algorithm>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcStoreShell, "qor.store.shell")

namespace qor::store {

namespace {

const auto kFirstParty = QStringLiteral("dev.qorsync");

QJsonObject readJson(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 256 * 1024)
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

double clamped(const QJsonValue &value, double fallback, double low, double high)
{
    return value.isDouble() ? std::clamp(value.toDouble(), low, high) : fallback;
}

} // namespace

Store::Store(QObject *parent) : QObject(parent)
{
    StoreEngine *store = engine();
    connect(store, &StoreEngine::stateChanged, this, [this] {
        invalidate();
        Q_EMIT stateChanged();
    });
    // Whether what is installed is intact is asked of the developers' keys the catalogue holds.
    connect(store, &StoreEngine::catalogChanged, this, [this] {
        m_contentValid = false;
        invalidate();
        Q_EMIT installedChanged();
    });
    connect(store, &StoreEngine::progressChanged, this, &Store::invalidate);
    connect(store, &StoreEngine::installedChanged, this, [this] {
        m_contentValid = false;
        invalidate();
        Q_EMIT installedChanged();
    });
    connect(store, &StoreEngine::finished, this, &Store::finished);
}

StoreEngine *Store::engine()
{
    static StoreEngine *shared = [] {
        QString dir = qEnvironmentVariable("QOR_STORE_DIR");
        if (dir.isEmpty())
            dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/store"_s;
        auto *store = new StoreEngine(dir, storeKey(), QCoreApplication::instance());
        const QString url = qEnvironmentVariable("QOR_STORE_URL");
        store->setCatalogUrl(url.isEmpty() ? QUrl(u"qrc:/qt/qml/QOR/Store/catalog/catalog.json"_s)
                                           : QUrl::fromUserInput(url));
        store->refresh();
        return store;
    }();
    return shared;
}

QByteArray Store::storeKey()
{
    QFile file(u":/qt/qml/QOR/Store/keys/store.pub"_s);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QByteArray::fromBase64(file.readAll().trimmed());
}

bool Store::isReady() const { return engine()->state() == StoreEngine::State::Ready; }
bool Store::isLoading() const { return engine()->state() == StoreEngine::State::Loading; }
QString Store::error() const { return engine()->error(); }
QString Store::name() const { return engine()->catalog().name; }

void Store::invalidate()
{
    m_itemsValid = false;
    Q_EMIT itemsChanged();
}

QVariantMap Store::describe(const QString &id) const
{
    const StoreEngine *store = engine();
    const CatalogItem *item = store->catalog().item(id);
    const auto installed = store->installed().constFind(id);
    const bool isInstalled = installed != store->installed().cend();
    if (!item && !isInstalled)
        return {};

    QVariantMap map;
    QString developerId;
    if (item) {
        map = item->toVariant();
        developerId = item->developer;
    } else {
        map = { { u"id"_s, id },
                { u"kind"_s, installed->kind },
                { u"name"_s, installed->name },
                { u"version"_s, installed->version },
                { u"price"_s, 0 } };
        developerId = installed->developer;
    }
    const Developer developer = store->catalog().developers.value(developerId);
    map.insert(u"developerName"_s, developer.name.isEmpty() ? developerId : developer.name);
    map.insert(u"verified"_s, developer.verified);
    map.insert(u"inCatalog"_s, item != nullptr);
    map.insert(u"installed"_s, isInstalled);
    map.insert(u"installedVersion"_s, isInstalled ? installed->version : QString());
    map.insert(u"updateAvailable"_s, isInstalled && item && compareVersions(installed->version, item->version) < 0);
    map.insert(u"busy"_s, store->isBusy(id));
    map.insert(u"progress"_s, store->progress(id));
    return map;
}

QVariantList Store::items() const
{
    if (!m_itemsValid) {
        m_items.clear();
        for (const CatalogItem &item : engine()->catalog().items)
            m_items.append(describe(item.id));
        m_itemsValid = true;
    }
    return m_items;
}

QVariantList Store::library() const
{
    QVariantList list;
    for (const StoreEngine::Installed &installed : engine()->installed())
        list.append(describe(installed.id));
    return list;
}

int Store::updateCount() const
{
    int count = 0;
    for (const QVariant &item : library())
        count += item.toMap().value(u"updateAvailable"_s).toBool() ? 1 : 0;
    return count;
}

void Store::refresh() { engine()->refresh(); }
bool Store::install(const QString &id) { return engine()->install(id); }
bool Store::remove(const QString &id) { return engine()->remove(id); }
QVariantMap Store::item(const QString &id) const { return describe(id); }

QVariantList Store::search(const QString &query, const QString &kind) const
{
    const QStringList words = query.simplified().toLower().split(u' ', Qt::SkipEmptyParts);
    QVariantList found;
    for (const QVariant &value : items()) {
        const QVariantMap item = value.toMap();
        if (!kind.isEmpty() && item.value(u"kind"_s).toString() != kind)
            continue;
        const QString haystack =
            (item.value(u"name"_s).toString() + u' ' + item.value(u"summary"_s).toString() + u' '
             + item.value(u"kind"_s).toString() + u' ' + item.value(u"tags"_s).toStringList().join(u' '))
                .toLower();
        if (std::all_of(words.cbegin(), words.cend(), [&](const QString &word) { return haystack.contains(word); }))
            found.append(item);
    }
    return found;
}

// Installed themes and widgets, read back from their verified files.
void Store::loadInstalledContent() const
{
    if (m_contentValid)
        return;
    m_themes.clear();
    m_widgets.clear();
    const StoreEngine *store = engine();
    for (const StoreEngine::Installed &installed : store->installed()) {
        const QString dir = store->installDirectory(installed.id);
        const QString developer = describe(installed.id).value(u"developerName"_s).toString();
        // Verified as it is loaded, not only as it was installed: anything running as this user can write there, and a
        // widget's QML runs inside the shell.
        if (!store->isIntact(installed.id)) {
            qCWarning(lcStoreShell, "%s is not what its developer signed, and is not loaded", qPrintable(installed.id));
            continue;
        }
        if (installed.kind == u"theme"_s) {
            const QJsonObject theme = readJson(dir + u"/theme.json"_s);
            if (theme.isEmpty())
                continue;
            const QJsonObject neutral = theme.value(u"neutral"_s).toObject();
            const QJsonObject wallpaper = theme.value(u"wallpaper"_s).toObject();
            const QString appearance = theme.value(u"appearance"_s).toString();
            m_themes.append(QVariantMap {
                { u"id"_s, installed.id },
                { u"name"_s, installed.name },
                { u"developerName"_s, developer },
                { u"accentHue"_s, clamped(theme.value(u"accentHue"_s), 274, 0, 360) },
                { u"appearance"_s, appearance == u"dark"_s || appearance == u"light"_s ? appearance : u"any"_s },
                { u"neutralHue"_s, clamped(neutral.value(u"hue"_s), 262, 0, 360) },
                { u"neutralTint"_s, clamped(neutral.value(u"tint"_s), 1, 0, 2.5) },
                { u"wallpaperSeed"_s, clamped(wallpaper.value(u"seed"_s), 42, 0, 1000) },
                { u"wallpaperIntensity"_s, clamped(wallpaper.value(u"intensity"_s), 1, 0.3, 1.6) },
            });
        } else if (installed.kind == u"widget"_s) {
            // A widget runs inside Prism, with all the shell can do: only QOR OS's own (docs/STORE.md).
            if (installed.developer != kFirstParty)
                continue;
            const QJsonObject widget = readJson(dir + u"/widget.json"_s);
            const QString entry = widget.value(u"entry"_s).toString();
            const QString size = widget.value(u"size"_s).toString();
            if (!isSafePackagePath(entry) || !entry.endsWith(u".qml"_s) || !QFileInfo::exists(dir + u'/' + entry)
                || (size != u"small"_s && size != u"medium"_s && size != u"large"_s))
                continue;
            m_widgets.append(QVariantMap {
                { u"id"_s, installed.id },
                { u"name"_s, installed.name },
                { u"developerName"_s, developer },
                { u"size"_s, size },
                { u"glyph"_s, widget.value(u"glyph"_s).toString(u"layout-grid"_s) },
                { u"hue"_s, clamped(widget.value(u"hue"_s), 274, 0, 360) },
                { u"source"_s, QUrl::fromLocalFile(dir + u'/' + entry) },
            });
        }
    }
    const auto byName = [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(u"name"_s).toString() < b.toMap().value(u"name"_s).toString();
    };
    std::sort(m_themes.begin(), m_themes.end(), byName);
    std::sort(m_widgets.begin(), m_widgets.end(), byName);
    m_contentValid = true;
}

QVariantList Store::themes() const
{
    loadInstalledContent();
    return m_themes;
}

QVariantList Store::widgets() const
{
    loadInstalledContent();
    return m_widgets;
}

QVariantMap Store::theme(const QString &id) const
{
    for (const QVariant &theme : themes())
        if (theme.toMap().value(u"id"_s).toString() == id)
            return theme.toMap();
    return {};
}

QVariantMap Store::widget(const QString &id) const
{
    // Checked again as it is asked for: the list above may be as old as this session.
    if (!engine()->isIntact(id))
        return {};
    for (const QVariant &widget : widgets())
        if (widget.toMap().value(u"id"_s).toString() == id)
            return widget.toMap();
    return {};
}

} // namespace qor::store
