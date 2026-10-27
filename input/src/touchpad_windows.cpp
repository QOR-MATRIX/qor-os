// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Precision touchpads on Windows, read through Raw Input. Windows keeps
// multi-finger touchpad gestures for itself and Qt reports none, so QOR OS reads
// the contacts (HID usage page 0x0D, usage 0x05) and finds its own swipes.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// After windows.h, in this order: NTSTATUS, then the HID usage types.
#include <winternl.h>

#include <hidsdi.h>

#include <hidpi.h>

#include "touchpad.h"

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtGui/QPointingDevice>
#include <QtGui/qpa/qwindowsysteminterface.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <vector>

Q_LOGGING_CATEGORY(lcTouchpad, "qor.input.touchpad")

namespace qor::input {

namespace {

constexpr USHORT kDigitizer = 0x0D;
constexpr USHORT kGenericDesktop = 0x01;
constexpr USAGE kTouchPad = 0x05;
constexpr USAGE kTipSwitch = 0x42;
constexpr USAGE kConfidence = 0x47;
constexpr USAGE kContactId = 0x51;
constexpr USAGE kContactCount = 0x54;
constexpr USAGE kX = 0x30;
constexpr USAGE kY = 0x31;

// A finger collection's coordinate: its logical range and millimetres per
// logical unit.
struct Axis
{
    LONG minimum = 0;
    LONG maximum = 0;
    qreal millimetresPerUnit = 0;
};

// The physical length of an axis in millimetres, from its HID units, or 0.
qreal physicalMillimetres(const HIDP_VALUE_CAPS &cap)
{
    if (cap.PhysicalMax <= cap.PhysicalMin)
        return 0;
    const ULONG system = cap.Units & 0xF;        // 1: SI linear (cm), 3: English linear (inch)
    const ULONG length = (cap.Units >> 4) & 0xF; // the length exponent, 1 for a distance
    if (length != 1 || (system != 1 && system != 3))
        return 0;
    const int exponent = cap.UnitsExp > 7 ? int(cap.UnitsExp) - 16 : int(cap.UnitsExp);
    const qreal value = qreal(cap.PhysicalMax - cap.PhysicalMin) * std::pow(10.0, exponent);
    return system == 1 ? value * 10 : value * 25.4;
}

struct Finger
{
    USHORT link = 0;
    Axis x;
    Axis y;
    bool hasId = false;
};

struct Device
{
    std::vector<char> preparsed;
    USHORT contactCountLink = 0;
    bool hasContactCount = false;
    bool hasConfidence = false;
    QList<Finger> fingers;
    TouchpadFrames frames;

    PHIDP_PREPARSED_DATA data() { return reinterpret_cast<PHIDP_PREPARSED_DATA>(preparsed.data()); }
};

class PrecisionTouchpad final : public TouchpadSource
{
public:
    PrecisionTouchpad()
    {
        m_device = new QPointingDevice(QStringLiteral("Precision touchpad"), 0x51504454,
                                       QInputDevice::DeviceType::TouchPad, QPointingDevice::PointerType::Finger,
                                       QInputDevice::Capability::Position | QInputDevice::Capability::Area, 5, 0,
                                       QString(), QPointingDeviceUniqueId(), this);
        QWindowSystemInterface::registerInputDevice(m_device);

        m_quiet.setSingleShot(true);
        m_quiet.setInterval(150);
        connect(&m_quiet, &QTimer::timeout, this, [this] {
            if (const auto end = m_swipe.reset())
                Q_EMIT swipe(end->phase, end->delta, end->fingers);
        });

        WNDCLASSEXW windowClass {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = &PrecisionTouchpad::windowProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"QorPrecisionTouchpad";
        RegisterClassExW(&windowClass);
        m_window = CreateWindowExW(0, windowClass.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                   windowClass.hInstance, nullptr);
        if (!m_window) {
            qCWarning(lcTouchpad) << "Cannot create the Raw Input window:" << GetLastError();
            return;
        }
        SetWindowLongPtrW(m_window, GWLP_USERDATA, LONG_PTR(this));
        // INPUTSINK: contacts arrive whichever window has focus; SystemGestures
        // acts on them only while a QOR OS window does.
        RAWINPUTDEVICE device { kDigitizer, kTouchPad, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, m_window };
        if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
            qCWarning(lcTouchpad) << "Cannot register for touchpad input:" << GetLastError();

        // Look at the touchpads now rather than at their first touch, so the
        // log says up front what was found.
        UINT count = 0;
        GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST));
        std::vector<RAWINPUTDEVICELIST> list(count);
        if (count && GetRawInputDeviceList(list.data(), &count, sizeof(RAWINPUTDEVICELIST)) != UINT(-1)) {
            for (UINT i = 0; i < count; ++i) {
                RID_DEVICE_INFO info {};
                info.cbSize = sizeof(info);
                UINT size = sizeof(info);
                if (list[i].dwType == RIM_TYPEHID
                    && GetRawInputDeviceInfoW(list[i].hDevice, RIDI_DEVICEINFO, &info, &size) != UINT(-1)
                    && info.hid.usUsagePage == kDigitizer && info.hid.usUsage == kTouchPad)
                    deviceFor(list[i].hDevice);
            }
        }
    }

