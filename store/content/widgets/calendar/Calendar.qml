// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit

// Calendar: this month at a glance, today in the accent colour. Weeks start
// on the locale's first day.
Item {
    id: calendar

    property date today: new Date()
    readonly property int year: today.getFullYear()
    readonly property int month: today.getMonth()
    // JavaScript counts Sunday as 0; Qt.DayOfWeek counts it as 7.
    readonly property int firstDay: Qt.locale().firstDayOfWeek % 7
    readonly property int offset: (new Date(year, month, 1).getDay() - firstDay + 7) % 7
    readonly property int days: new Date(year, month + 1, 0).getDate()
    readonly property real cell: (width - 36) / 7

    Timer {
        interval: 60000
        running: true
        repeat: true
        onTriggered: calendar.today = new Date()
    }

    Text {
        id: title
        x: 18
        y: 16
        text: Qt.locale().standaloneMonthName(calendar.month) + "  "
        color: Theme.textPrimary
        font: Theme.title3
        Text {
            anchors.left: parent.right
            anchors.baseline: parent.baseline
            text: calendar.year
            color: Theme.textTertiary
            font: Theme.title3
        }
    }

    Row {
        id: weekdays
        x: 18
        y: title.y + title.height + 12
        Repeater {
            model: 7
            Text {
                required property int index
                width: calendar.cell
                horizontalAlignment: Text.AlignHCenter
                text: Qt.locale().dayName((calendar.firstDay + index) % 7, Locale.NarrowFormat)
                color: Theme.textTertiary
                font: Theme.caption
            }
        }
    }

    Grid {
        x: 18
        y: weekdays.y + weekdays.height + 6
        columns: 7
        Repeater {
            model: 42
            Item {
                id: day
                required property int index
                readonly property int number: index - calendar.offset + 1
                readonly property bool inMonth: number >= 1 && number <= calendar.days
                readonly property bool isToday: inMonth && number === calendar.today.getDate()
                width: calendar.cell
                height: Math.min(calendar.cell, (calendar.height - weekdays.y - weekdays.height - 22) / 6)

                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(parent.width, parent.height) - 6
                    height: width
                    radius: width / 2
                    color: Theme.accent
                    visible: day.isToday
                }
                Text {
                    anchors.centerIn: parent
                    text: day.inMonth ? day.number : ""
                    color: day.isToday ? Theme.onAccent : Theme.textSecondary
                    font: day.isToday ? Theme.bodyStrong : Theme.body
                }
            }
        }
    }
}
