// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// End-to-end Qt Remote Objects round trips between the simulated sources
// (qor-sim) and the replicas the shell consumes (QOR.Services).

#include "serviceendpoint.h"
#include "services.h"
#include "simservices.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#if defined(Q_OS_WIN)
#include <qt_windows.h>

#include <aclapi.h>
#endif

using namespace Qt::StringLiterals;

class tst_Services : public QObject
{
    Q_OBJECT

private:
    qor::sim::SimServices m_sim;
    QUrl m_url;

private Q_SLOTS:
    void initTestCase()
    {
        qor::sim::SimOptions options;
        options.url = qor::services::endpoint(u"qor-test-"_s + QString::number(QCoreApplication::applicationPid()));
        options.persistent = false;
        QVERIFY(m_sim.start(options));
        m_url = m_sim.url();
        QVERIFY(qor::Services::instance()->connectTo(m_url, 5000));
        QVERIFY(qor::Services::instance()->isConnected());
    }

    // The bus has no authentication of its own, and takes what it is sent on trust: nobody else may reach it. On Linux
    // it is a socket in a folder only this user can enter, never an abstract socket, which takes no permissions at all.
    void theBusIsThisPersonsAlone()
    {
#if defined(Q_OS_LINUX)
        const QUrl url = qor::Services::defaultUrl();
        QCOMPARE(url.scheme(), u"local"_s);
        QVERIFY(QDir::isAbsolutePath(url.path()));
        const QFileInfo folder(QFileInfo(url.path()).path());
        QVERIFY(folder.isDir());
        QCOMPARE(folder.ownerId(), QFileInfo(QDir::homePath()).ownerId());
        const auto others = QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup | QFile::ReadOther
                            | QFile::WriteOther | QFile::ExeOther;
        QVERIFY2(!(folder.permissions() & others), qPrintable(folder.filePath()));
        const QFileInfo socket(m_url.path());
        QVERIFY(socket.exists());
        QVERIFY2(!(socket.permissions() & others), qPrintable(socket.filePath()));
#elif defined(Q_OS_WIN)
        // Left alone, Windows lets everyone on the machine read a new pipe. Here the only allowance is this user's.
        const QString pipe = u"\\\\.\\pipe\\"_s + m_url.path();
        const HANDLE handle =
            CreateFileW(reinterpret_cast<LPCWSTR>(pipe.utf16()), READ_CONTROL, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        PACL allowances = nullptr;
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        const DWORD read = GetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr,
                                           &allowances, nullptr, &descriptor);
        CloseHandle(handle);
        QCOMPARE(read, DWORD(ERROR_SUCCESS));
        const auto freed = qScopeGuard([descriptor] { LocalFree(descriptor); });
        QVERIFY(allowances);

        HANDLE token = nullptr;
        QVERIFY(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token));
        const auto closed = qScopeGuard([token] { CloseHandle(token); });
        DWORD size = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        QByteArray user(qsizetype(size), Qt::Uninitialized);
        QVERIFY(GetTokenInformation(token, TokenUser, user.data(), size, &size));
        const PSID me = reinterpret_cast<PTOKEN_USER>(user.data())->User.Sid;