    ~PrecisionTouchpad() override
    {
        if (m_window) {
            RAWINPUTDEVICE device { kDigitizer, kTouchPad, RIDEV_REMOVE, nullptr };
            RegisterRawInputDevices(&device, 1, sizeof(device));
            DestroyWindow(m_window);
        }
    }

    const QPointingDevice *device() const override { return m_device; }

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto *self = reinterpret_cast<PrecisionTouchpad *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (self && message == WM_INPUT)
            self->input(HRAWINPUT(lParam));
        else if (self && message == WM_INPUT_DEVICE_CHANGE && wParam == GIDC_REMOVAL)
            self->m_devices.erase(HANDLE(lParam));
        return DefWindowProcW(window, message, wParam, lParam);
    }

    Device *deviceFor(HANDLE handle)
    {
        auto it = m_devices.find(handle);
        if (it != m_devices.end())
            return it->second.get();
        auto device = std::make_unique<Device>();
        UINT size = 0;
        GetRawInputDeviceInfoW(handle, RIDI_PREPARSEDDATA, nullptr, &size);
        device->preparsed.resize(size);
        if (!size || GetRawInputDeviceInfoW(handle, RIDI_PREPARSEDDATA, device->preparsed.data(), &size) == UINT(-1))
            return nullptr;
        HIDP_CAPS caps {};
        if (HidP_GetCaps(device->data(), &caps) != HIDP_STATUS_SUCCESS)
            return nullptr;

        QHash<USHORT, Finger> fingers;
        std::vector<HIDP_VALUE_CAPS> values(caps.NumberInputValueCaps);
        USHORT count = caps.NumberInputValueCaps;
        if (count && HidP_GetValueCaps(HidP_Input, values.data(), &count, device->data()) == HIDP_STATUS_SUCCESS) {
            for (USHORT i = 0; i < count; ++i) {
                const HIDP_VALUE_CAPS &cap = values[i];
                const USAGE usage = cap.IsRange ? cap.Range.UsageMin : cap.NotRange.Usage;
                if (cap.UsagePage == kDigitizer && usage == kContactCount) {
                    device->contactCountLink = cap.LinkCollection;
                    device->hasContactCount = true;
                } else if (cap.UsagePage == kDigitizer && usage == kContactId) {
                    fingers[cap.LinkCollection].hasId = true;
                } else if (cap.UsagePage == kGenericDesktop && (usage == kX || usage == kY)) {
                    Finger &finger = fingers[cap.LinkCollection];
                    finger.link = cap.LinkCollection;
                    Axis &axis = usage == kX ? finger.x : finger.y;
                    axis.minimum = cap.LogicalMin;
                    axis.maximum = cap.LogicalMax;
                    const qreal millimetres = physicalMillimetres(cap);
                    // Without physical units, assume a 100 mm wide pad.
                    if (axis.maximum > axis.minimum)
                        axis.millimetresPerUnit =
                            (millimetres > 0 ? millimetres : 100.0) / qreal(axis.maximum - axis.minimum);
                }
            }
        }
        std::vector<HIDP_BUTTON_CAPS> buttons(caps.NumberInputButtonCaps);
        count = caps.NumberInputButtonCaps;
        if (count && HidP_GetButtonCaps(HidP_Input, buttons.data(), &count, device->data()) == HIDP_STATUS_SUCCESS) {
            for (USHORT i = 0; i < count; ++i) {
                const HIDP_BUTTON_CAPS &cap = buttons[i];
                const USAGE first = cap.IsRange ? cap.Range.UsageMin : cap.NotRange.Usage;
                const USAGE last = cap.IsRange ? cap.Range.UsageMax : cap.NotRange.Usage;
                if (cap.UsagePage == kDigitizer && first <= kConfidence && kConfidence <= last)
                    device->hasConfidence = true;
            }
        }
        for (const Finger &finger : std::as_const(fingers))
            if (finger.hasId && finger.x.millimetresPerUnit > 0 && finger.y.millimetresPerUnit > 0)
                device->fingers.append(finger);
        std::sort(device->fingers.begin(), device->fingers.end(),
                  [](const Finger &a, const Finger &b) { return a.link < b.link; });
        if (!device->hasContactCount || device->fingers.isEmpty()) {
            qCDebug(lcTouchpad) << "Not a precision touchpad:" << handle;
            m_devices.emplace(handle, nullptr);
            return nullptr;
        }
        const Finger &first = device->fingers.constFirst();
        qCInfo(lcTouchpad).nospace() << "Precision touchpad: " << device->fingers.size() << " finger collections, "
                                     << qRound((first.x.maximum - first.x.minimum) * first.x.millimetresPerUnit)
                                     << " x "
                                     << qRound((first.y.maximum - first.y.minimum) * first.y.millimetresPerUnit)
                                     << " mm" << (device->hasConfidence ? ", palm rejection" : "");
        return m_devices.emplace(handle, std::move(device)).first->second.get();
    }

