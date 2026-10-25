// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// Windows controller backends:
//  - XInput: Xbox controllers and everything that speaks XInput (SteelSeries
//    Stratus Duo, 8BitDo in X-input mode, Steam Input's virtual pads).
//  - HID: Sony DualShock 4 and DualSense over USB and Bluetooth (with rumble
//    and the light bar), and any other HID game pad, read directly.

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

#include <cfgmgr32.h>
#include <hidpi.h>
#include <setupapi.h>
#include <xinput.h>

#include "backends.h"
#include "mapping.h"
#include "playstation.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>

#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcHid, "qtos.input.hid")

namespace qtos::input {

namespace {

// ── HID enumeration ─────────────────────────────────────────────────────────

struct HidDevice
{
    QString path;
    quint16 vendor = 0;
    quint16 product = 0;
    QString productName;
    USAGE usagePage = 0;
    USAGE usage = 0;
    int inputLength = 0;
    int outputLength = 0;
    bool bluetooth = false;
    bool xinput = false; // an "IG_" interface: XInput owns it
};

QList<HidDevice> enumerateHid()
{
    QList<HidDevice> devices;
    GUID guid;
    HidD_GetHidGuid(&guid);
    HDEVINFO set = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE)
        return devices;
    SP_DEVICE_INTERFACE_DATA iface {};
    iface.cbSize = sizeof(iface);
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(set, nullptr, &guid, i, &iface); ++i) {
        DWORD size = 0;
        SetupDiGetDeviceInterfaceDetailW(set, &iface, nullptr, 0, &size, nullptr);
        if (size == 0)
            continue;
        std::vector<char> buffer(size);
        auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(set, &iface, detail, size, nullptr, nullptr))
            continue;

        HidDevice device;
        device.path = QString::fromWCharArray(detail->DevicePath);
        const QString lower = device.path.toLower();
        device.xinput = lower.contains(u"ig_"_s);
        device.bluetooth = lower.contains(u"{00001124-0000-1000-8000-00805f9b34fb}"_s)
                           || lower.contains(u"{00001812-0000-1000-8000-00805f9b34fb}"_s);

        // No access is needed for attributes, strings and capabilities.
        HANDLE handle =
            CreateFileW(detail->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            continue;
        HIDD_ATTRIBUTES attributes {};
        attributes.Size = sizeof(attributes);
        if (HidD_GetAttributes(handle, &attributes)) {
            device.vendor = attributes.VendorID;
            device.product = attributes.ProductID;
        }
        wchar_t name[128] {};
        if (HidD_GetProductString(handle, name, sizeof(name)))
            device.productName = QString::fromWCharArray(name).trimmed();
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        if (HidD_GetPreparsedData(handle, &preparsed)) {
            HIDP_CAPS caps {};
            if (HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS) {
                device.usagePage = caps.UsagePage;
                device.usage = caps.Usage;
                device.inputLength = caps.InputReportByteLength;
                device.outputLength = caps.OutputReportByteLength;
            }
            HidD_FreePreparsedData(preparsed);
        }
        CloseHandle(handle);
        devices.append(device);
    }
    SetupDiDestroyDeviceInfoList(set);
    return devices;
}

bool isGamePad(const HidDevice &device) { return device.usagePage == 0x01 && device.usage == 0x05; }

// ── XInput ──────────────────────────────────────────────────────────────────

class XInputBackend : public Backend
{
public:
    QString name() const override { return u"xinput"_s; }

    ~XInputBackend() override
    {
        if (m_library)
            FreeLibrary(m_library);
    }

