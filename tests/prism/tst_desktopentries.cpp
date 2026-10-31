// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// A Wayland client's app id is whatever it says. DesktopEntries (prism/shell/desktopentries.h) looks up only what could
// be a desktop file's name, reads only plain files of a sensible size, and remembers only so many: it runs on the
// shell's thread, so a pipe or a device would stop every window.

#include "desktopentries.h"

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#if defined(Q_OS_UNIX)
#include <sys/stat.h>
#endif

using namespace Qt::StringLiterals;

class TestDesktopEntries : public QObject
{
    Q_OBJECT

    QTemporaryDir m_data;
    QString m_apps;

    void write(const QString &name, const QByteArray &text)
    {
        QFile file(m_apps + u'/' + name);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(text);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_data.isValid());
        qputenv("XDG_DATA_HOME", m_data.path().toUtf8());
        qputenv("XDG_DATA_DIRS", m_data.path().toUtf8());
        m_apps = m_data.path() + u"/applications"_s;
        QVERIFY(QDir().mkpath(m_apps));
    }

    void a_real_entry_is_found()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("Windows has no desktop entries: every lookup reports none.");
#endif
        write(u"org.example.App.desktop"_s, "[Desktop Entry]\nName=Example\nStartupWMClass=exampleclass\n");
        qor::DesktopEntries entries;
        const QVariantMap found = entries.lookup(u"org.example.App"_s);
        QVERIFY(found.value(u"found"_s).toBool());
        QCOMPARE(found.value(u"name"_s).toString(), u"Example"_s);
        QVERIFY(entries.lookup(u"exampleclass"_s).value(u"found"_s).toBool());
    }

    void an_id_is_no_path()
    {
        QTemporaryDir outside;
        QFile file(outside.filePath(u"x.desktop"_s));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[Desktop Entry]\nName=Outside\n");
        file.close();
        qor::DesktopEntries entries;
        const QString climbing = QDir(m_apps).relativeFilePath(outside.filePath(u"x"_s));
        QVERIFY(climbing.startsWith(u".."_s));
        QVERIFY(!entries.lookup(climbing).value(u"found"_s).toBool());
        for (const QString &id :
             { u"../x"_s, u"a/b"_s, u".hidden"_s, u"client:settings"_s, QString(u'a').repeated(300) })
            QVERIFY2(!entries.lookup(id).value(u"found"_s).toBool(), qPrintable(id));
    }

    void an_entry_too_large_is_not_read()
    {
        write(u"too.large.desktop"_s, "[Desktop Entry]\nName=Large\n" + QByteArray(128 * 1024, '#'));
        qor::DesktopEntries entries;
        QVERIFY(!entries.lookup(u"too.large"_s).value(u"found"_s).toBool());
    }

#if defined(Q_OS_UNIX)
    // Read as it stands, each of these would stop the shell for good.
    void a_pipe_or_a_device_is_not_read()
    {
        QVERIFY(::mkfifo((m_apps + u"/a.pipe.desktop"_s).toUtf8().constData(), 0600) == 0);
        QVERIFY(QFile::link(u"/dev/zero"_s, m_apps + u"/a.device.desktop"_s));
        qor::DesktopEntries entries;
        QElapsedTimer took;
        took.start();
        QVERIFY(!entries.lookup(u"a.pipe"_s).value(u"found"_s).toBool());
        QVERIFY(!entries.lookup(u"a.device"_s).value(u"found"_s).toBool());
        QVERIFY(took.elapsed() < 2000);
    }
#endif

    // A client can change its id as often as it likes.
    void ids_changing_without_end_cost_little()
    {
        qor::DesktopEntries entries;
        QElapsedTimer took;
        took.start();
        for (int i = 0; i < 5000; ++i)
            entries.lookup(u"id"_s + QString::number(i));
        QVERIFY2(took.elapsed() < 2000, qPrintable(QString::number(took.elapsed())));
    }
};

QTEST_GUILESS_MAIN(TestDesktopEntries)
#include "tst_desktopentries.moc"
