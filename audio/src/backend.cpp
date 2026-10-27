// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "backend.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcBackend, "qor.audio.backend")

namespace qor::audio {

void configureBackend()
{
#if defined(Q_OS_LINUX)
    if (qEnvironmentVariableIsSet("QT_AUDIO_BACKEND"))
        return;
    // Where libpipewire looks for the daemon: $PIPEWIRE_REMOTE (a name or a
    // path, default "pipewire-0") in $PIPEWIRE_RUNTIME_DIR or $XDG_RUNTIME_DIR.
    const QString remote = qEnvironmentVariable("PIPEWIRE_REMOTE", u"pipewire-0"_s);
    QString runtime = qEnvironmentVariable("PIPEWIRE_RUNTIME_DIR");
    if (runtime.isEmpty())
        runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    const QString socket = QDir::isAbsolutePath(remote) ? remote : runtime + u'/' + remote;
    if ((QDir::isAbsolutePath(remote) || !runtime.isEmpty()) && QFileInfo::exists(socket))
        return;
    qCInfo(lcBackend) << "No PipeWire daemon; using the PulseAudio backend";
    qputenv("QT_AUDIO_BACKEND", "pulseaudio");
#endif
}

} // namespace qor::audio