    void start() override
    {
        m_library = LoadLibraryW(L"xinput1_4.dll");
        if (!m_library)
            m_library = LoadLibraryW(L"xinput9_1_0.dll");
        if (!m_library)
            return;
        m_getState = reinterpret_cast<GetState>(GetProcAddress(m_library, "XInputGetState"));
        // Ordinal 100, XInputGetStateEx, also reports the Guide button.
        m_getStateEx = reinterpret_cast<GetState>(GetProcAddress(m_library, reinterpret_cast<LPCSTR>(100)));
        m_setState = reinterpret_cast<SetState>(GetProcAddress(m_library, "XInputSetState"));
        m_getCapabilities = reinterpret_cast<GetCapabilities>(GetProcAddress(m_library, "XInputGetCapabilities"));
        m_getBattery = reinterpret_cast<GetBattery>(GetProcAddress(m_library, "XInputGetBatteryInformation"));
        m_probe.start();
    }

    void poll() override
    {
        GetState get = m_getStateEx ? m_getStateEx : m_getState;
        if (!get)
            return;
        // Querying an empty slot is slow, so empty slots are probed once a
        // second.
        const bool probe = m_probe.elapsed() >= 1000;
        if (probe)
            m_probe.restart();
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
            Slot &slot = m_slots[i];
            if (!slot.connected && !probe)
                continue;
            XINPUT_STATE state {};
            if (get(i, &state) != ERROR_SUCCESS) {
                if (slot.connected) {
                    slot.connected = false;
                    Q_EMIT removed(id(i));
                }
                continue;
            }
            if (!slot.connected) {
                slot.connected = true;
                slot.packet = ~0u;
                slot.info = describe(i);
                slot.batteryCheck.invalidate();
                Q_EMIT added(slot.info);
            }
            if (state.dwPacketNumber != slot.packet) {
                slot.packet = state.dwPacketNumber;
                XInputPad pad;
                pad.buttons = state.Gamepad.wButtons;
                pad.leftTrigger = state.Gamepad.bLeftTrigger;
                pad.rightTrigger = state.Gamepad.bRightTrigger;
                pad.thumbLX = state.Gamepad.sThumbLX;
                pad.thumbLY = state.Gamepad.sThumbLY;
                pad.thumbRX = state.Gamepad.sThumbRX;
                pad.thumbRY = state.Gamepad.sThumbRY;
                Q_EMIT changed(id(i), mapXInput(pad));
            }
            if (!slot.batteryCheck.isValid() || slot.batteryCheck.elapsed() > 30000) {
                slot.batteryCheck.start();
                updateBattery(i);
            }
        }
    }

    void setRumble(const QString &padId, float strong, float weak) override
    {
        const DWORD i = padId.mid(7).toUInt();
        if (!m_setState || i >= XUSER_MAX_COUNT || !m_slots[i].connected)
            return;
        XINPUT_VIBRATION vibration { WORD(strong * 65535.0f), WORD(weak * 65535.0f) };
        m_setState(i, &vibration);
    }

