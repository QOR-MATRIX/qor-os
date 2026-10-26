// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "gamepadmanager.h"
#include "injection.h"
#include "mapping.h"
#include "navigator.h"
#include "playstation.h"
#include "systemgestures.h"
#include "touchpad.h"

#include <QtCore/QByteArray>
#include <QtGui/QPointingDevice>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <array>
#include <memory>

using namespace qtos::input;
using namespace Qt::StringLiterals;

namespace {

// Counts the events a window's other filters let through.
class EventCounter : public QObject
{
public:
    QHash<QEvent::Type, int> counts;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        ++counts[event->type()];
        return QObject::eventFilter(watched, event);
    }
};

// A USB DualShock 4 report (0x01, 64 bytes) with the given controls.
QByteArray ds4Usb(quint8 lx, quint8 ly, quint8 buttons0, quint8 buttons1, quint8 buttons2, quint8 l2, quint8 r2,
                  quint8 status = 0x1b)
{
    QByteArray report(64, '\0');
    auto *p = reinterpret_cast<quint8 *>(report.data());
    p[0] = 0x01;
    p[1] = lx;
    p[2] = ly;
    p[3] = 0x80;
    p[4] = 0x80;
    p[5] = buttons0;
    p[6] = buttons1;
    p[7] = buttons2;
    p[8] = l2;
    p[9] = r2;
    p[30] = status; // battery status byte (offset 29 of the input report)
    return report;
}

const quint8 *bytes(const QByteArray &data) { return reinterpret_cast<const quint8 *>(data.constData()); }

GamepadState press(Button button)
{
    GamepadState state;
    state.set(button, true);
    return state;
}

} // namespace

class TestInput : public QObject
{
    Q_OBJECT

private slots:
    // ── PlayStation ──────────────────────────────────────────────────────────
    void identifiesSonyControllers()
    {
        QCOMPARE(playstation::identify(0x054c, 0x09cc)->model, playstation::Model::DualShock4);
        QCOMPARE(playstation::identify(0x054c, 0x0ce6)->model, playstation::Model::DualSense);
        QVERIFY(!playstation::identify(0x054c, 0x1234));
        QVERIFY(!playstation::identify(0x045e, 0x09cc));
    }

    void parsesDualShock4OverUsb()
    {
        // Cross (0x20) with the hat pointing right (2); L1 (0x01) and Options
        // (0x20); PS (0x01); L2 fully pulled.
        const QByteArray report = ds4Usb(0xff, 0x00, 0x22, 0x21, 0x01, 0xff, 0x00, 0x1b);
        playstation::Battery battery;
        const auto state =
            playstation::parseInput(playstation::Model::DualShock4, bytes(report), report.size(), &battery);
        QVERIFY(state);
        QVERIFY(state->pressed(Button::South));
        QVERIFY(!state->pressed(Button::East));
        QVERIFY(state->pressed(Button::DPadRight));
        QVERIFY(!state->pressed(Button::DPadUp));
        QVERIFY(state->pressed(Button::LeftShoulder));
        QVERIFY(state->pressed(Button::Start));
        QVERIFY(state->pressed(Button::Guide));
        QVERIFY(state->pressed(Button::LeftTrigger));
        QCOMPARE(state->axis(Axis::LeftX), 1.0f);
        QCOMPARE(state->axis(Axis::LeftY), -1.0f);
        QCOMPARE(state->axis(Axis::LeftTrigger), 1.0f);
        // Cable connected (0x10), level 11: full.
        QCOMPARE(battery.percent, 100);
        QVERIFY(!battery.charging);
    }

    void parsesDualShock4OverBluetooth()
    {
        // Full Bluetooth report 0x11: the USB layout two bytes later.
        const QByteArray usb = ds4Usb(0x80, 0x80, 0x88 /* triangle, hat centred */, 0x00, 0x02, 0, 0);
        QByteArray bt(78, '\0');
        bt[0] = char(0x11);
        for (int i = 1; i < 32; ++i)
            bt[i + 2] = usb[i];
        const auto state = playstation::parseInput(playstation::Model::DualShock4, bytes(bt), bt.size());
        QVERIFY(state);
        QVERIFY(state->pressed(Button::North));
        QVERIFY(state->pressed(Button::Touchpad));
        QVERIFY(!state->pressed(Button::DPadUp));
        QVERIFY(!state->pressed(Button::DPadLeft));
    }

