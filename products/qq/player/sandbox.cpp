// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "sandbox.h"

#include "confinement.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QUrl>
#include <QtGlobal>

#if defined(Q_OS_LINUX)

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/landlock.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

// Newer than some systems' headers; the kernel is asked which it knows.
#ifndef LANDLOCK_ACCESS_FS_REFER
#define LANDLOCK_ACCESS_FS_REFER (1ULL << 13)
#endif
#ifndef LANDLOCK_ACCESS_FS_TRUNCATE
#define LANDLOCK_ACCESS_FS_TRUNCATE (1ULL << 14)
#endif
#ifndef LANDLOCK_ACCESS_FS_IOCTL_DEV
#define LANDLOCK_ACCESS_FS_IOCTL_DEV (1ULL << 15)
#endif

namespace qq {

namespace {

constexpr quint64 readRights = LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR;

/// What writing means on the kernel's first version of Landlock, in which every later right is unknown to it.
constexpr quint64 writeRightsV1 = LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_REMOVE_DIR
                                  | LANDLOCK_ACCESS_FS_REMOVE_FILE | LANDLOCK_ACCESS_FS_MAKE_DIR
                                  | LANDLOCK_ACCESS_FS_MAKE_REG | LANDLOCK_ACCESS_FS_MAKE_SOCK
                                  | LANDLOCK_ACCESS_FS_MAKE_FIFO | LANDLOCK_ACCESS_FS_MAKE_SYM
                                  | LANDLOCK_ACCESS_FS_MAKE_CHAR | LANDLOCK_ACCESS_FS_MAKE_BLOCK;

/// One file or folder's rights in a ruleset. A path that is not there is left out: there is nothing to grant. The handle
/// stays open for as long as the Player runs: Landlock holds a rule by the file's inode, and on a file system that makes a
/// new inode for each lookup (9p, as WSL's Windows drives are) closing it would lose the rule with it.
void allow(int ruleset, const QString &path, quint64 rights, quint64 handled)
{
    const int fd = ::open(QFile::encodeName(path).constData(), O_PATH | O_CLOEXEC);
    if (fd < 0)
        return;
    struct stat st{};
    if (::fstat(fd, &st) == 0 && !S_ISDIR(st.st_mode))
        rights &= LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_READ_FILE
                  | LANDLOCK_ACCESS_FS_TRUNCATE | LANDLOCK_ACCESS_FS_IOCTL_DEV;
    landlock_path_beneath_attr rule{};
    rule.allowed_access = rights & handled;
    rule.parent_fd = fd;
    syscall(SYS_landlock_add_rule, ruleset, LANDLOCK_RULE_PATH_BENEATH, &rule, 0);
}

QString home()
{
    return QDir::homePath();
}

/// The person's own folder for a kind of thing, as the XDG variables say.
QString xdg(const char *variable, const char *fallback)
{
    const QByteArray set = qgetenv(variable);
    return set.isEmpty() ? home() + QLatin1Char('/') + QLatin1String(fallback) : QFile::decodeName(set);
}

bool raiseLandlock(const PlayerWall &wall)
{
    const int abi = int(syscall(SYS_landlock_create_ruleset, nullptr, 0, LANDLOCK_CREATE_RULESET_VERSION));
    if (abi < 1)
        return false;

    // Whatever this kernel knows how to refuse: reading, writing and running. A right it does not know would make it
    // refuse the whole ruleset.
    quint64 handled = readRights | writeRightsV1 | LANDLOCK_ACCESS_FS_EXECUTE;
    if (abi >= 2)
        handled |= LANDLOCK_ACCESS_FS_REFER;
    if (abi >= 3)
        handled |= LANDLOCK_ACCESS_FS_TRUNCATE;
    if (abi >= 5)
        handled |= LANDLOCK_ACCESS_FS_IOCTL_DEV;

    landlock_ruleset_attr attr{};
    attr.handled_access_fs = handled;
    const int ruleset = int(syscall(SYS_landlock_create_ruleset, &attr, sizeof(attr), 0));
    if (ruleset < 0)
        return false;

    quint64 writeRights = writeRightsV1;
    if (abi >= 2)
        writeRights |= LANDLOCK_ACCESS_FS_REFER;
    if (abi >= 3)
        writeRights |= LANDLOCK_ACCESS_FS_TRUNCATE;

    const auto reads = [&](const QString &path) { allow(ruleset, path, readRights, handled); };
    const auto writes = [&](const QString &path) { allow(ruleset, path, readRights | writeRights, handled); };

    // The game, the Player and Qt.
    if (!wall.package.isEmpty())
        reads(wall.package);
    const QFileInfo self(QFile::symLinkTarget(QStringLiteral("/proc/self/exe")));
    reads(self.absolutePath());
    // A build keeps its QML modules beside the Player's own folder.
    const QString above = QFileInfo(self.absolutePath()).absolutePath();
    if (!Confinement::tooWide(QUrl::fromLocalFile(above)))
        reads(above);
    reads(QLibraryInfo::path(QLibraryInfo::PrefixPath));
    // The system: libraries, drivers, fonts, certificates, and what a graphics driver asks of the kernel.
    for (const char *place : {"/usr", "/lib", "/lib64", "/lib32", "/bin", "/sbin", "/etc", "/opt", "/var/lib", "/var/cache",
                              "/nix", "/sys", "/proc", "/run"})
        reads(QLatin1String(place));
    // From a person's home, only what fonts, sound and the look of things need.
    const QString config = xdg("XDG_CONFIG_HOME", ".config");
    const QString cache = xdg("XDG_CACHE_HOME", ".cache");
    const QString data = xdg("XDG_DATA_HOME", ".local/share");
    for (const QString &place : {QString(data + QStringLiteral("/fonts")), QString(home() + QStringLiteral("/.fonts")),
                                 QString(config + QStringLiteral("/fontconfig")), QString(cache + QStringLiteral("/fontconfig")),
                                 QString(config + QStringLiteral("/pulse")), QString(home() + QStringLiteral("/.Xauthority"))})
        reads(place);
    if (!qEnvironmentVariableIsEmpty("XAUTHORITY"))
        reads(QFile::decodeName(qgetenv("XAUTHORITY")));

    // Devices: the graphics card and what games ask of it, and the null device.
    allow(ruleset, QStringLiteral("/dev"), readRights | LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_TRUNCATE
                                               | LANDLOCK_ACCESS_FS_IOCTL_DEV, handled);
    // The games' stores (store.h, under a name no game chooses), and a cache of the Player's own for shaders, where every driver is told to keep
    // them; what the command line names.
    for (const QString &own : {QString(data + QStringLiteral("/Demiurge/QQ/saves")),
                               QString(cache + QStringLiteral("/Demiurge/QQ Player")),
                               QString(cache + QStringLiteral("/qq-player"))}) {
        QDir().mkpath(own);
        writes(own);
    }
    writes(QStringLiteral("/dev/shm"));
    for (const QString &path : wall.writes) {
        // Created now, so the wall has a file to grant.
        QDir().mkpath(QFileInfo(path).absolutePath());
        if (!QFileInfo::exists(path)) {
            QFile made(path);
            if (!made.open(QIODevice::WriteOnly))
                continue;
        }
        writes(path);
    }
    for (const QString &path : wall.reads)
        reads(path);

    const bool raised = syscall(SYS_landlock_restrict_self, ruleset, 0) == 0;
    ::close(ruleset);
    return raised;
}

/// No socket but a local one. What is not a socket call, or is on another machine's calls, carries on.
bool raiseSeccomp()
{
#if defined(__x86_64__)
    const quint32 arch = AUDIT_ARCH_X86_64;
#elif defined(__aarch64__)
    const quint32 arch = AUDIT_ARCH_AARCH64;
#else
    const quint32 arch = 0;
    return false;
#endif
    sock_filter program[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, arch, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_socket, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, args[0])),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AF_UNIX, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EACCES),
    };
    sock_fprog filter{};
    filter.len = sizeof(program) / sizeof(program[0]);
    filter.filter = program;
    return syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &filter) == 0;
}

}  // namespace

