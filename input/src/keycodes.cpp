// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "keycodes.h"

#include <QtCore/QHash>
#include <QtCore/qnamespace.h>

namespace qtos::input {

quint32 evdevKeyCode(int qtKey)
{
    static const QHash<int, quint32> codes = [] {
        QHash<int, quint32> map {
            { Qt::Key_Escape, 1 },
            { Qt::Key_Backspace, 14 },
            { Qt::Key_Tab, 15 },
            { Qt::Key_Backtab, 15 },
            { Qt::Key_Return, 28 },
            { Qt::Key_Enter, 28 },
            { Qt::Key_Space, 57 },
            { Qt::Key_Up, 103 },
            { Qt::Key_Left, 105 },
            { Qt::Key_Right, 106 },
            { Qt::Key_Down, 108 },
            { Qt::Key_Minus, 12 },
            { Qt::Key_Equal, 13 },
            { Qt::Key_Comma, 51 },
            { Qt::Key_Period, 52 },
            { Qt::Key_Slash, 53 },
            { Qt::Key_Apostrophe, 40 },
            { Qt::Key_Semicolon, 39 },
            { Qt::Key_Home, 102 },
            { Qt::Key_End, 107 },
            { Qt::Key_PageUp, 104 },
            { Qt::Key_PageDown, 109 },
            { Qt::Key_Delete, 111 },
            // Media and volume keys.
            { Qt::Key_VolumeMute, 113 },
            { Qt::Key_VolumeDown, 114 },
            { Qt::Key_VolumeUp, 115 },
            { Qt::Key_MediaNext, 163 },
            { Qt::Key_MediaTogglePlayPause, 164 },
            { Qt::Key_MediaPrevious, 165 },
            { Qt::Key_MediaStop, 166 },
            { Qt::Key_MediaPlay, 200 },
            { Qt::Key_MediaPause, 201 },
        };
        const char *rows[] = { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
        const quint32 rowStart[] = { 16, 30, 44 };
        for (int r = 0; r < 3; ++r)
            for (int i = 0; rows[r][i]; ++i)
                map.insert(Qt::Key_A + (rows[r][i] - 'a'), rowStart[r] + quint32(i));
        for (int d = 1; d <= 9; ++d)
            map.insert(Qt::Key_0 + d, quint32(d + 1));
        map.insert(Qt::Key_0, 11);
        return map;
    }();
    return codes.value(qtKey, 0);
}

} // namespace qtos::input
