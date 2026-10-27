// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit
import QOR.Services
import QOR.Apps

// Notification banners (top right) and the session notification history.
// Banners are suppressed — but still recorded — during Focus, on the lock
// screen and while Control Center is open.
Item {
    id: stack

    property Backdrop backdrop
    property bool suppressed: false
    readonly property alias history: historyModel

    signal activated(string appId)

    ListModel { id: banners }
    ListModel { id: historyModel }

    Connections {
        target: Services.notifications
        function onPosted(id: int, appId: string, title: string, body: string, icon: string) {
            const entry = { nid: id, appId: appId, title: title, body: body, icon: icon, time: new Date().getTime() };
            historyModel.insert(0, entry);
            if (historyModel.count > 30)
                historyModel.remove(historyModel.count - 1);
            if (stack.suppressed || Services.notifications.doNotDisturb)
                return;
            banners.insert(0, entry);
            if (banners.count > 4)
                banners.remove(banners.count - 1);
        }
    }

    function removeBanner(nid: int) {
        for (let i = 0; i < banners.count; ++i) {
            if (banners.get(i).nid === nid) {
                banners.remove(i);
                return;
            }
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        model: banners
        spacing: 10
        interactive: false
        opacity: stack.suppressed ? 0 : 1
        Behavior on opacity { NumberAnimation { duration: 180 } }

        displaced: Transition {
            NumberAnimation { properties: "y"; duration: 260; easing.type: Easing.OutCubic }
        }

        delegate: NotificationCard {
            required property int nid
            required property string appId
            required property string title
            required property string body
            required property string icon
            required property real time
            width: list.width
            backdrop: stack.backdrop
            notificationId: nid
            app: appId
            heading: title
            message: body
            glyph: icon
            postedAt: time
            onActivated: {
                stack.activated(appId);
                stack.removeBanner(nid);
            }
            onExpired: stack.removeBanner(nid)
        }
    }
}
