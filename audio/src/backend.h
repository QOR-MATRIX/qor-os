// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

namespace qor::audio {

// Chooses a Qt Multimedia audio backend that can work here. Call before
// anything in the process uses Qt Multimedia (the engine, SoundEffect).
//
// Qt 6.12 on Linux picks its PipeWire backend whenever libpipewire can be
// loaded, and crashes creating it when no PipeWire daemon is running, as
// under WSLg, which serves PulseAudio. Without a PipeWire socket this asks for
// the PulseAudio backend instead. An explicit QT_AUDIO_BACKEND always wins.
void configureBackend();

} // namespace qor::audio