    void parsesDualSense()
    {
        // USB 0x01: LX LY RX RY L2 R2 counter buttons0 buttons1 buttons2.
        QByteArray usb(64, '\0');
        auto *p = reinterpret_cast<quint8 *>(usb.data());
        p[0] = 0x01;
        p[1] = 0x80;
        p[2] = 0x80;
        p[3] = 0x00; // right stick left
        p[4] = 0x80;
        p[6] = 0xff;        // R2
        p[8] = 0x40 | 0x08; // circle, hat centred
        p[9] = 0x10 | 0x80; // create, R3
        p[10] = 0x04;       // mute
        p[53] = 0x15;       // battery: charging, level 5
        playstation::Battery battery;
        auto state = playstation::parseInput(playstation::Model::DualSense, p, usb.size(), &battery);
        QVERIFY(state);
        QVERIFY(state->pressed(Button::East));
        QVERIFY(state->pressed(Button::Back));
        QVERIFY(state->pressed(Button::RightStick));
        QVERIFY(state->pressed(Button::Misc));
        QVERIFY(state->pressed(Button::RightTrigger));
        QCOMPARE(state->axis(Axis::RightX), -1.0f);
        QCOMPARE(battery.percent, 55);
        QVERIFY(battery.charging);

        // Full Bluetooth 0x31: the USB layout one byte later.
        QByteArray bt(78, '\0');
        bt[0] = char(0x31);
        for (int i = 1; i < 63; ++i)
            bt[i + 1] = usb[i];
        state = playstation::parseInput(playstation::Model::DualSense, bytes(bt), bt.size());
        QVERIFY(state && state->pressed(Button::East) && state->pressed(Button::RightTrigger));

        // Basic Bluetooth 0x01 (10 bytes): laid out like the DualShock 4.
        const QByteArray basic = ds4Usb(0x80, 0x80, 0x18 /* square, hat centred */, 0, 0, 0, 0).left(10);
        state = playstation::parseInput(playstation::Model::DualSense, bytes(basic), basic.size());
        QVERIFY(state && state->pressed(Button::West));
    }

    void ignoresUnrelatedReports()
    {
        QByteArray other(64, '\0');
        other[0] = char(0x05);
        QVERIFY(!playstation::parseInput(playstation::Model::DualShock4, bytes(other), other.size()));
        QVERIFY(!playstation::parseInput(playstation::Model::DualSense, bytes(other), 4));
    }

    void crcMatchesIeee()
    {
        // The CRC-32 check value: "123456789" -> 0xCBF43926 (seed byte '1').
        const QByteArray rest("23456789");
        QCOMPARE(playstation::crc32('1', bytes(rest), rest.size()), 0xCBF43926u);
    }