private:
    using GetState = DWORD(WINAPI *)(DWORD, XINPUT_STATE *);
    using SetState = DWORD(WINAPI *)(DWORD, XINPUT_VIBRATION *);
    using GetCapabilities = DWORD(WINAPI *)(DWORD, DWORD, XINPUT_CAPABILITIES *);
    using GetBattery = DWORD(WINAPI *)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION *);

    struct Slot
    {
        bool connected = false;
        DWORD packet = 0;
        GamepadInfo info;
        QElapsedTimer batteryCheck;
    };

    static QString id(DWORD slot) { return u"xinput:"_s + QString::number(slot); }

    GamepadInfo describe(DWORD i)
    {
        GamepadInfo info;
        info.id = id(i);
        info.family = Family::Xbox;
        info.rumble = m_setState != nullptr;
        info.name = u"Xbox Controller"_s;
        info.connection = u"USB"_s;
        if (m_getCapabilities) {
            XINPUT_CAPABILITIES caps {};
            if (m_getCapabilities(i, XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS
                && (caps.Flags & XINPUT_CAPS_WIRELESS))
                info.connection = u"Wireless"_s;
        }
        // XInput has no names. When exactly one XInput HID interface is not
        // yet claimed, it is this controller: take its product name.
        QStringList claimed;
        for (const Slot &slot : m_slots)
            if (slot.connected && !slot.info.id.isEmpty() && slot.info.id != info.id)
                claimed.append(slot.info.name);
        QList<HidDevice> candidates;
        for (const HidDevice &device : enumerateHid())
            if (device.xinput && isGamePad(device) && !claimed.contains(device.productName))
                candidates.append(device);
        if (candidates.size() == 1) {
            const HidDevice &device = candidates.constFirst();
            if (!device.productName.isEmpty())
                info.name = device.productName;
            info.vendor = device.vendor;
            info.product = device.product;
            if (device.bluetooth)
                info.connection = u"Bluetooth"_s;
        }
        return info;
    }

    void updateBattery(DWORD i)
    {
        if (!m_getBattery)
            return;
        XINPUT_BATTERY_INFORMATION battery {};
        if (m_getBattery(i, BATTERY_DEVTYPE_GAMEPAD, &battery) != ERROR_SUCCESS)
            return;
        GamepadInfo info = m_slots[i].info;
        if (battery.BatteryType == BATTERY_TYPE_WIRED || battery.BatteryType == BATTERY_TYPE_DISCONNECTED) {
            info.battery = -1;
        } else {
            static const int levels[] = { 5, 30, 65, 100 };
            info.battery = levels[std::min<int>(battery.BatteryLevel, 3)];
        }
        if (info != m_slots[i].info) {
            m_slots[i].info = info;
            Q_EMIT infoChanged(info);
        }
    }

    HMODULE m_library = nullptr;
    GetState m_getState = nullptr;
    GetState m_getStateEx = nullptr;
    SetState m_setState = nullptr;
    GetCapabilities m_getCapabilities = nullptr;
    GetBattery m_getBattery = nullptr;
    Slot m_slots[XUSER_MAX_COUNT];
    QElapsedTimer m_probe;
};

// ── HID ─────────────────────────────────────────────────────────────────────

class HidBackend : public Backend
{
public:
    QString name() const override { return u"hid"_s; }

    ~HidBackend() override
    {
        if (m_notification)
            CM_Unregister_Notification(m_notification);
        for (auto &[path, device] : m_devices)
            close(*device);
    }

    void start() override
    {
        GUID guid;
        HidD_GetHidGuid(&guid);
        CM_NOTIFY_FILTER filter {};
        filter.cbSize = sizeof(filter);
        filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
        filter.u.DeviceInterface.ClassGuid = guid;
        CM_Register_Notification(&filter, this, &HidBackend::notified, &m_notification);
        m_scanned.start();
    }

    void poll() override
    {
        // Rescan on arrival/removal notifications, and every few seconds as
        // a safety net.
        if (m_rescan.exchange(false) || m_scanned.elapsed() > 5000) {
            m_scanned.restart();
            scan();
        }
        for (auto it = m_devices.begin(); it != m_devices.end();) {
            Device &device = *it->second;
            if (device.gone) {
                const QString padId = device.info.id;
                close(device);
                it = m_devices.erase(it);
                Q_EMIT removed(padId);
                continue;
            }
            GamepadState latest;
            quint32 latched = 0;
            bool fresh = false;
            playstation::Battery battery;
            {
                std::lock_guard lock(device.lock);
                fresh = std::exchange(device.fresh, false);
                latest = device.latest;
                latched = std::exchange(device.latched, 0);
                battery = device.battery;
            }
            if (fresh) {
                // A press and release between two polls still counts.
                if (latched & ~latest.buttons) {
                    GamepadState press = latest;
                    press.buttons |= latched;
                    Q_EMIT changed(device.info.id, press);
                }
                Q_EMIT changed(device.info.id, latest);
            }
            if (battery.percent != device.info.battery || battery.charging != device.info.charging) {
                device.info.battery = battery.percent;
                device.info.charging = battery.charging;
                Q_EMIT infoChanged(device.info);
            }
            ++it;
        }
    }

