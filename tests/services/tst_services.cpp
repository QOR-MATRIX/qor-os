// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// End-to-end Qt Remote Objects round trips between the simulated sources
// (qtos-sim) and the replicas the shell consumes (QTOS.Services).

#include "services.h"
#include "simservices.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

using namespace Qt::StringLiterals;

class tst_Services : public QObject
{
    Q_OBJECT

private:
    qtos::sim::SimServices m_sim;
    QUrl m_url;

private Q_SLOTS:
    void initTestCase()
    {
        qtos::sim::SimOptions options;
        options.url = QUrl(u"local:qtos-test-"_s + QString::number(QCoreApplication::applicationPid()));
        options.persistent = false;
        QVERIFY(m_sim.start(options));
        m_url = m_sim.url();
        QVERIFY(qtos::Services::instance()->connectTo(m_url, 5000));
        QVERIFY(qtos::Services::instance()->isConnected());
    }

    void initialStateReplicates()
    {
        auto *services = qtos::Services::instance();
        QCOMPARE(services->network()->activeSsid(), u"Lumen Studio"_s);
        QVERIFY(services->network()->online());
        QCOMPARE(services->power()->batteryLevel(), 82);
        QCOMPARE(services->settings()->userName(), u"Alex Morgan"_s);
        QVERIFY(services->media()->available());
        QCOMPARE(services->media()->queue().size(), 5);
    }

    void replicaWriteReachesSource()
    {
        auto *settings = qtos::Services::instance()->settings();
        const bool before = m_sim.settings()->darkMode();
        settings->setDarkMode(!before);
        QTRY_COMPARE(m_sim.settings()->darkMode(), !before);
        QTRY_COMPARE(settings->darkMode(), !before);
    }

    void sourceChangeReachesReplica()
    {
        m_sim.power()->setBattery(37, true);
        auto *power = qtos::Services::instance()->power();
        QTRY_COMPARE(power->batteryLevel(), 37);
        QTRY_VERIFY(power->charging());
    }

    void remoteSlotAndSignal()
    {
        auto *notifications = qtos::Services::instance()->notifications();
        QSignalSpy posted(notifications, &NotificationsReplica::posted);
        notifications->notify(u"system"_s, u"Hello"_s, u"From the test"_s, u"bell"_s);
        QTRY_COMPARE(posted.count(), 1);
        QCOMPARE(posted.at(0).at(2).toString(), u"Hello"_s);
        QTRY_VERIFY(notifications->unreadCount() >= 1);
    }

    void wifiToggleDisconnectsAndReconnects()
    {
        auto *network = qtos::Services::instance()->network();
        network->setWifiEnabled(false);
        QTRY_VERIFY(!network->online());
        QTRY_COMPARE(network->activeSsid(), QString());
        network->setWifiEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(network->online(), 5000);
        QCOMPARE(network->activeSsid(), u"Lumen Studio"_s);
    }

    void connectToUnknownNetworkFails()
    {
        auto *network = qtos::Services::instance()->network();
        QSignalSpy failed(network, &NetworkReplica::connectionFailed);
        network->connectTo(u"Nowhere"_s);
        QTRY_COMPARE(failed.count(), 1);
    }

    void mediaQueueAdvances()
    {
        auto *media = qtos::Services::instance()->media();
        const QString first = media->title();
        media->next();
        QTRY_VERIFY(media->title() != first);
        media->playIndex(0);
        QTRY_COMPARE(media->title(), first);
    }
};

QTEST_MAIN(tst_Services)
#include "tst_services.moc"