    void input(HRAWINPUT handle)
    {
        UINT size = 0;
        GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        m_buffer.resize(size);
        if (!size || GetRawInputData(handle, RID_INPUT, m_buffer.data(), &size, sizeof(RAWINPUTHEADER)) == UINT(-1))
            return;
        const auto *raw = reinterpret_cast<const RAWINPUT *>(m_buffer.data());
        if (raw->header.dwType != RIM_TYPEHID)
            return;
        Device *device = deviceFor(raw->header.hDevice);
        if (!device)
            return;
        const RAWHID &hid = raw->data.hid;
        for (DWORD i = 0; i < hid.dwCount; ++i)
            report(*device, reinterpret_cast<PCHAR>(const_cast<BYTE *>(hid.bRawData)) + i * hid.dwSizeHid,
                   ULONG(hid.dwSizeHid));
    }

    void report(Device &device, PCHAR data, ULONG length)
    {
        ULONG value = 0;
        int contactCount = 0;
        if (HidP_GetUsageValue(HidP_Input, kDigitizer, device.contactCountLink, kContactCount, &value, device.data(),
                               data, length)
            == HIDP_STATUS_SUCCESS)
            contactCount = int(value);

        QList<TouchpadContact> contacts;
        for (const Finger &finger : std::as_const(device.fingers)) {
            USAGE usages[16];
            ULONG usageCount = 16;
            bool tip = false;
            bool confident = !device.hasConfidence;
            if (HidP_GetUsages(HidP_Input, kDigitizer, finger.link, usages, &usageCount, device.data(), data, length)
                == HIDP_STATUS_SUCCESS) {
                for (ULONG u = 0; u < usageCount; ++u) {
                    tip = tip || usages[u] == kTipSwitch;
                    confident = confident || usages[u] == kConfidence;
                }
            }
            ULONG id = 0;
            ULONG x = 0;
            ULONG y = 0;
            HidP_GetUsageValue(HidP_Input, kDigitizer, finger.link, kContactId, &id, device.data(), data, length);
            HidP_GetUsageValue(HidP_Input, kGenericDesktop, finger.link, kX, &x, device.data(), data, length);
            HidP_GetUsageValue(HidP_Input, kGenericDesktop, finger.link, kY, &y, device.data(), data, length);
            // A palm (no confidence) counts as lifted.
            contacts.append({ int(id),
                              QPointF((LONG(x) - finger.x.minimum) * finger.x.millimetresPerUnit,
                                      (LONG(y) - finger.y.minimum) * finger.y.millimetresPerUnit),
                              tip && confident });
        }
        const auto frame = device.frames.add(contactCount, contacts);
        if (!frame)
            return;
        if (lcTouchpad().isDebugEnabled()) {
            QStringList fingers;
            for (const TouchpadContact &contact : *frame)
                fingers.append(QStringLiteral("%1%2 (%3, %4) mm")
                                   .arg(contact.id)
                                   .arg(contact.touching ? QString() : QStringLiteral(" lifted"))
                                   .arg(contact.position.x(), 0, 'f', 1)
                                   .arg(contact.position.y(), 0, 'f', 1));
            qCDebug(lcTouchpad) << "frame:" << fingers;
        }
        for (const TouchpadSwipe::Event &event : m_swipe.add(*frame)) {
            qCDebug(lcTouchpad) << "swipe" << int(event.phase) << event.delta << event.fingers;
            Q_EMIT swipe(event.phase, event.delta, event.fingers);
        }
        if (m_swipe.isActive())
            m_quiet.start();
        else
            m_quiet.stop();
    }

    HWND m_window = nullptr;
    QPointingDevice *m_device = nullptr;
    std::map<HANDLE, std::unique_ptr<Device>> m_devices;
    std::vector<BYTE> m_buffer;
    TouchpadSwipe m_swipe;
    QTimer m_quiet;
};

} // namespace

TouchpadSource *TouchpadSource::instance()
{
    static PrecisionTouchpad *source = qEnvironmentVariable("QOR_TOUCHPAD") == u"off" ? nullptr : new PrecisionTouchpad;
    return source;
}

} // namespace qor::input