    void setRumble(const QString &padId, float strong, float weak) override
    {
        if (Device *device = find(padId)) {
            device->strong = quint8(strong * 255.0f);
            device->weak = quint8(weak * 255.0f);
            writeOutput(*device);
        }
    }

    void setLight(const QString &padId, const QColor &color) override
    {
        if (Device *device = find(padId)) {
            device->light = color;
            writeOutput(*device);
        }
    }

private:
    struct Device
    {
        HidDevice hid;
        GamepadInfo info;
        std::optional<playstation::Device> playstation;
        HANDLE handle = INVALID_HANDLE_VALUE;
        HANDLE stop = nullptr;
        std::thread reader;
        std::atomic<bool> gone { false };

        // Generic HID layout.
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        int buttonCount = 0;
        std::vector<HIDP_VALUE_CAPS> values;

        std::mutex lock;
        GamepadState latest;
        quint32 latched = 0;
        bool fresh = false;
        playstation::Battery battery;

        quint8 strong = 0;
        quint8 weak = 0;
        QColor light;
        quint8 sequence = 0;
    };

    static DWORD CALLBACK notified(HCMNOTIFICATION, PVOID context, CM_NOTIFY_ACTION, PCM_NOTIFY_EVENT_DATA, DWORD)
    {
        static_cast<HidBackend *>(context)->m_rescan = true;
        return ERROR_SUCCESS;
    }

    Device *find(const QString &padId)
    {
        for (auto &[path, device] : m_devices)
            if (device->info.id == padId)
                return device.get();
        return nullptr;
    }

    void scan()
    {
        const QList<HidDevice> devices = enumerateHid();
        for (const HidDevice &hid : devices) {
            qCDebug(lcHid).nospace() << "HID " << Qt::hex << hid.vendor << ":" << hid.product << " usage "
                                     << hid.usagePage << "/" << hid.usage << Qt::dec << " " << hid.productName
                                     << (hid.xinput ? " (XInput)" : "") << (hid.bluetooth ? " (Bluetooth)" : "");
            if (hid.xinput || m_devices.count(hid.path))
                continue;
            const auto sony = playstation::identify(hid.vendor, hid.product);
            if (!sony && !isGamePad(hid))
                continue;
            if (sony && hid.usagePage != 0x01) // the gamepad collection only
                continue;
            open(hid, sony);
        }
    }

    void open(const HidDevice &hid, const std::optional<playstation::Device> &sony)
    {
        const std::wstring path = hid.path.toStdWString();
        HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED, nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            qCWarning(lcHid) << "Cannot open" << hid.productName << GetLastError();
            return;
        }

        auto device = std::make_unique<Device>();
        device->hid = hid;
        device->handle = handle;
        device->playstation = sony;
        device->light = QColor::fromRgb(0x6b, 0x5b, 0xff);
        device->info.id = u"hid:"_s + hid.path;
        device->info.vendor = hid.vendor;
        device->info.product = hid.product;
        device->info.connection = hid.bluetooth ? u"Bluetooth"_s : u"USB"_s;
        if (sony) {
            device->info.name = QString::fromLatin1(sony->name);
            device->info.family = Family::PlayStation;
            device->info.rumble = true;
            device->info.lightbar = true;
            if (hid.bluetooth) {
                // Switch the controller from its basic report to full reports.
                const auto request = playstation::fullReportRequest(sony->model);
                std::vector<char> feature(std::size_t(request.length), 0);
                feature[0] = char(request.id);
                HidD_GetFeature(handle, feature.data(), ULONG(feature.size()));
            }
        } else {
            device->info.name = hid.productName.isEmpty() ? u"Game Controller"_s : hid.productName;
            device->info.family = hid.vendor == 0x057e ? Family::Nintendo : Family::Generic;
            if (!HidD_GetPreparsedData(handle, &device->preparsed)) {
                CloseHandle(handle);
                return;
            }
            prepareGeneric(*device);
        }

