// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// Linux controller backend: every game pad the kernel supports (xpad for
// Xbox controllers, hid-playstation for DualShock 4 and DualSense, hid-generic
// and friends for the rest), read through evdev. Devices appear and disappear
// with /dev/input; access comes from logind's uaccess rules for joysticks.

#include "backends.h"
#include "mapping.h"

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTimer>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <map>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcEvdev, "qtos.input.evdev")

namespace qtos::input {

namespace {

bool testBit(const unsigned long *bits, int bit)
{
    constexpr int perLong = int(sizeof(unsigned long) * 8);
    return (bits[bit / perLong] >> (bit % perLong)) & 1ul;
}

class EvdevBackend : public Backend
{
public:
    QString name() const override { return u"evdev"_s; }

    ~EvdevBackend() override
    {
        for (auto &[path, device] : m_devices)
            ::close(device->fd);
    }

    void start() override
    {
        // /dev/input itself only exists once some input device does, so /dev
        // is watched too, for its creation.
        m_watcher.addPath(u"/dev"_s);
        if (QFileInfo::exists(u"/dev/input"_s))
            m_watcher.addPath(u"/dev/input"_s);
        // New nodes get their permissions a moment after they appear.
        m_rescan.setSingleShot(true);
        m_rescan.setInterval(300);
        connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path) {
            if (path == u"/dev"_s) {
                if (!QFileInfo::exists(u"/dev/input"_s) || m_watcher.directories().contains(u"/dev/input"_s))
                    return;
                m_watcher.addPath(u"/dev/input"_s);
            }
            m_rescan.start();
        });
        connect(&m_rescan, &QTimer::timeout, this, [this] { scan(); });
        scan();
        m_battery.start();
    }

    void poll() override
    {
        if (m_battery.elapsed() < 30000)
            return;
        m_battery.restart();
        for (auto &[path, device] : m_devices)
            readBattery(*device);
    }

    void setRumble(const QString &id, float strong, float weak) override
    {
        Device *device = find(id);
        if (!device || !device->info.rumble)
            return;
        ff_effect effect {};
        effect.type = FF_RUMBLE;
        effect.id = device->effect;
        effect.u.rumble.strong_magnitude = quint16(strong * 65535.0f);
        effect.u.rumble.weak_magnitude = quint16(weak * 65535.0f);
        effect.replay.length = 2000; // the manager stops it sooner
        if (ioctl(device->fd, EVIOCSFF, &effect) < 0)
            return;
        device->effect = effect.id;
        input_event play {};
        play.type = EV_FF;
        play.code = quint16(effect.id);
        play.value = (strong > 0 || weak > 0) ? 1 : 0;
        [[maybe_unused]] const auto written = ::write(device->fd, &play, sizeof(play));
    }

private:
    struct Device
    {
        int fd = -1;
        QString path;
        GamepadInfo info;
        std::unique_ptr<EvdevMapper> mapper;
        std::unique_ptr<QSocketNotifier> notifier;
        QString batteryPath;
        short effect = -1;
    };

    Device *find(const QString &id)
    {
        for (auto &[path, device] : m_devices)
            if (device->info.id == id)
                return device.get();
        return nullptr;
    }

    void scan()
    {
        const QStringList nodes = QDir(u"/dev/input"_s).entryList({ u"event*"_s }, QDir::System);
        for (const QString &node : nodes) {
            const QString path = u"/dev/input/"_s + node;
            if (!m_devices.count(path))
                open(path);
        }
        // Removed nodes are noticed by their reads failing.
    }

