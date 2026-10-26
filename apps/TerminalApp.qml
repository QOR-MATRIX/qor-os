// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Services

// Terminal: a GPU-rendered QML console. In simulation it runs a built-in
// command set wired to the QTOS services; on device it hosts a PTY.
FocusScope {
    id: app

    property real topInset: 40
    property var launchArgs: ({})

    readonly property string user: Services.settings.userName.split(" ")[0].toLowerCase()
    readonly property string host: Services.settings.hostName
    property var history: []
    property int historyIndex: -1

    ListModel { id: lines }

    function writeLine(text: string, kind: string) {
        for (const line of text.split("\n"))
            lines.append({ line: line, kind: kind ?? "out" });
        output.positionViewAtEnd();
    }

    function prompt(): string {
        return app.user + "@" + app.host + " ~ ❯";
    }

    function run(command: string) {
        writeLine(prompt() + " " + command, "cmd");
        const parts = command.trim().split(/\s+/);
        const name = parts[0];
        const args = parts.slice(1);
        if (command.trim().length > 0) {
            history = history.concat([command]);
            historyIndex = history.length;
        }
        switch (name) {
        case "":
            break;
        case "help":
            writeLine(["Built-in commands:",
                   "  qtfetch               system summary",
                   "  ls                    list the home directory",
                   "  theme dark|light      switch appearance (qtos-settingsd)",
                   "  accent <hue>          set the accent hue, 0-360",
                   "  wifi on|off|status    control qtos-netd",
                   "  play | pause | next   control the media session",
                   "  notify <text>         post a notification (qtos-notifyd)",
                   "  open <app>            open an app (files, music, notes, monitor, settings)",
                   "  lock                  lock the session",
                   "  date | whoami | uname | echo | history | clear"].join("\n"), "dim");
            break;
        case "clear":
            lines.clear();
            break;
        case "echo":
            writeLine(args.join(" "), "out");
            break;
        case "date":
            writeLine(new Date().toString(), "out");
            break;
        case "whoami":
            writeLine(app.user, "out");
            break;
        case "uname":
            writeLine("QTOS " + Kit.version + " prism-simulation x86_64 (Qt Quick, Lumen)", "out");
            break;
        case "history":
            writeLine(history.map((h, i) => String(i + 1).padStart(4) + "  " + h).join("\n"), "dim");
            break;
        case "ls":
            writeLine("Documents   Downloads   Music   Pictures   Projects   .config", "accent");
            break;
        case "qtfetch":
            writeLine(["   ▄▀▀▀▀▄      " + app.user + "@" + app.host,
                   "  █  ▄▄  █     ─────────────────────",
                   "  █ █  █ █     OS       QTOS " + Kit.version,
                   "  █  ▀▀ ▄█     Shell    Prism (Qt Quick)",
                   "   ▀▄▄▄▀ ▀▄    Kit      Lumen · " + Theme.fontFamily,
                   "               Accent   " + Math.round(Services.settings.accentHue) + "° OKLCH",
                   "               Network  " + (Services.network.online ? Services.network.activeSsid : "offline"),
                   "               Battery  " + Services.power.batteryLevel + "%" + (Services.power.charging ? " (charging)" : ""),
                   "               Media    " + Services.media.title + " — " + Services.media.artist].join("\n"), "accent");
            break;
        case "theme":
            if (args[0] === "dark" || args[0] === "light") {
                Services.settings.darkMode = args[0] === "dark";
                writeLine("appearance → " + args[0], "ok");
            } else {
                writeLine("usage: theme dark|light", "err");
            }
            break;
        case "accent": {
            const hue = Number(args[0]);
            if (isNaN(hue)) {
                writeLine("usage: accent <hue 0-360>", "err");
            } else {
                Services.settings.accentHue = ((hue % 360) + 360) % 360;
                writeLine("accent hue → " + Services.settings.accentHue + "°", "ok");
            }
            break;
        }
        case "wifi":
            if (args[0] === "on" || args[0] === "off") {
                Services.network.wifiEnabled = args[0] === "on";
                writeLine("wi-fi → " + args[0], "ok");
            } else {
                writeLine("wi-fi " + (Services.network.wifiEnabled ? "on" : "off") + ", "
                      + (Services.network.online ? "connected to " + Services.network.activeSsid : "not connected"), "out");
            }
            break;
        case "play":
        case "pause":
            if ((name === "play") !== Services.media.playing)
                Services.media.togglePlayback();
            writeLine(name === "play" ? "▶ " + Services.media.title : "⏸ paused", "ok");
            break;
        case "next":
            Services.media.next();
            writeLine("⏭ next track", "ok");
            break;
        case "notify":
            Services.notifications.notify("system", "Terminal", args.join(" ") || "Hello!", "terminal");
            writeLine("notification posted", "ok");
            break;
        case "open":
            Intents.openApp(args[0] ?? "", {});
            break;
        case "lock":
            Intents.request("session.lock", {});
            break;
        default:
            writeLine("qtsh: command not found: " + name + "   (try 'help')", "err");
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Oklch.color(0.17, 0.02, Theme.hue, 1)
    }
    Glass {
        anchors.fill: parent
        backdrop: Materials.desktopFor(Window.window)
        tint: Oklch.color(0.16, 0.02, Theme.hue, 0.86)
        radius: 0
        elevation: 0
        strokeWidth: 0
        highlight: 0
    }

    ListView {
        id: output
        x: 18
        y: app.topInset + 6
        width: parent.width - 36
        height: parent.height - y - inputRow.height - 18
        clip: true
        model: lines
        boundsBehavior: Flickable.StopAtBounds
        delegate: Text {
            required property string line
            required property string kind
            width: ListView.view.width
            text: line
            textFormat: Text.PlainText
            font: Theme.mono
            wrapMode: Text.WrapAnywhere
            color: kind === "cmd" ? "#e8eaf2"
                   : kind === "err" ? Oklch.color(0.72, 0.17, 25)
                   : kind === "ok" ? Oklch.color(0.78, 0.15, 150)
                   : kind === "accent" ? Oklch.color(0.80, 0.12, Theme.hue)
                   : kind === "dim" ? "#9aa0ae" : "#d5d8e2"
        }
    }

    Row {
        id: inputRow
        x: 18
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        width: parent.width - 36
        spacing: 8
        Text {
            id: promptText
            text: app.prompt()
            font: Theme.mono
            color: Oklch.color(0.80, 0.13, Theme.hue)
        }
        TextInput {
            id: input
            width: parent.width - promptText.width - 8
            font: Theme.mono
            color: "#f2f3f8"
            selectionColor: Oklch.color(0.5, 0.15, Theme.hue, 0.6)
            focus: true
            cursorDelegate: Rectangle {
                width: 8
                color: Oklch.color(0.82, 0.12, Theme.hue)
                opacity: input.activeFocus ? 0.85 : 0.3
                SequentialAnimation on visible {
                    loops: Animation.Infinite
                    running: input.activeFocus
                    PropertyAction { value: true }
                    PauseAnimation { duration: 530 }
                    PropertyAction { value: false }
                    PauseAnimation { duration: 530 }
                }
            }
            Keys.onReturnPressed: {
                app.run(text);
                text = "";
            }
            Keys.onUpPressed: {
                if (app.historyIndex > 0) {
                    app.historyIndex--;
                    text = app.history[app.historyIndex];
                }
            }
            Keys.onDownPressed: {
                if (app.historyIndex < app.history.length - 1) {
                    app.historyIndex++;
                    text = app.history[app.historyIndex];
                } else {
                    app.historyIndex = app.history.length;
                    text = "";
                }
            }
        }
    }

    TapHandler { onTapped: input.forceActiveFocus() }

    Component.onCompleted: {
        writeLine("QTOS " + Kit.version + " · Prism terminal · Qt Quick, Lumen, Qt Remote Objects", "accent");
        writeLine("Type 'help' for commands, 'qtfetch' for a system summary.", "dim");
        writeLine("", "out");
        run("qtfetch");
    }
}
