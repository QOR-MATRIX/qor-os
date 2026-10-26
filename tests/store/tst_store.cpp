// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "catalog.h"
#include "package.h"
#include "signature.h"
#include "store.h"
#include "storeengine.h"

#include <QtCore/QBuffer>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/private/qzipreader_p.h>
#include <QtCore/private/qzipwriter_p.h>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

using namespace qtos::store;
using namespace Qt::StringLiterals;

namespace {

QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QByteArray publicKey(const QString &path) { return QByteArray::fromBase64(readFile(path).trimmed()); }

const QString kCatalog = QStringLiteral(QTOS_STORE_CATALOG);
const QString kFixtures = QStringLiteral(QTOS_STORE_FIXTURES);
const QString kStorePub = QStringLiteral(QTOS_STORE_PUB);

// Rewrites a package with `edit` applied to its entries.
QByteArray rezip(const QByteArray &archive, const std::function<void(QMap<QString, QByteArray> &)> &edit)
{
    QBuffer in;
    in.setData(archive);
    in.open(QIODevice::ReadOnly);
    QZipReader reader(&in);
    QMap<QString, QByteArray> entries;
    for (const QZipReader::FileInfo &info : reader.fileInfoList())
        entries.insert(info.filePath, reader.fileData(info.filePath));
    edit(entries);
    QBuffer out;
    out.open(QIODevice::WriteOnly);
    QZipWriter writer(&out);
    for (auto it = entries.cbegin(); it != entries.cend(); ++it)
        writer.addFile(it.key(), it.value());
    writer.close();
    return out.data();
}

struct Fixture
{
    QByteArray json;
    QByteArray signature;
    QString dir;
};

Fixture fixture(const QString &name)
{
    const QString dir = kFixtures + u'/' + name;
    return { readFile(dir + u"/catalog.json"_s), readFile(dir + u"/catalog.json.sig"_s), dir };
}

} // namespace

class TestStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void verifiesSignatures()
    {
        const QByteArray key = publicKey(kStorePub);
        const QByteArray json = readFile(kCatalog + u"/catalog.json"_s);
        const QByteArray signature = readFile(kCatalog + u"/catalog.json.sig"_s);
        QCOMPARE(key.size(), 65);
        QVERIFY(verifySignature(key, json, signature));

        QByteArray altered = json;
        altered[altered.size() / 2] = altered.at(altered.size() / 2) ^ 1;
        QVERIFY(!verifySignature(key, altered, signature));
        QByteArray badSignature = signature;
        badSignature[10] = badSignature.at(10) ^ 1;
        QVERIFY(!verifySignature(key, json, badSignature));
        QVERIFY(!verifySignature(key.left(64), json, signature));
        QVERIFY(!verifySignature(publicKey(kFixtures + u"/store.pub"_s), json, signature));
    }

    void readsTheFirstPartyCatalogue()
    {
        const CatalogResult result = parseCatalog(readFile(kCatalog + u"/catalog.json"_s),
                                                  readFile(kCatalog + u"/catalog.json.sig"_s), publicKey(kStorePub));
        QVERIFY2(result.ok, qPrintable(result.error));
        const Catalog &catalog = result.catalog;
        QCOMPARE(catalog.name, u"QTOS Store"_s);
        QVERIFY(catalog.developers.value(u"org.qtos"_s).verified);
        int themes = 0;
        int widgets = 0;
        for (const CatalogItem &item : catalog.items) {
            (item.kind == u"theme"_s ? themes : widgets)++;
            // Every package matches its listing and carries its developer's signature.
            const QByteArray package = readFile(kCatalog + u'/' + item.packageFile);
            QCOMPARE(QCryptographicHash::hash(package, QCryptographicHash::Sha256), item.packageSha256);
            QCOMPARE(package.size(), item.packageSize);
            const PackageResult opened = openPackage(package, catalog.developers.value(item.developer).key);
            QVERIFY2(opened.ok, qPrintable(item.id + u": "_s + opened.error));
            QCOMPARE(opened.manifest.id, item.id);
            QCOMPARE(opened.manifest.version, item.version);
            QVERIFY(opened.contents.contains(item.kind == u"theme"_s ? u"theme.json"_s : u"widget.json"_s));
        }
        QCOMPARE(themes, 8);
        QCOMPARE(widgets, 6);
    }

    void refusesCataloguesTheStoreDidNotSign()
    {
        const QByteArray key = publicKey(kFixtures + u"/store.pub"_s);
        const Fixture unsigned_ = fixture(u"unsigned"_s);
        const CatalogResult result = parseCatalog(unsigned_.json, unsigned_.signature, key);
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains(u"signature"_s));

        const Fixture v1 = fixture(u"v1"_s);
        QVERIFY(parseCatalog(v1.json, v1.signature, key).ok);
        QByteArray edited = v1.json;
        edited.replace("Test Theme", "Evil Theme");
        QVERIFY(!parseCatalog(edited, v1.signature, key).ok);
    }

    void refusesBadPackages()
    {
        const CatalogResult catalog = parseCatalog(readFile(kCatalog + u"/catalog.json"_s),
                                                   readFile(kCatalog + u"/catalog.json.sig"_s), publicKey(kStorePub));
        QVERIFY(catalog.ok);
        const CatalogItem *clock = catalog.catalog.item(u"org.qtos.widget.clock"_s);
        QVERIFY(clock);
        const QByteArray key = catalog.catalog.developers.value(u"org.qtos"_s).key;
        const QByteArray package = readFile(kCatalog + u'/' + clock->packageFile);
        QVERIFY(openPackage(package, key).ok);

        // A file changed after signing.
        const PackageResult changed =
            openPackage(rezip(package, [](auto &entries) { entries[u"Clock.qml"_s].append("\nRectangle {}"); }), key);
        QVERIFY(!changed.ok);
        QVERIFY2(changed.error.contains(u"Clock.qml"_s), qPrintable(changed.error));
        // An extra file, and a missing one.
        QVERIFY(!openPackage(rezip(package, [](auto &entries) { entries.insert(u"extra.qml"_s, "x"); }), key).ok);
        QVERIFY(!openPackage(rezip(package, [](auto &entries) { entries.remove(u"widget.json"_s); }), key).ok);
        // A path that climbs out of the install folder.
        QVERIFY(!openPackage(rezip(package, [](auto &entries) { entries.insert(u"../evil.qml"_s, "x"); }), key).ok);
        // Somebody else's key, and no signature at all.
        QVERIFY(!openPackage(package, publicKey(kFixtures + u"/store.pub"_s)).ok);
        QVERIFY(!openPackage(rezip(package, [](auto &entries) { entries.remove(u"manifest.sig"_s); }), key).ok);
        QVERIFY(!openPackage("not a zip", key).ok);

        QVERIFY(isSafePackagePath(u"qml/Clock.qml"_s));
        for (const QString &path : { u"../x"_s, u"/abs"_s, u"a/../b"_s, u"a\\b"_s, u""_s, u".hidden"_s, u"a//b"_s })
            QVERIFY2(!isSafePackagePath(path), qPrintable(path));
    }

    void comparesVersions()
    {
        QVERIFY(compareVersions(u"1.0.0"_s, u"1.1.0"_s) < 0);
        QVERIFY(compareVersions(u"1.10.0"_s, u"1.9.9"_s) > 0);
        QCOMPARE(compareVersions(u"2.0.0"_s, u"2.0.0"_s), 0);
        QVERIFY(isValidVersion(u"1.2.3"_s));
        QVERIFY(!isValidVersion(u"1.2"_s));
        QVERIFY(isValidItemId(u"org.qtos.theme.ember"_s));
        QVERIFY(!isValidItemId(u"Org.qtos.x"_s));
        QVERIFY(!isValidItemId(u"org.qtos"_s));
    }

    void installsUpdatesAndRemoves()
    {
        QTemporaryDir data;
        const QByteArray key = publicKey(kFixtures + u"/store.pub"_s);
        const QString theme = u"org.example.theme.test"_s;
        const QString widget = u"org.example.widget.test"_s;
        {
            StoreEngine store(data.path(), key);
            store.setCatalogUrl(QUrl::fromLocalFile(kFixtures + u"/v1/catalog.json"_s));
            store.refresh();
            QTRY_COMPARE(store.state(), StoreEngine::State::Ready);
            QCOMPARE(store.catalog().items.size(), 2);

            QSignalSpy finished(&store, &StoreEngine::finished);
            QVERIFY(store.install(theme));
            QVERIFY(store.isBusy(theme));
            QVERIFY(!store.install(theme)); // already under way
            QTRY_COMPARE(finished.count(), 1);
            QVERIFY2(finished.at(0).at(1).toBool(), qPrintable(finished.at(0).at(2).toString()));
            QVERIFY(store.install(widget));
            QTRY_COMPARE(finished.count(), 2);
            QVERIFY(finished.at(1).at(1).toBool());
            QCOMPARE(store.installed().value(theme).version, u"1.0.0"_s);
            QVERIFY(QFile::exists(store.installDirectory(widget) + u"/Test.qml"_s));
        }
        // What is installed is remembered.
        StoreEngine store(data.path(), key);
        QCOMPARE(store.installed().size(), 2);
        const QByteArray before = readFile(store.installDirectory(theme) + u"/theme.json"_s);

        // A newer catalogue offers an update.
        store.setCatalogUrl(QUrl::fromLocalFile(kFixtures + u"/v2/catalog.json"_s));
        store.refresh();
        QTRY_COMPARE(store.state(), StoreEngine::State::Ready);
        QVERIFY(compareVersions(store.installed().value(theme).version, store.catalog().item(theme)->version) < 0);
        QSignalSpy finished(&store, &StoreEngine::finished);
        QVERIFY(store.install(theme));
        QTRY_COMPARE(finished.count(), 1);
        QVERIFY(finished.at(0).at(1).toBool());
        QCOMPARE(store.installed().value(theme).version, u"1.1.0"_s);
        QVERIFY(readFile(store.installDirectory(theme) + u"/theme.json"_s) != before);

        QVERIFY(store.remove(widget));
        QVERIFY(!QFileInfo::exists(store.installDirectory(widget)));
        QCOMPARE(StoreEngine(data.path(), key).installed().size(), 1);
        // No staging or old folders are left behind.
        QCOMPARE(QDir(data.path() + u"/items"_s).entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden).size(), 1);
    }

    void refusesForgedPackagesAndUnsignedCatalogues()
    {
        QTemporaryDir data;
        const QByteArray key = publicKey(kFixtures + u"/store.pub"_s);
        StoreEngine store(data.path(), key);
        store.setCatalogUrl(QUrl::fromLocalFile(kFixtures + u"/forged/catalog.json"_s));
        store.refresh();
        QTRY_COMPARE(store.state(), StoreEngine::State::Ready);
        QSignalSpy finished(&store, &StoreEngine::finished);
        QVERIFY(store.install(u"org.example.theme.test"_s));
        QTRY_COMPARE(finished.count(), 1);
        QVERIFY(!finished.at(0).at(1).toBool());
        QVERIFY(finished.at(0).at(2).toString().contains(u"developer signature"_s));
        QVERIFY(store.installed().isEmpty());
        QCOMPARE(QDir(data.path() + u"/items"_s).entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden).size(), 0);

        store.setCatalogUrl(QUrl::fromLocalFile(kFixtures + u"/unsigned/catalog.json"_s));
        store.refresh();
        QTRY_COMPARE(store.state(), StoreEngine::State::Failed);
        QVERIFY(store.error().contains(u"signature"_s));
    }

    void describesTheBuiltInStore()
    {
        // The shell's view: the signed catalogue built into QTOS (qrc).
        QTemporaryDir data;
        qputenv("QTOS_STORE_DIR", data.path().toUtf8());
        qunsetenv("QTOS_STORE_URL");
        Store store;
        QTRY_VERIFY(store.isReady());
        QCOMPARE(store.items().size(), 14);
        QVariantMap ember = store.item(u"org.qtos.theme.ember"_s);
        QCOMPARE(ember.value(u"developerName"_s).toString(), u"QOR-MATRIX"_s);
        QVERIFY(ember.value(u"verified"_s).toBool());
        QVERIFY(!ember.value(u"installed"_s).toBool());
        QCOMPARE(ember.value(u"preview"_s).toMap().value(u"accentHue"_s).toDouble(), 38.0);

        QSignalSpy finished(&store, &Store::finished);
        QVERIFY(store.install(u"org.qtos.theme.ember"_s));
        QVERIFY(store.install(u"org.qtos.widget.clock"_s));
        QTRY_COMPARE(finished.count(), 2);
        QVERIFY(store.item(u"org.qtos.theme.ember"_s).value(u"installed"_s).toBool());
        const QVariantMap theme = store.theme(u"org.qtos.theme.ember"_s);
        QCOMPARE(theme.value(u"neutralHue"_s).toDouble(), 40.0);
        QCOMPARE(theme.value(u"appearance"_s).toString(), u"dark"_s);
        const QVariantMap clock = store.widget(u"org.qtos.widget.clock"_s);
        QCOMPARE(clock.value(u"size"_s).toString(), u"medium"_s);
        QVERIFY(QFile::exists(clock.value(u"source"_s).toUrl().toLocalFile()));
        QCOMPARE(store.library().size(), 2);
        QCOMPARE(store.updateCount(), 0);

        const QVariantList warm = store.search(u"warm"_s);
        QVERIFY(std::any_of(warm.cbegin(), warm.cend(), [](const QVariant &item) {
            return item.toMap().value(u"id"_s) == u"org.qtos.theme.ember"_s;
        }));
        QCOMPARE(store.search(QString(), u"widget"_s).size(), 6);
        QVERIFY(store.remove(u"org.qtos.widget.clock"_s));
        QVERIFY(store.widget(u"org.qtos.widget.clock"_s).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestStore)
#include "tst_store.moc"