    void open(const QString &path)
    {
        const int fd = ::open(QFile::encodeName(path).constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
        const int readOnly =
            fd >= 0 ? fd : ::open(QFile::encodeName(path).constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (readOnly < 0)
            return; // no permission, or not ours
        unsigned long keys[(KEY_MAX + 1) / (sizeof(unsigned long) * 8) + 1] {};
        if (ioctl(readOnly, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) < 0 || !testBit(keys, BTN_SOUTH)) {
            ::close(readOnly); // a keyboard, mouse, touchpad or joystick
            return;
        }

        auto device = std::make_unique<Device>();
        device->fd = readOnly;
        device->path = path;
        char name[256] {};
        ioctl(readOnly, EVIOCGNAME(sizeof(name) - 1), name);
        input_id identity {};
        ioctl(readOnly, EVIOCGID, &identity);
        device->info.id = u"evdev:"_s + path;
        device->info.name = QString::fromUtf8(name).trimmed();
        device->info.vendor = identity.vendor;
        device->info.product = identity.product;
        device->info.connection = identity.bustype == BUS_BLUETOOTH ? u"Bluetooth"_s
                                  : identity.bustype == BUS_USB     ? u"USB"_s
                                                                    : u"Wireless"_s;
        const QString lowerName = device->info.name.toLower();
        const bool xbox = identity.vendor == 0x045e || lowerName.contains(u"xbox"_s) || lowerName.contains(u"x-box"_s);
        device->info.family = xbox                        ? Family::Xbox
                              : identity.vendor == 0x054c ? Family::PlayStation
                              : identity.vendor == 0x057e ? Family::Nintendo
                                                          : Family::Generic;
        unsigned long features[(FF_MAX + 1) / (sizeof(unsigned long) * 8) + 1] {};
        device->info.rumble = fd >= 0 && ioctl(readOnly, EVIOCGBIT(EV_FF, sizeof(features)), features) >= 0
                              && testBit(features, FF_RUMBLE);

        std::array<AbsRange, evdev::AbsCount> ranges {};
        unsigned long axes[(ABS_MAX + 1) / (sizeof(unsigned long) * 8) + 1] {};
        if (ioctl(readOnly, EVIOCGBIT(EV_ABS, sizeof(axes)), axes) >= 0) {
            for (int code = 0; code < evdev::AbsCount; ++code) {
                if (!testBit(axes, code))
                    continue;
                input_absinfo info {};
                if (ioctl(readOnly, EVIOCGABS(code), &info) >= 0)
                    ranges[std::size_t(code)] = { info.minimum, info.maximum };
            }
        }
        // xpad reports Xbox X and Y as BTN_X/BTN_Y: north and west swapped.
        device->mapper = std::make_unique<EvdevMapper>(ranges, xbox);
        device->batteryPath = findBattery(path);

        Device *raw = device.get();
        device->notifier = std::make_unique<QSocketNotifier>(readOnly, QSocketNotifier::Read);
        connect(device->notifier.get(), &QSocketNotifier::activated, this, [this, raw] { read(*raw); });
        readBattery(*device);
        const GamepadInfo info = device->info;
        m_devices.emplace(path, std::move(device));
        qCInfo(lcEvdev) << "Game pad" << info.name << "at" << path;
        Q_EMIT added(info);
    }

    void read(Device &device)
    {
        input_event events[64];
        for (;;) {
            const ssize_t bytes = ::read(device.fd, events, sizeof(events));
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    return;
                remove(device.path); // ENODEV: unplugged
                return;
            }
            if (bytes == 0)
                return;
            const int count = int(bytes / ssize_t(sizeof(input_event)));
            for (int i = 0; i < count; ++i)
                if (device.mapper->handle(events[i].type, events[i].code, events[i].value))
                    Q_EMIT changed(device.info.id, device.mapper->state());
        }
    }

    void remove(const QString &path)
    {
        auto it = m_devices.find(path);
        if (it == m_devices.end())
            return;
        const QString id = it->second->info.id;
        it->second->notifier->setEnabled(false);
        ::close(it->second->fd);
        // Deleting the notifier from its own signal is not allowed.
        it->second->notifier.release()->deleteLater();
        m_devices.erase(it);
        Q_EMIT removed(id);
    }

    // /sys/class/input/eventN/device/device/power_supply/*/capacity
    static QString findBattery(const QString &path)
    {
        const QString event = path.section(u'/', -1);
        QDir supplies(u"/sys/class/input/"_s + event + u"/device/device/power_supply"_s);
        const QStringList entries = supplies.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        return entries.isEmpty() ? QString() : supplies.filePath(entries.constFirst());
    }

    void readBattery(Device &device)
    {
        if (device.batteryPath.isEmpty())
            return;
        QFile capacity(device.batteryPath + u"/capacity"_s);
        QFile status(device.batteryPath + u"/status"_s);
        if (!capacity.open(QIODevice::ReadOnly))
            return;
        GamepadInfo info = device.info;
        info.battery = capacity.readAll().trimmed().toInt();
        info.charging = status.open(QIODevice::ReadOnly) && status.readAll().trimmed() == "Charging";
        if (info != device.info) {
            device.info = info;
            Q_EMIT infoChanged(info);
        }
    }

    QFileSystemWatcher m_watcher;
    QTimer m_rescan;
    QElapsedTimer m_battery;
    std::map<QString, std::unique_ptr<Device>> m_devices;
};

} // namespace

std::vector<std::unique_ptr<Backend>> createHardwareBackends()
{
    std::vector<std::unique_ptr<Backend>> backends;
    backends.push_back(std::make_unique<EvdevBackend>());
    return backends;
}

} // namespace qtos::input