    void buildsOutputReports()
    {
        const QColor orange(255, 107, 0);
        QByteArray usb = playstation::outputReport(playstation::Model::DualShock4, false, 200, 100, orange);
        QCOMPARE(usb.size(), 32);
        QCOMPARE(quint8(usb[0]), 0x05);
        QCOMPARE(quint8(usb[1]), 0x03);
        QCOMPARE(quint8(usb[4]), 100); // weak (right)
        QCOMPARE(quint8(usb[5]), 200); // strong (left)
        QCOMPARE(quint8(usb[6]), 255);
        QCOMPARE(quint8(usb[7]), 107);
        QCOMPARE(quint8(usb[8]), 0);

        const QByteArray bt = playstation::outputReport(playstation::Model::DualShock4, true, 200, 100, orange);
        QCOMPARE(bt.size(), 78);
        QCOMPARE(quint8(bt[0]), 0x11);
        QCOMPARE(quint8(bt[8]), 255); // light bar red, three bytes later than USB
        const quint32 crc = playstation::crc32(0xa2, bytes(bt), 74);
        QCOMPARE(quint8(bt[74]), quint8(crc));
        QCOMPARE(quint8(bt[77]), quint8(crc >> 24));

        const QByteArray sense = playstation::outputReport(playstation::Model::DualSense, false, 0, 0, orange);
        QCOMPARE(quint8(sense[0]), 0x02);
        QCOMPARE(quint8(sense[2]), 0x04); // light bar control enable
        QCOMPARE(quint8(sense[45]), 255);
        QCOMPARE(quint8(sense[46]), 107);

        const QByteArray senseBt = playstation::outputReport(playstation::Model::DualSense, true, 0, 0, orange, 3);
        QCOMPARE(senseBt.size(), 78);
        QCOMPARE(quint8(senseBt[1]), 0x30); // sequence 3
        QCOMPARE(quint8(senseBt[2]), 0x10);
        QCOMPARE(quint8(senseBt[47]), 255);
    }

    // ── XInput, evdev, generic HID ───────────────────────────────────────────
    void mapsXInput()
    {
        XInputPad pad;
        pad.buttons = 0x1000 | 0x0400 | 0x0001 | 0x0200; // A, Guide, up, RB
        pad.thumbLY = 32767;                             // up
        pad.rightTrigger = 255;
        const GamepadState state = mapXInput(pad);
        QVERIFY(state.pressed(Button::South));
        QVERIFY(state.pressed(Button::Guide));
        QVERIFY(state.pressed(Button::DPadUp));
        QVERIFY(state.pressed(Button::RightShoulder));
        QVERIFY(state.pressed(Button::RightTrigger));
        QCOMPARE(state.axis(Axis::LeftY), -1.0f); // the screen's up
        QVERIFY(!state.pressed(Button::North));
    }

    void mapsEvdev()
    {
        using namespace evdev;
        std::array<AbsRange, AbsCount> ranges {};
        ranges[AbsX] = ranges[AbsY] = ranges[AbsRx] = ranges[AbsRy] = { -32768, 32767 };
        ranges[AbsZ] = ranges[AbsRz] = { 0, 255 };
        EvdevMapper xbox(ranges, true);
        xbox.handle(EvKey, BtnNorth, 1); // xpad's BTN_X is the west button
        xbox.handle(EvAbs, AbsZ, 255);   // left trigger
        xbox.handle(EvAbs, AbsHat0X, 1);
        QVERIFY(xbox.handle(EvSyn, SynReport, 0));
        QVERIFY(xbox.state().pressed(Button::West));
        QVERIFY(xbox.state().pressed(Button::LeftTrigger));
        QVERIFY(xbox.state().pressed(Button::DPadRight));
        QVERIFY(!xbox.handle(EvSyn, SynReport, 0)); // nothing new

        // A generic pad: right stick on Z/RZ, triggers on brake/gas.
        std::array<AbsRange, AbsCount> generic {};
        generic[AbsX] = generic[AbsY] = generic[AbsZ] = generic[AbsRz] = { 0, 255 };
        generic[AbsBrake] = generic[AbsGas] = { 0, 1023 };
        EvdevMapper pad(generic, false);
        pad.handle(EvKey, BtnNorth, 1);
        pad.handle(EvAbs, AbsZ, 0);
        pad.handle(EvAbs, AbsGas, 1023);
        pad.handle(EvSyn, SynReport, 0);
        QVERIFY(pad.state().pressed(Button::North));
        QCOMPARE(pad.state().axis(Axis::RightX), -1.0f);
        QVERIFY(pad.state().pressed(Button::RightTrigger));

        // After SYN_DROPPED the partial report is ignored.
        pad.handle(EvSyn, SynDropped, 0);
        pad.handle(EvKey, BtnSouth, 1);
        QVERIFY(!pad.handle(EvSyn, SynReport, 0));
    }