        device->stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        Device *raw = device.get();
        device->reader = std::thread([raw] { readLoop(*raw); });
        const GamepadInfo info = device->info;
        m_devices.emplace(hid.path, std::move(device));
        Q_EMIT added(info);
    }

    static void prepareGeneric(Device &device)
    {
        HIDP_CAPS caps {};
        HidP_GetCaps(device.preparsed, &caps);
        std::vector<HIDP_BUTTON_CAPS> buttons(caps.NumberInputButtonCaps);
        USHORT count = caps.NumberInputButtonCaps;
        if (count && HidP_GetButtonCaps(HidP_Input, buttons.data(), &count, device.preparsed) == HIDP_STATUS_SUCCESS) {
            for (USHORT i = 0; i < count; ++i) {
                if (buttons[i].UsagePage != 0x09)
                    continue;
                const int last = buttons[i].IsRange ? buttons[i].Range.UsageMax : buttons[i].NotRange.Usage;
                device.buttonCount = std::max(device.buttonCount, last);
            }
        }
        device.values.resize(caps.NumberInputValueCaps);
        count = caps.NumberInputValueCaps;
        if (count
            && HidP_GetValueCaps(HidP_Input, device.values.data(), &count, device.preparsed) == HIDP_STATUS_SUCCESS)
            device.values.resize(count);
        else
            device.values.clear();
    }

    static void readLoop(Device &device)
    {
        std::vector<quint8> buffer(std::size_t(std::max(device.hid.inputLength, 64)));
        OVERLAPPED overlapped {};
        overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE waits[2] = { overlapped.hEvent, device.stop };
        for (;;) {
            ResetEvent(overlapped.hEvent);
            DWORD read = 0;
            if (!ReadFile(device.handle, buffer.data(), DWORD(buffer.size()), &read, &overlapped)) {
                if (GetLastError() != ERROR_IO_PENDING) {
                    device.gone = true;
                    break;
                }
                if (WaitForMultipleObjects(2, waits, FALSE, INFINITE) != WAIT_OBJECT_0) {
                    CancelIoEx(device.handle, &overlapped);
                    GetOverlappedResult(device.handle, &overlapped, &read, TRUE);
                    break;
                }
                if (!GetOverlappedResult(device.handle, &overlapped, &read, FALSE)) {
                    device.gone = true;
                    break;
                }
            }
            if (read == 0)
                continue;
            std::optional<GamepadState> state;
            playstation::Battery battery = device.battery;
            if (device.playstation)
                state = playstation::parseInput(device.playstation->model, buffer.data(), int(read), &battery);
            else
                state = parseGeneric(device, buffer.data(), int(read));
            if (!state)
                continue;
            std::lock_guard lock(device.lock);
            device.latest = *state;
            device.latched |= state->buttons;
            device.battery = battery;
            device.fresh = true;
        }
        CloseHandle(overlapped.hEvent);
    }

    static std::optional<GamepadState> parseGeneric(Device &device, quint8 *data, int size)
    {
        auto *report = reinterpret_cast<PCHAR>(data);
        GenericHidReport parsed;
        parsed.buttonCount = device.buttonCount;
        std::vector<USAGE> usages(std::size_t(std::max(device.buttonCount, 1) + 8));
        ULONG count = ULONG(usages.size());
        const NTSTATUS status =
            HidP_GetUsages(HidP_Input, 0x09, 0, usages.data(), &count, device.preparsed, report, ULONG(size));
        if (status == HIDP_STATUS_INCOMPATIBLE_REPORT_ID)
            return std::nullopt;
        if (status == HIDP_STATUS_SUCCESS)
            for (ULONG i = 0; i < count; ++i)
                parsed.pressed.append(usages[i]);

        for (const HIDP_VALUE_CAPS &cap : device.values) {
            const USAGE usage = cap.IsRange ? cap.Range.UsageMin : cap.NotRange.Usage;
            ULONG raw = 0;
            if (HidP_GetUsageValue(HidP_Input, cap.UsagePage, cap.LinkCollection, usage, &raw, device.preparsed, report,
                                   ULONG(size))
                != HIDP_STATUS_SUCCESS)
                continue;
            long value = long(raw);
            long minimum = cap.LogicalMin;
            long maximum = cap.LogicalMax;
            if (minimum < 0 && cap.BitSize > 0 && cap.BitSize < 32 && (raw & (1ul << (cap.BitSize - 1))))
                value = long(raw) - (1l << cap.BitSize); // sign-extend
            if (maximum <= minimum)
                maximum = (1l << std::min<int>(cap.BitSize, 30)) - 1;
            const float t = float(value - minimum) / float(maximum - minimum);
            const float centred = std::clamp(t * 2.0f - 1.0f, -1.0f, 1.0f);
            if (cap.UsagePage == 0x01) {
                switch (usage) {
                case 0x30: parsed.x = centred; break;
                case 0x31: parsed.y = centred; break;
                case 0x32: parsed.z = centred; break;
                case 0x33: parsed.rx = centred; break;
                case 0x34: parsed.ry = centred; break;
                case 0x35: parsed.rz = centred; break;
                case 0x39: {
                    const long hat = value - minimum;
                    parsed.hat = (hat >= 0 && hat <= 7 && value <= cap.LogicalMax) ? int(hat) : -1;
                    break;
                }
                default: break;
                }
            } else if (cap.UsagePage == 0x02) {
                if (usage == 0xc5)
                    parsed.brake = std::clamp(t, 0.0f, 1.0f);
                else if (usage == 0xc4)
                    parsed.accelerator = std::clamp(t, 0.0f, 1.0f);
            }
        }
        return mapGenericHid(parsed);
    }

    void writeOutput(Device &device)
    {
        if (!device.playstation || device.handle == INVALID_HANDLE_VALUE)
            return;
        QByteArray report = playstation::outputReport(device.playstation->model, device.hid.bluetooth, device.strong,
                                                      device.weak, device.light, device.sequence++ & 0x0f);
        // Windows wants the collection's full output length.
        if (device.hid.outputLength > report.size())
            report.resize(device.hid.outputLength, '\0');
        OVERLAPPED overlapped {};
        overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        DWORD written = 0;
        if (!WriteFile(device.handle, report.constData(), DWORD(report.size()), &written, &overlapped)
            && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(overlapped.hEvent, 50) != WAIT_OBJECT_0)
                CancelIoEx(device.handle, &overlapped);
            GetOverlappedResult(device.handle, &overlapped, &written, TRUE);
        }
        CloseHandle(overlapped.hEvent);
    }

    static void close(Device &device)
    {
        if (device.stop)
            SetEvent(device.stop);
        if (device.handle != INVALID_HANDLE_VALUE)
            CancelIoEx(device.handle, nullptr);
        if (device.reader.joinable())
            device.reader.join();
        if (device.handle != INVALID_HANDLE_VALUE)
            CloseHandle(device.handle);
        if (device.stop)
            CloseHandle(device.stop);
        if (device.preparsed)
            HidD_FreePreparsedData(device.preparsed);
        device.handle = INVALID_HANDLE_VALUE;
        device.stop = nullptr;
        device.preparsed = nullptr;
    }

    std::map<QString, std::unique_ptr<Device>> m_devices;
    std::atomic<bool> m_rescan { true };
    HCMNOTIFICATION m_notification = nullptr;
    QElapsedTimer m_scanned;
};

} // namespace

std::vector<std::unique_ptr<Backend>> createHardwareBackends()
{
    std::vector<std::unique_ptr<Backend>> backends;
    backends.push_back(std::make_unique<XInputBackend>());
    backends.push_back(std::make_unique<HidBackend>());
    return backends;
}

} // namespace qtos::input
