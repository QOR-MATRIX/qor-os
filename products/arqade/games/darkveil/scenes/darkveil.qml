// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QQ

Scene {
    name: "darkveil"
    skyTop: "#10142c"
    skyHorizon: "#b4603c"
    groundHorizon: "#2a1a1a"
    groundBottom: "#07080c"
    sky: "../assets/darkveil.hdr"
    skyTurn: 0
    skyLight: 0.62
    exposure: 1
    bloom: true
    gravity: 9.81
    screens: "title"
    view: "3d"
    pixelsPerUnit: 16
    viewHeight: 12
    pixelArt: false
    seed: 0

    Sun {
        name: "Sun"
        elevation: 26
        azimuth: -52
        color: "#ffb98a"
        brightness: 0.72
    }

    Group {
        name: "Stage"
        position: Qt.vector3d(0, 0.4, 0)
        eulerRotation: Qt.vector3d(0, 0, 0)
        scale: Qt.vector3d(1, 1, 1)

        Group {
            name: "Tilt"
            position: Qt.vector3d(0, 0, 0)
            eulerRotation: Qt.vector3d(26, 0, 0)
            scale: Qt.vector3d(1.22, 1.22, 1.22)

            Group {
                name: "Turn"
                position: Qt.vector3d(0, 0, 0)
                eulerRotation: Qt.vector3d(0, 0, 0)
                scale: Qt.vector3d(1, 1, 1)

            Group {
                name: "Valley"
                position: Qt.vector3d(0, 0, 0)
                eulerRotation: Qt.vector3d(0, 0, 0)
                scale: Qt.vector3d(1, 1, 1)

                Prop {
                    name: "Land"
                    position: Qt.vector3d(0, 0, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1, 1, 1)
                    source: "../assets/land.glb"
                    clip: ""
                    clipSpeed: 1
                    clipLoops: true
                }

                Prop {
                    name: "Scenery 0"
                    position: Qt.vector3d(0, 0, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1, 1, 1)
                    source: "../assets/scenery-0.glb"
                    clip: ""
                    clipSpeed: 1
                    clipLoops: true
                }

                Prop {
                    name: "Scenery 1"
                    position: Qt.vector3d(0, 0, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1, 1, 1)
                    source: "../assets/scenery-1.glb"
                    clip: ""
                    clipSpeed: 1
                    clipLoops: true
                }

                Prop {
                    name: "Scenery 2"
                    position: Qt.vector3d(0, 0, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1, 1, 1)
                    source: "../assets/scenery-2.glb"
                    clip: ""
                    clipSpeed: 1
                    clipLoops: true
                }

                Prop {
                    name: "Scenery 3"
                    position: Qt.vector3d(0, 0, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1, 1, 1)
                    source: "../assets/scenery-3.glb"
                    clip: ""
                    clipSpeed: 1
                    clipLoops: true
                }

                Shape {
                    name: "Water"
                    position: Qt.vector3d(32, -0.25, 32)
                    eulerRotation: Qt.vector3d(-90, 0, 0)
                    scale: Qt.vector3d(64, 64, 1)
                    form: "plane"
                    colour: "#2c4a5e"
                    metalness: 0.25
                    roughness: 0.18
                    emissive: "#16324a"
                    emissivePower: 0.12
                    image: ""
                    imageSize: 1
                    normalMap: ""
                    crisp: false
                    body: "none"
                    mass: 1
                    bounce: 0.2
                    friction: 0.6
                }

                Prop {
                    name: "Hero"
                    position: Qt.vector3d(0, 0, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1, 1, 1)
                    source: "../assets/hero.glb"
                    clip: "idle"
                    clipSpeed: 1
                    clipLoops: true
                }

                Shape {
                    name: "Mark"
                    position: Qt.vector3d(0, -40, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(0.1, 14, 0.1)
                    form: "cylinder"
                    colour: "#2a1a06"
                    metalness: 0
                    roughness: 0.5
                    emissive: "#ffc76a"
                    emissivePower: 0.85
                    image: ""
                    imageSize: 1
                    normalMap: ""
                    crisp: false
                    body: "none"
                    mass: 1
                    bounce: 0.2
                    friction: 0.6
                }

                Shape {
                    name: "Gate bars"
                    position: Qt.vector3d(0, -40, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(2.6, 2.3, 0.2)
                    form: "cube"
                    colour: "#3a3d44"
                    metalness: 0.8
                    roughness: 0.4
                    emissive: "#ff7a3d"
                    emissivePower: 0.08
                    image: ""
                    imageSize: 1
                    normalMap: ""
                    crisp: false
                    body: "none"
                    mass: 1
                    bounce: 0.2
                    friction: 0.6
                }

                Shape {
                    name: "Slam"
                    position: Qt.vector3d(0, -40, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1, 0.01, 1)
                    form: "cylinder"
                    colour: "#1a0606"
                    metalness: 0
                    roughness: 0.6
                    emissive: "#ff3b2f"
                    emissivePower: 0.3
                    image: ""
                    imageSize: 1
                    normalMap: ""
                    crisp: false
                    body: "none"
                    mass: 1
                    bounce: 0.2
                    friction: 0.6
                }

                Shape {
                    name: "Burst"
                    position: Qt.vector3d(0, -40, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(5.6, 0.01, 5.6)
                    form: "cylinder"
                    colour: "#06181c"
                    metalness: 0
                    roughness: 0.6
                    emissive: "#74cdd3"
                    emissivePower: 0.6
                    image: ""
                    imageSize: 1
                    normalMap: ""
                    crisp: false
                    body: "none"
                    mass: 1
                    bounce: 0.2
                    friction: 0.6
                }

                Emitter {
                    name: "Beacon 0"
                    position: Qt.vector3d(0, -40, 0)
                    colour: "#ffd9a0"
                    rate: 0
                    life: 1.5
                    size: 0.16
                    speed: 1.6
                    spread: 0.35
                }

                Lamp {
                    name: "Beacon light 0"
                    position: Qt.vector3d(0, -40, 0)
                    color: "#ffd9a0"
                    brightness: 0
                    reach: 9
                }

                Emitter {
                    name: "Beacon 1"
                    position: Qt.vector3d(0, -40, 0)
                    colour: "#ffc27a"
                    rate: 0
                    life: 1.5
                    size: 0.16
                    speed: 1.6
                    spread: 0.35
                }

                Lamp {
                    name: "Beacon light 1"
                    position: Qt.vector3d(0, -40, 0)
                    color: "#ffc27a"
                    brightness: 0
                    reach: 9
                }

                Emitter {
                    name: "Beacon 2"
                    position: Qt.vector3d(0, -40, 0)
                    colour: "#ffe6c0"
                    rate: 0
                    life: 1.5
                    size: 0.16
                    speed: 1.6
                    spread: 0.35
                }

                Lamp {
                    name: "Beacon light 2"
                    position: Qt.vector3d(0, -40, 0)
                    color: "#ffe6c0"
                    brightness: 0
                    reach: 9
                }

                Lamp {
                    name: "Lantern"
                    position: Qt.vector3d(0, 2.4, 0)
                    color: "#ffd9a8"
                    brightness: 2.4
                    reach: 8
                }

            }

            }

        }

    }

    Behaviour {
        name: "Darkveil"
        source: "../logic/darkveil.qml"
        target: ""
    }

    Music {
        name: "Music"
        source: "../assets/darkveil.wav"
        volume: 0.4
        fade: 1.5
        loops: true
        group: "music"
    }

    ScreenPanel {
        name: "Title veil"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: 0
        width: 0
        height: 0
        colour: "#05060a"
        shade: 0.5
    }

    ScreenText {
        name: "Title"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: -190
        text: "DARKVEIL"
        size: 92
        colour: "#ead39a"
        typeface: "display"
    }

    ScreenText {
        name: "Tagline"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: -110
        text: "Light the three beacons. End the Hollow King."
        size: 26
        colour: "#d6d1c6"
        typeface: "ui"
    }

    ScreenText {
        name: "How"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: 150
        text: "Move with W, A, S and D, the arrows, or the pad's stick or d-pad; Z and C, or the shoulders, turn the view."
        size: 18
        colour: "#9ba2ad"
        typeface: "ui"
    }

    ScreenText {
        name: "How more"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: 182
        text: "Strike with Space or A, roll with L or B, bolt with K or X, nova with I or RT, drink with Q or Y, talk with E or LT."
        size: 18
        colour: "#9ba2ad"
        typeface: "ui"
    }

    ScreenButton {
        name: "Play"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: 266
        width: 0
        text: "Play"
        colour: "#c9a45c"
        action: ""
    }

    ScreenPanel {
        name: "Top band"
        screen: "hud"
        anchor: "top"
        insetX: 0
        insetY: 0
        width: 2600
        height: 150
        colour: "#06070c"
        shade: 0.42
    }

    ScreenText {
        name: "Objective"
        screen: "hud"
        anchor: "top"
        insetX: 0
        insetY: 28
        text: "Speak to Elder Maren, in Hearth"
        size: 24
        colour: "#ead39a"
        typeface: "ui"
    }

    ScreenText {
        name: "Place"
        screen: "hud"
        anchor: "top"
        insetX: 0
        insetY: 64
        text: "HEARTH"
        size: 15
        colour: "#9ba2ad"
        typeface: "ui"
    }

    ScreenBar {
        name: "Health"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 36
        width: 320
        height: 16
        value: 1
        maximum: 1
        colour: "#d2483c"
    }

    ScreenText {
        name: "Health words"
        screen: "hud"
        anchor: "top-left"
        insetX: 372
        insetY: 32
        text: "120 / 120"
        size: 16
        colour: "#f3ede1"
        typeface: "ui"
    }

    ScreenBar {
        name: "Mana"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 60
        width: 320
        height: 9
        value: 1
        maximum: 1
        colour: "#5aa7e0"
    }

    ScreenBar {
        name: "Experience"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 76
        width: 320
        height: 5
        value: 1
        maximum: 1
        colour: "#c9a45c"
    }

    ScreenText {
        name: "Stats"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 92
        text: "LEVEL 1  ·  GOLD 0"
        size: 18
        colour: "#ead39a"
        typeface: "ui"
    }

    ScreenText {
        name: "Kit"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 122
        text: "POTIONS 3  ·  Rusted blade  ·  Worn leathers"
        size: 15
        colour: "#9ba2ad"
        typeface: "ui"
    }

    ScreenText {
        name: "Message"
        screen: "hud"
        anchor: "centre"
        insetX: 0
        insetY: -230
        text: ""
        size: 30
        colour: "#ead39a"
        typeface: "display"
    }

    ScreenText {
        name: "Hint"
        screen: "hud"
        anchor: "bottom"
        insetX: 0
        insetY: 148
        text: ""
        size: 20
        colour: "#f3ede1"
        typeface: "ui"
    }

    ScreenButton {
        name: "Pause"
        screen: "hud"
        anchor: "top-right"
        insetX: 32
        insetY: 28
        width: 0
        text: "Pause"
        colour: "#9ba2ad"
        action: "pause"
    }

    ScreenButton {
        name: "Up"
        screen: "hud"
        anchor: "bottom-left"
        insetX: 110
        insetY: 110
        width: 78
        text: "Up"
        colour: "#74cdd3"
        action: "up"
    }

    ScreenButton {
        name: "Left"
        screen: "hud"
        anchor: "bottom-left"
        insetX: 28
        insetY: 62
        width: 78
        text: "Left"
        colour: "#74cdd3"
        action: "left"
    }

    ScreenButton {
        name: "Right"
        screen: "hud"
        anchor: "bottom-left"
        insetX: 192
        insetY: 62
        width: 78
        text: "Right"
        colour: "#74cdd3"
        action: "right"
    }

    ScreenButton {
        name: "Down"
        screen: "hud"
        anchor: "bottom-left"
        insetX: 110
        insetY: 14
        width: 78
        text: "Down"
        colour: "#74cdd3"
        action: "down"
    }

    ScreenButton {
        name: "Strike"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 110
        insetY: 14
        width: 78
        text: "Strike"
        colour: "#c9a45c"
        action: "attack"
    }

    ScreenButton {
        name: "Roll"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 28
        insetY: 62
        width: 78
        text: "Roll"
        colour: "#74cdd3"
        action: "roll"
    }

    ScreenButton {
        name: "Bolt"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 192
        insetY: 62
        width: 78
        text: "Bolt"
        colour: "#5aa7e0"
        action: "bolt"
    }

    ScreenButton {
        name: "Nova"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 192
        insetY: 110
        width: 78
        text: "Nova"
        colour: "#74cdd3"
        action: "nova"
    }

    ScreenButton {
        name: "Drink"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 110
        insetY: 110
        width: 78
        text: "Drink"
        colour: "#ff6a7a"
        action: "potion"
    }

    ScreenButton {
        name: "Talk"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 28
        insetY: 110
        width: 78
        text: "Talk"
        colour: "#ead39a"
        action: "talk"
    }

    ScreenButton {
        name: "Turn left"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 192
        insetY: 14
        width: 78
        text: "View L"
        colour: "#9ba2ad"
        action: "turnLeft"
    }

    ScreenButton {
        name: "Turn right"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 28
        insetY: 14
        width: 78
        text: "View R"
        colour: "#9ba2ad"
        action: "turnRight"
    }

    ScreenText {
        name: "Foe"
        screen: "foe"
        anchor: "top"
        insetX: 0
        insetY: 96
        text: "WOLF"
        size: 15
        colour: "#ff9a8a"
        typeface: "ui"
    }

    ScreenBar {
        name: "Foe health"
        screen: "foe"
        anchor: "top"
        insetX: 0
        insetY: 120
        width: 360
        height: 8
        value: 1
        maximum: 1
        colour: "#d2483c"
    }

    ScreenPanel {
        name: "Hurt veil"
        screen: "hurt"
        anchor: "centre"
        insetX: 0
        insetY: 0
        width: 0
        height: 0
        colour: "#5a0a0a"
        shade: 0.28
    }

    ScreenPanel {
        name: "Talk panel"
        screen: "talk"
        anchor: "bottom"
        insetX: 0
        insetY: 24
        width: 1080
        height: 540
        colour: "#08090e"
        shade: 0.8
    }

    ScreenText {
        name: "Who"
        screen: "talk"
        anchor: "bottom"
        insetX: 0
        insetY: 512
        text: "Elder Maren"
        size: 26
        colour: "#ead39a"
        typeface: "display"
    }

    ScreenText {
        name: "Line 0"
        screen: "talk"
        anchor: "bottom"
        insetX: 0
        insetY: 472
        text: ""
        size: 21
        colour: "#e6e1d6"
        typeface: "ui"
    }

    ScreenText {
        name: "Line 1"
        screen: "talk"
        anchor: "bottom"
        insetX: 0
        insetY: 442
        text: ""
        size: 21
        colour: "#e6e1d6"
        typeface: "ui"
    }

    ScreenText {
        name: "Line 2"
        screen: "talk"
        anchor: "bottom"
        insetX: 0
        insetY: 412
        text: ""
        size: 21
        colour: "#e6e1d6"
        typeface: "ui"
    }

    ScreenText {
        name: "Line 3"
        screen: "talk"
        anchor: "bottom"
        insetX: 0
        insetY: 382
        text: ""
        size: 21
        colour: "#e6e1d6"
        typeface: "ui"
    }

    ScreenButton {
        name: "Say 1"
        screen: "say1"
        anchor: "bottom"
        insetX: 0
        insetY: 300
        width: 640
        text: ""
        colour: "#c9a45c"
        action: ""
    }

    ScreenButton {
        name: "Say 2"
        screen: "say2"
        anchor: "bottom"
        insetX: 0
        insetY: 248
        width: 640
        text: ""
        colour: "#c9a45c"
        action: ""
    }

    ScreenButton {
        name: "Say 3"
        screen: "say3"
        anchor: "bottom"
        insetX: 0
        insetY: 196
        width: 640
        text: ""
        colour: "#c9a45c"
        action: ""
    }

    ScreenButton {
        name: "Say 4"
        screen: "say4"
        anchor: "bottom"
        insetX: 0
        insetY: 144
        width: 640
        text: ""
        colour: "#c9a45c"
        action: ""
    }

    ScreenButton {
        name: "Say 5"
        screen: "say5"
        anchor: "bottom"
        insetX: 0
        insetY: 92
        width: 640
        text: ""
        colour: "#c9a45c"
        action: ""
    }

    ScreenButton {
        name: "Say 6"
        screen: "say6"
        anchor: "bottom"
        insetX: 0
        insetY: 40
        width: 640
        text: ""
        colour: "#c9a45c"
        action: ""
    }

    ScreenPanel {
        name: "Pause veil"
        screen: "pause"
        anchor: "centre"
        insetX: 0
        insetY: 0
        width: 0
        height: 0
        colour: "#05060a"
        shade: 0.62
    }

    ScreenText {
        name: "Paused"
        screen: "pause"
        anchor: "centre"
        insetX: 0
        insetY: -60
        text: "PAUSED"
        size: 64
        colour: "#ead39a"
        typeface: "display"
    }

    ScreenButton {
        name: "Resume"
        screen: "pause"
        anchor: "centre"
        insetX: 0
        insetY: 40
        width: 0
        text: "Resume"
        colour: "#c9a45c"
        action: ""
    }

    ScreenPanel {
        name: "Result veil"
        screen: "result"
        anchor: "centre"
        insetX: 0
        insetY: 0
        width: 0
        height: 0
        colour: "#05060a"
        shade: 0.55
    }

    ScreenText {
        name: "Result"
        screen: "result"
        anchor: "centre"
        insetX: 0
        insetY: -120
        text: "THE VALLEY BURNS BRIGHT"
        size: 62
        colour: "#ead39a"
        typeface: "display"
    }

    ScreenText {
        name: "Final"
        screen: "result"
        anchor: "centre"
        insetX: 0
        insetY: -40
        text: "SCORE 0"
        size: 30
        colour: "#f3ede1"
        typeface: "ui"
    }

    ScreenText {
        name: "Details"
        screen: "result"
        anchor: "centre"
        insetX: 0
        insetY: 4
        text: ""
        size: 18
        colour: "#9ba2ad"
        typeface: "ui"
    }

    ScreenButton {
        name: "Play again"
        screen: "result"
        anchor: "centre"
        insetX: 0
        insetY: 70
        width: 0
        text: "Play again"
        colour: "#c9a45c"
        action: ""
    }

    InputAction {
        name: "up"
        keys: "W, Up"
        pad: "Up, LS Up"
    }

    InputAction {
        name: "down"
        keys: "S, Down"
        pad: "Down, LS Down"
    }

    InputAction {
        name: "left"
        keys: "A, Left"
        pad: "Left, LS Left"
    }

    InputAction {
        name: "right"
        keys: "D, Right"
        pad: "Right, LS Right"
    }

    InputAction {
        name: "attack"
        keys: "Space, J"
        pad: "A"
    }

    InputAction {
        name: "roll"
        keys: "L, Shift"
        pad: "B"
    }

    InputAction {
        name: "bolt"
        keys: "K"
        pad: "X"
    }

    InputAction {
        name: "nova"
        keys: "I"
        pad: "RT"
    }

    InputAction {
        name: "potion"
        keys: "Q, H"
        pad: "Y"
    }

    InputAction {
        name: "talk"
        keys: "E, Return"
        pad: "LT"
    }

    InputAction {
        name: "turnLeft"
        keys: "Z"
        pad: "LB"
    }

    InputAction {
        name: "turnRight"
        keys: "C"
        pad: "RB"
    }

    InputAction {
        name: "say1"
        keys: "1"
        pad: ""
    }

    InputAction {
        name: "say2"
        keys: "2"
        pad: ""
    }

    InputAction {
        name: "say3"
        keys: "3"
        pad: ""
    }

    InputAction {
        name: "say4"
        keys: "4"
        pad: ""
    }

    InputAction {
        name: "say5"
        keys: "5"
        pad: ""
    }

    InputAction {
        name: "say6"
        keys: "6"
        pad: ""
    }

    InputAction {
        name: "pause"
        keys: "P"
        pad: "Start"
    }
}