    void mapsGenericHid()
    {
        GenericHidReport android;
        android.buttonCount = 15;
        android.pressed = { 4, 12 }; // X, Start
        android.z = 1.0f;
        android.rz = 0.0f;
        android.hat = 4; // down
        GamepadState state = mapGenericHid(android);
        QVERIFY(state.pressed(Button::West));
        QVERIFY(state.pressed(Button::Start));
        QVERIFY(state.pressed(Button::DPadDown));
        QCOMPARE(state.axis(Axis::RightX), 1.0f);

        GenericHidReport directInput;
        directInput.buttonCount = 12;
        directInput.pressed = { 4, 10 };
        state = mapGenericHid(directInput);
        QVERIFY(state.pressed(Button::North));
        QVERIFY(state.pressed(Button::Start));
    }

    void deadZones()
    {
        float x = 0.1f, y = 0.05f;
        applyStickDeadZone(x, y);
        QCOMPARE(x, 0.0f);
        QCOMPARE(y, 0.0f);
        x = 1.0f;
        y = 0.0f;
        applyStickDeadZone(x, y);
        QCOMPARE(x, 1.0f);
        QCOMPARE(applyTriggerDeadZone(0.02f), 0.0f);
        QCOMPARE(applyTriggerDeadZone(1.0f), 1.0f);
    }

    // ── Manager ──────────────────────────────────────────────────────────────
    void managerReportsEdgesAndMergesDuplicates()
    {
        GamepadManager *manager = GamepadManager::instance();
        QSignalSpy connected(manager, &GamepadManager::connected);
        QSignalSpy pressed(manager, &GamepadManager::buttonPressed);
        QSignalSpy merged(manager, &GamepadManager::mergedChanged);
        QSignalSpy disconnected(manager, &GamepadManager::disconnected);

        // The same controller seen twice (Steam Input's virtual Xbox pad, say).
        const QString a = manager->addVirtual(Family::PlayStation, u"DualShock 4"_s);
        const QString b = manager->addVirtual(Family::Xbox, u"Xbox (Steam)"_s);
        QCOMPARE(connected.count(), 2);
        QCOMPARE(manager->gamepads().size(), 2);

        manager->setVirtualState(a, press(Button::South));
        manager->setVirtualState(b, press(Button::South));
        QCOMPARE(pressed.count(), 2); // per controller
        int mergedPresses = 0;
        for (const auto &args : merged) {
            const auto now = args.at(0).value<GamepadState>();
            const auto before = args.at(1).value<GamepadState>();
            mergedPresses += now.pressed(Button::South) && !before.pressed(Button::South);
        }
        QCOMPARE(mergedPresses, 1); // but one press for the interface
        QCOMPARE(manager->activeId(), b);

        // Dead zones apply to raw states.
        GamepadState drift;
        drift.axis(Axis::LeftX) = 0.08f;
        manager->setVirtualState(a, drift);
        QCOMPARE(manager->state(a).axis(Axis::LeftX), 0.0f);

        manager->removeVirtual(a);
        manager->removeVirtual(b);
        QCOMPARE(disconnected.count(), 2);
        QCOMPARE(manager->merged().buttons, 0u);
    }