QString raisePlayerWall(const PlayerWall &wall)
{
    if (qgetenv("QQ_PLAYER_SANDBOX") == "off") {
        qWarning("QQ_PLAYER_SANDBOX=off: this game plays behind QQ's own checks alone.");
        return {};
    }
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0)
        qWarning("The QQ Player could not give up new privileges: %d", errno);

    // Every driver keeps its shader cache where the wall lets it.
    const QString cache = xdg("XDG_CACHE_HOME", ".cache") + QStringLiteral("/qq-player");
    qputenv("MESA_SHADER_CACHE_DIR", QFile::encodeName(cache + QStringLiteral("/mesa")));
    qputenv("__GL_SHADER_DISK_CACHE_PATH", QFile::encodeName(cache + QStringLiteral("/nvidia")));

    const bool walled = raiseLandlock(wall);
    if (!walled)
        qWarning("This kernel has no Landlock: this game's files are walled by QQ's own checks alone.");
    const bool noNetwork = raiseSeccomp();
    if (!noNetwork)
        qWarning("The QQ Player could not refuse itself the network: only QQ's own checks do.");
    QString standing = walled ? QStringLiteral("landlock") : QString();
    if (noNetwork)
        standing += standing.isEmpty() ? QStringLiteral("seccomp") : QStringLiteral("+seccomp");
    return standing;
}

}  // namespace qq

#else

namespace qq {

QString raisePlayerWall(const PlayerWall &)
{
    return {};
}

}  // namespace qq

#endif
