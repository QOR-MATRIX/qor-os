// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "keycodes.h"

#include <QtCore/QHash>

namespace qtos::input {

namespace {

// Keys a US layout types with Shift, and the key that types them.
const QHash<int, int> &shiftedKeys()
{
    static const QHash<int, int> keys {
        { Qt::Key_Exclam, Qt::Key_1 },
        { Qt::Key_At, Qt::Key_2 },
        { Qt::Key_NumberSign, Qt::Key_3 },
        { Qt::Key_Dollar, Qt::Key_4 },
        { Qt::Key_Percent, Qt::Key_5 },
        { Qt::Key_AsciiCircum, Qt::Key_6 },
        { Qt::Key_Ampersand, Qt::Key_7 },
        { Qt::Key_Asterisk, Qt::Key_8 },
        { Qt::Key_ParenLeft, Qt::Key_9 },
        { Qt::Key_ParenRight, Qt::Key_0 },
        { Qt::Key_Underscore, Qt::Key_Minus },
        { Qt::Key_Plus, Qt::Key_Equal },
        { Qt::Key_BraceLeft, Qt::Key_BracketLeft },
        { Qt::Key_BraceRight, Qt::Key_BracketRight },
        { Qt::Key_Bar, Qt::Key_Backslash },
        { Qt::Key_Colon, Qt::Key_Semicolon },
        { Qt::Key_QuoteDbl, Qt::Key_Apostrophe },
        { Qt::Key_AsciiTilde, Qt::Key_QuoteLeft },
        { Qt::Key_Less, Qt::Key_Comma },
        { Qt::Key_Greater, Qt::Key_Period },
        { Qt::Key_Question, Qt::Key_Slash },
    };
    return keys;
}

} // namespace

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
            { Qt::Key_BracketLeft, 26 },
            { Qt::Key_BracketRight, 27 },
            { Qt::Key_Semicolon, 39 },
            { Qt::Key_Apostrophe, 40 },
            { Qt::Key_QuoteLeft, 41 },
            { Qt::Key_Backslash, 43 },
            { Qt::Key_Comma, 51 },
            { Qt::Key_Period, 52 },
            { Qt::Key_Slash, 53 },
            { Qt::Key_Home, 102 },
            { Qt::Key_End, 107 },
            { Qt::Key_PageUp, 104 },
            { Qt::Key_PageDown, 109 },
            { Qt::Key_Delete, 111 },
            // Modifiers (the left-hand ones).
            { Qt::Key_Control, 29 },
            { Qt::Key_Shift, 42 },
            { Qt::Key_Alt, 56 },
            { Qt::Key_Meta, 125 },
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
        for (int f = 0; f < 10; ++f)
            map.insert(Qt::Key_F1 + f, quint32(59 + f));
        map.insert(Qt::Key_F11, 87);
        map.insert(Qt::Key_F12, 88);
        return map;
    }();
    return codes.value(shiftedKeys().value(qtKey, qtKey), 0);
}

bool needsShift(int qtKey) { return shiftedKeys().contains(qtKey); }

} // namespace qtos::input