    // ── Navigator ────────────────────────────────────────────────────────────
    void navigatorDrivesAWindow()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            Window {
                id: win
                width: 400; height: 300; visible: true
                property int clicks: 0
                property int rightClicks: 0
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: mouse => mouse.button === Qt.RightButton ? win.rightClicks++ : win.clicks++
                }
                TextInput { objectName: "field"; width: 100; height: 20 }
            })",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto *window = qobject_cast<QQuickWindow *>(root.get());
        QVERIFY(QTest::qWaitForWindowExposed(window));
        window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(window));

        GamepadManager *manager = GamepadManager::instance();
        const QString pad = manager->addVirtual(Family::Xbox, u"Test pad"_s);
        ControllerNavigator navigator;
        navigator.setWindow(window);
        navigator.setEnabled(true);
        QSignalSpy actions(&navigator, &ControllerNavigator::action);

        // Shell actions.
        manager->setVirtualState(pad, press(Button::North));
        manager->setVirtualState(pad, {});
        QCOMPARE(actions.count(), 1);
        QCOMPARE(actions.at(0).at(0).toString(), u"search"_s);

        // The left stick moves the cursor; South clicks where it is.
        const QPointF start = navigator.cursor();
        GamepadState stick;
        stick.axis(Axis::LeftX) = 1.0f;
        manager->setVirtualState(pad, stick);
        QTRY_VERIFY(navigator.cursor().x() > start.x() + 20);
        QVERIFY(navigator.cursorVisible());
        manager->setVirtualState(pad, {});
        manager->setVirtualState(pad, press(Button::South));
        QVERIFY(navigator.cursorPressed());
        manager->setVirtualState(pad, {});
        QTRY_COMPARE(window->property("clicks").toInt(), 1);
        manager->setVirtualState(pad, press(Button::West));
        manager->setVirtualState(pad, {});
        QTRY_COMPARE(window->property("rightClicks").toInt(), 1);

        // A focused text field gets typed text, and is reported.
        auto *field = window->findChild<QQuickItem *>(u"field"_s);
        field->forceActiveFocus();
        QTRY_VERIFY(navigator.textInputFocused());
        navigator.typeText(u"Hi 5"_s);
        QCOMPARE(field->property("text").toString(), u"Hi 5"_s);

        // With the keyboard open, the D-pad moves its selection instead.
        navigator.setKeyboardOpen(true);
        QSignalSpy moves(&navigator, &ControllerNavigator::keyboardMove);
        QSignalSpy keys(&navigator, &ControllerNavigator::keyboardButton);
        manager->setVirtualState(pad, press(Button::DPadRight));
        manager->setVirtualState(pad, {});
        manager->setVirtualState(pad, press(Button::South));
        manager->setVirtualState(pad, {});
        QCOMPARE(moves.count(), 1);
        QCOMPARE(moves.at(0).at(0).toInt(), 1);
        QCOMPARE(keys.count(), 1);
        QCOMPARE(keys.at(0).at(0).toInt(), int(Button::South));

        // A real mouse takes over from the controller's cursor.
        navigator.setKeyboardOpen(false);
        QTest::mouseMove(window, QPoint(10, 10));
        QTRY_VERIFY(!navigator.cursorVisible());

        manager->removeVirtual(pad);
    }

    void assemblesTouchpadFrames()
    {
        TouchpadFrames frames;
        const TouchpadContact a { 1, { 10, 10 } };
        const TouchpadContact b { 2, { 20, 10 } };
        const TouchpadContact c { 3, { 30, 10 } };
        const TouchpadContact padding { 0, {} };
        // Hybrid mode: three contacts in two reports of two finger collections.
        QVERIFY(!frames.add(3, { a, b }));
        const auto frame = frames.add(0, { c, padding });
        QVERIFY(frame);
        QCOMPARE(frame->size(), 3);
        QCOMPARE(frame->at(2).id, 3);
        // One report holds a whole frame; collections past the count are padding.
        const auto single = frames.add(1, { a, b });
        QVERIFY(single);
        QCOMPARE(single->size(), 1);
        // A report with no frame under way is ignored.
        QVERIFY(!frames.add(0, { a }));
    }

    void findsTouchpadSwipes()
    {
        TouchpadSwipe swipe;
        using Phase = TouchpadSwipe::Phase;
        // Two fingers scroll; they are not a swipe.
        QVERIFY(swipe.add({ { 1, { 10, 10 } }, { 2, { 20, 10 } } }).isEmpty());
        auto events = swipe.add({ { 1, { 10, 10 } }, { 2, { 20, 10 } }, { 3, { 30, 10 } } });
        QCOMPARE(events.size(), 1);
        QCOMPARE(events.at(0).phase, Phase::Begin);
        QCOMPARE(events.at(0).fingers, 3);
        // 2 mm to the right: 16 logical pixels.
        events = swipe.add({ { 1, { 12, 10 } }, { 2, { 22, 10 } }, { 3, { 32, 10 } } });
        QCOMPARE(events.size(), 1);
        QCOMPARE(events.at(0).phase, Phase::Update);
        QCOMPARE(events.at(0).delta, QPointF(16, 0));
        // A fourth finger landing elsewhere does not make the swipe jump.
        events = swipe.add({ { 1, { 13, 10 } }, { 2, { 23, 10 } }, { 3, { 33, 10 } }, { 4, { 80, 40 } } });
        QCOMPARE(events.size(), 1);
        QCOMPARE(events.at(0).delta, QPointF(8, 0));
        QCOMPARE(events.at(0).fingers, 4);
        // Fewer than three touching (a lifted contact does not count) ends it.
        events = swipe.add({ { 1, { 13, 10 } }, { 2, { 23, 10 } }, { 3, { 33, 10 }, false } });
        QCOMPARE(events.size(), 1);
        QCOMPARE(events.at(0).phase, Phase::End);
        QVERIFY(!swipe.reset());
    }

    void recognisesTouchpadSwipes()
    {
        QQuickWindow window;
        window.resize(800, 600);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        // Installed first, so it sees only what SystemGestures lets through.
        EventCounter seen;
        window.installEventFilter(&seen);
        SystemGestures gestures;
        gestures.setWindow(&window);
        QSignalSpy started(&gestures, &SystemGestures::started);
        QSignalSpy finished(&gestures, &SystemGestures::finished);

        QPointingDevice touchpad(u"test touchpad"_s, 0x7e5701, QInputDevice::DeviceType::TouchPad,
                                 QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 5, 0);
        QWindowSystemInterface::registerInputDevice(&touchpad);
        ulong time = 1000;
        const auto send = [&](Qt::NativeGestureType type, QPointF delta, int fingers) {
            const InjectedInput injected; // delivered synchronously
            QWindowSystemInterface::handleGestureEventWithValueAndDelta(&window, time, &touchpad, type, 0, delta,
                                                                        QPointF(400, 300), QPointF(400, 300), fingers);
        };

        // Three fingers: the swipe picks its axis past the slop, and a flick
        // releases with its velocity (104 px in 100 ms).
        send(Qt::BeginNativeGesture, {}, 3);
        send(Qt::PanNativeGesture, QPointF(-4, 1), 3);
        QVERIFY(!gestures.isActive());
        for (int i = 0; i < 10; ++i) {
            time += 10;
            send(Qt::PanNativeGesture, QPointF(-10, 0), 3);
        }
        QVERIFY(gestures.isActive());
        QCOMPARE(gestures.axis(), Qt::Horizontal);
        QCOMPARE(gestures.source(), u"touchpad"_s);
        QCOMPARE(started.count(), 1);
        QCOMPARE(gestures.translation(), QPointF(-104, 1));
        send(Qt::EndNativeGesture, {}, 3);
        QCOMPARE(finished.count(), 1);
        QVERIFY(!gestures.isActive());
        QVERIFY2(std::abs(gestures.velocity().x() + 1040) < 1, qPrintable(QString::number(gestures.velocity().x())));
        QCOMPARE(seen.counts.value(QEvent::NativeGesture), 0);

        // The axis stays put; fingers that stop before lifting release at rest.
        time += 1000;
        send(Qt::BeginNativeGesture, {}, 4);
        time += 10;
        send(Qt::PanNativeGesture, QPointF(2, -30), 4);
        QCOMPARE(gestures.axis(), Qt::Vertical);
        time += 10;
        send(Qt::PanNativeGesture, QPointF(200, 0), 4);
        QCOMPARE(gestures.axis(), Qt::Vertical);
        time += 300;
        send(Qt::EndNativeGesture, {}, 4);
        QCOMPARE(finished.count(), 2);
        QCOMPARE(gestures.velocity(), QPointF());

        // Two fingers are not a system gesture: they reach the window.
        time += 1000;
        send(Qt::BeginNativeGesture, {}, 2);
        send(Qt::PanNativeGesture, QPointF(-50, 0), 2);
        send(Qt::EndNativeGesture, {}, 2);
        QCOMPARE(started.count(), 2);
        QCOMPARE(seen.counts.value(QEvent::NativeGesture), 3);

        // Disabled, it lets everything through.
        gestures.setEnabled(false);
        send(Qt::BeginNativeGesture, {}, 3);
        send(Qt::PanNativeGesture, QPointF(-50, 0), 3);
        send(Qt::EndNativeGesture, {}, 3);
        QCOMPARE(started.count(), 2);
        QCOMPARE(seen.counts.value(QEvent::NativeGesture), 6);
    }

    void recognisesTouchscreenSwipes()
    {
        QQuickWindow window;
        window.resize(800, 600);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        EventCounter seen;
        window.installEventFilter(&seen);
        SystemGestures gestures;
        gestures.setWindow(&window);
        QSignalSpy started(&gestures, &SystemGestures::started);
        QSignalSpy finished(&gestures, &SystemGestures::finished);
        QPointingDevice *screen = QTest::createTouchDevice();

        // Fingers land one after another; until they move together past the
        // slop, the window gets every touch.
        QTest::touchEvent(&window, screen).press(0, QPoint(300, 300), &window);
        QTest::touchEvent(&window, screen).move(0, QPoint(300, 300), &window).press(1, QPoint(360, 300), &window);
        QTest::touchEvent(&window, screen)
            .move(0, QPoint(300, 300), &window)
            .move(1, QPoint(360, 300), &window)
            .press(2, QPoint(420, 300), &window);
        QTest::touchEvent(&window, screen)
            .move(0, QPoint(305, 300), &window)
            .move(1, QPoint(365, 300), &window)
            .move(2, QPoint(425, 300), &window);
        QCOMPARE(started.count(), 0);
        const int touches = seen.counts.value(QEvent::TouchBegin) + seen.counts.value(QEvent::TouchUpdate);
        QCOMPARE(touches, 4);

        // Then the swipe takes over: the window's items get a cancel and no
        // more touches.
        QTest::touchEvent(&window, screen)
            .move(0, QPoint(260, 300), &window)
            .move(1, QPoint(320, 300), &window)
            .move(2, QPoint(380, 300), &window);
        QCOMPARE(started.count(), 1);
        QCOMPARE(gestures.axis(), Qt::Horizontal);
        QCOMPARE(gestures.source(), u"touchscreen"_s);
        QCOMPARE(seen.counts.value(QEvent::TouchCancel), 1);
        QTest::touchEvent(&window, screen)
            .move(0, QPoint(200, 300), &window)
            .move(1, QPoint(260, 300), &window)
            .move(2, QPoint(320, 300), &window);
        QCOMPARE(gestures.translation().x(), -100.0);
        QTest::touchEvent(&window, screen)
            .release(0, QPoint(200, 300), &window)
            .release(1, QPoint(260, 300), &window)
            .release(2, QPoint(320, 300), &window);
        QCOMPARE(finished.count(), 1);
        QCOMPARE(seen.counts.value(QEvent::TouchBegin) + seen.counts.value(QEvent::TouchUpdate), touches);
        QCOMPARE(seen.counts.value(QEvent::TouchEnd), 0);

        // Two fingers stay with the window.
        QTest::touchEvent(&window, screen).press(0, QPoint(300, 300), &window).press(1, QPoint(360, 300), &window);
        QTest::touchEvent(&window, screen).move(0, QPoint(200, 300), &window).move(1, QPoint(260, 300), &window);
        QTest::touchEvent(&window, screen).release(0, QPoint(200, 300), &window).release(1, QPoint(260, 300), &window);
        QCOMPARE(started.count(), 1);
        QCOMPARE(seen.counts.value(QEvent::TouchEnd), 1);
    }
};

QTEST_MAIN(TestInput)
#include "tst_input.moc"
