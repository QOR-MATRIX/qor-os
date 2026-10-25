#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
"""Creates a kernel game pad with uinput and plays a few inputs.

Exercises QTOS's Linux controller backend (evdev) end to end without
hardware, including hotplug. Needs write access to /dev/uinput (root):

    sudo python3 tools/dev/uinput-gamepad.py &
    qtos-gamepads --seconds 6      # as root too: the node is root-only
"""
import fcntl
import os
import struct
import time

UI_SET_EVBIT, UI_SET_KEYBIT, UI_SET_ABSBIT = 0x40045564, 0x40045565, 0x40045567
UI_DEV_CREATE, UI_DEV_DESTROY = 0x5501, 0x5502
EV_SYN, EV_KEY, EV_ABS = 0, 1, 3
BTN = {'south': 0x130, 'east': 0x131, 'north': 0x133, 'west': 0x134, 'tl': 0x136, 'tr': 0x137,
       'select': 0x13a, 'start': 0x13b, 'mode': 0x13c, 'thumbl': 0x13d, 'thumbr': 0x13e}
ABS = {'x': 0, 'y': 1, 'z': 2, 'rx': 3, 'ry': 4, 'rz': 5, 'hat0x': 0x10, 'hat0y': 0x11}

fd = os.open('/dev/uinput', os.O_WRONLY | os.O_NONBLOCK)
fcntl.ioctl(fd, UI_SET_EVBIT, EV_KEY)
fcntl.ioctl(fd, UI_SET_EVBIT, EV_ABS)
for code in BTN.values():
    fcntl.ioctl(fd, UI_SET_KEYBIT, code)
for code in ABS.values():
    fcntl.ioctl(fd, UI_SET_ABSBIT, code)

absmax = [0] * 64
absmin = [0] * 64
for name in ('x', 'y', 'rx', 'ry'):
    absmin[ABS[name]], absmax[ABS[name]] = -32768, 32767
for name in ('z', 'rz'):
    absmin[ABS[name]], absmax[ABS[name]] = 0, 255
for name in ('hat0x', 'hat0y'):
    absmin[ABS[name]], absmax[ABS[name]] = -1, 1
name = b'QTOS Test Pad'.ljust(80, b'\0')
dev = name + struct.pack('<HHHH', 0x03, 0x1234, 0x5678, 1) + struct.pack('<I', 0)
dev += struct.pack('<64i', *absmax) + struct.pack('<64i', *absmin) + struct.pack('<64i', *([0] * 64)) * 2
os.write(fd, dev)
fcntl.ioctl(fd, UI_DEV_CREATE)
os.chmod('/dev/input', 0o755) if os.path.isdir('/dev/input') else None


def emit(kind, code, value):
    os.write(fd, struct.pack('llHHi', 0, 0, kind, code, value))


def sync():
    emit(EV_SYN, 0, 0)


time.sleep(2.5)  # the reader starts and finds us
for step in [('key', 'south', 1), ('key', 'south', 0), ('key', 'north', 1), ('key', 'north', 0),
             ('abs', 'x', 32767), ('abs', 'x', 0), ('abs', 'hat0x', 1), ('abs', 'hat0x', 0),
             ('abs', 'rz', 255), ('abs', 'rz', 0), ('key', 'start', 1), ('key', 'start', 0)]:
    kind, what, value = step
    emit(EV_KEY if kind == 'key' else EV_ABS, BTN[what] if kind == 'key' else ABS[what], value)
    sync()
    time.sleep(0.15)
time.sleep(1.0)
fcntl.ioctl(fd, UI_DEV_DESTROY)
os.close(fd)