        QVERIFY(allowances->AceCount > 0);
        for (DWORD i = 0; i < allowances->AceCount; ++i) {
            ACCESS_ALLOWED_ACE *allowance = nullptr;
            QVERIFY(GetAce(allowances, i, reinterpret_cast<LPVOID *>(&allowance)));
            QCOMPARE(allowance->Header.AceType, BYTE(ACCESS_ALLOWED_ACE_TYPE));
            QVERIFY(EqualSid(reinterpret_cast<PSID>(&allowance->SidStart), me));
        }
#else
        QSKIP("No bus here.");
#endif
    }

    void initialStateReplicates()
    {
        auto *services = qor::Services::instance();
        QCOMPARE(services->network()->activeSsid(), u"Lumen Studio"_s);
        QVERIFY(services->network()->online());
        QCOMPARE(services->power()->batteryLevel(), 82);
        QCOMPARE(services->settings()->userName(), u"Alex Morgan"_s);
        QVERIFY(services->media()->available());
        QCOMPARE(services->media()->queue().size(), 5);
    }

    void replicaWriteReachesSource()
    {
        auto *settings = qor::Services::instance()->settings();
        const bool before = m_sim.settings()->darkMode();
        settings->setDarkMode(!before);
        QTRY_COMPARE(m_sim.settings()->darkMode(), !before);
        QTRY_COMPARE(settings->darkMode(), !before);
    }

    void sourceChangeReachesReplica()
    {
        m_sim.power()->setBattery(37, true);
        auto *power = qor::Services::instance()->power();
        QTRY_COMPARE(power->batteryLevel(), 37);
        QTRY_VERIFY(power->charging());
    }

    void remoteSlotAndSignal()
    {
        auto *notifications = qor::Services::instance()->notifications();
        QSignalSpy posted(notifications, &NotificationsReplica::posted);
        notifications->notify(u"system"_s, u"Hello"_s, u"From the test"_s, u"bell"_s);
        QTRY_COMPARE(posted.count(), 1);
        QCOMPARE(posted.at(0).at(2).toString(), u"Hello"_s);
        QTRY_VERIFY(notifications->unreadCount() >= 1);
    }

    void wifiToggleDisconnectsAndReconnects()
    {
        auto *network = qor::Services::instance()->network();
        network->setWifiEnabled(false);
        QTRY_VERIFY(!network->online());
        QTRY_COMPARE(network->activeSsid(), QString());
        network->setWifiEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(network->online(), 5000);
        QCOMPARE(network->activeSsid(), u"Lumen Studio"_s);
    }

    void connectToUnknownNetworkFails()
    {
        auto *network = qor::Services::instance()->network();
        QSignalSpy failed(network, &NetworkReplica::connectionFailed);
        network->connectTo(u"Nowhere"_s);
        QTRY_COMPARE(failed.count(), 1);
    }

    void mediaQueueAdvances()
    {
        auto *media = qor::Services::instance()->media();
        const QString first = media->title();
        media->next();
        QTRY_VERIFY(media->title() != first);
        media->playIndex(0);
        QTRY_COMPARE(media->title(), first);
    }

    // The audio engine renders silently under test (QOR_AUDIO_OUTPUT=null)
    // but in real time, so playback, positions and analysis are all real.
    void playbackRunsThroughTheAudioEngine()
    {
        auto *media = qor::Services::instance()->media();
        auto *audio = qor::Services::instance()->audio();
        QSignalSpy analysis(audio, &AudioReplica::analysis);
        media->playIndex(0);
        QTRY_VERIFY(media->playing());
        QTRY_VERIFY_WITH_TIMEOUT(media->position() > 0.5, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(audio->active(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(analysis.count() > 10, 5000);
        QCOMPARE(analysis.last().at(0).toList().size(), 48);

        media->setPlaying(false);
        QTRY_VERIFY_WITH_TIMEOUT(!audio->active(), 5000);
        const double paused = media->position();
        QTest::qWait(400);
        QCOMPARE(media->position(), paused);

        media->seek(30);
        QTRY_COMPARE(media->position(), 30.0);
    }

    void equalizerPresetsAndBands()
    {
        auto *audio = qor::Services::instance()->audio();
        QVERIFY(audio->equalizerPresets().contains(u"Bass Boost"_s));
        audio->applyEqualizerPreset(u"Bass Boost"_s);
        QTRY_COMPARE(audio->equalizerPreset(), u"Bass Boost"_s);
        QCOMPARE(audio->equalizer().value(0).toDouble(), 6.0);

        audio->setEqualizerBand(2, 1.3);
        QTRY_COMPARE(audio->equalizerPreset(), u"Custom"_s);
        QCOMPARE(audio->equalizer().value(2).toDouble(), 1.5);

        audio->applyEqualizerPreset(u"Flat"_s);
        QTRY_COMPARE(audio->equalizer(), QVariantList({ 0.0, 0.0, 0.0, 0.0, 0.0 }));
    }

    void repeatOneReplaysTheTrack()
    {
        auto *media = qor::Services::instance()->media();
        media->setRepeat(2);
        media->setShuffle(false);
        media->playIndex(1);
        QTRY_COMPARE(media->queueIndex(), 1);
        QTRY_VERIFY(media->playing());
        QTRY_VERIFY(media->duration() > 10);
        media->seek(media->duration() - 0.4);
        QTRY_VERIFY(media->position() > media->duration() - 1);
        // The end comes, and the same track starts over.
        QTRY_VERIFY_WITH_TIMEOUT(media->position() < 1.0, 5000);
        QCOMPARE(media->queueIndex(), 1);
        QVERIFY(media->playing());
    }

    void repeatAllContinuesFromTheTop()
    {
        auto *media = qor::Services::instance()->media();
        media->setRepeat(1);
        const int last = int(media->queue().size()) - 1;
        media->playIndex(last);
        QTRY_COMPARE(media->queueIndex(), last);
        QTRY_VERIFY(media->duration() > 10);
        media->seek(media->duration() - 0.4);
        QTRY_COMPARE_WITH_TIMEOUT(media->queueIndex(), 0, 5000);
        QVERIFY(media->playing());
    }

    void repeatOffStopsAfterTheLastTrack()
    {
        auto *media = qor::Services::instance()->media();
        media->setRepeat(0);
        const int last = int(media->queue().size()) - 1;
        media->playIndex(last);
        QTRY_COMPARE(media->queueIndex(), last);
        QTRY_VERIFY(media->duration() > 10);
        media->seek(media->duration() - 0.4);
        QTRY_VERIFY_WITH_TIMEOUT(!media->playing(), 5000);
        QCOMPARE(media->queueIndex(), 0);
        media->setRepeat(1);
    }

    void shufflePlaysEveryTrackOncePerPass()
    {
        auto *media = qor::Services::instance()->media();
        media->setShuffle(true);
        QTRY_VERIFY(media->shuffle());
        media->playIndex(0);
        QTRY_COMPARE(media->queueIndex(), 0);
        QSet<int> seen { 0 };
        const int count = int(media->queue().size());
        for (int i = 1; i < count; ++i) {
            const int before = media->queueIndex();
            media->next();
            QTRY_VERIFY(media->queueIndex() != before);
            seen.insert(media->queueIndex());
        }
        QCOMPARE(seen.size(), count);
        media->setShuffle(false);
        media->setPlaying(false);
    }

    void reactiveSettingsReachTheSource()
    {
        auto *settings = qor::Services::instance()->settings();
        QVERIFY(settings->audioReactive());
        settings->setReactiveLowHz(300);
        settings->setReactiveHighHz(3000);
        settings->setReactiveIntensity(0.9);
        QTRY_COMPARE(m_sim.settings()->reactiveLowHz(), 300.0);
        QTRY_COMPARE(m_sim.settings()->reactiveHighHz(), 3000.0);
        QTRY_COMPARE(m_sim.settings()->reactiveIntensity(), 0.9);
    }
};

QTEST_MAIN(tst_Services)
#include "tst_services.moc"
