// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QQ

Scene {
    name: "rift-delve"
    skyTop: "#05070c"
    skyHorizon: "#101722"
    groundHorizon: "#0a0d12"
    groundBottom: "#020304"
    sky: "../assets/delve.hdr"
    skyTurn: 0
    skyLight: 0.6
    exposure: 1
    bloom: true
    gravity: 9.81
    screens: "title"
    view: "3d"
    pixelsPerUnit: 16
    viewHeight: 12
    pixelArt: false
    seed: 0

    Group {
        name: "Stage"
        position: Qt.vector3d(0, 1, 0)
        eulerRotation: Qt.vector3d(0, 0, 0)
        scale: Qt.vector3d(1, 1, 1)

        Group {
            name: "Board"
            position: Qt.vector3d(0, -0.2, 0)
            eulerRotation: Qt.vector3d(38, 0, 0)
            scale: Qt.vector3d(0.8, 0.8, 0.8)

            Group {
                name: "Dungeon"
                position: Qt.vector3d(0, 0, 0)
                eulerRotation: Qt.vector3d(0, 0, 0)
                scale: Qt.vector3d(1, 1, 1)

                Shape {
                    name: "Floor"
                    position: Qt.vector3d(22, 0, 16)
                    eulerRotation: Qt.vector3d(-90, 0, 0)
                    scale: Qt.vector3d(44, 32, 1)
                    form: "plane"
                    colour: "#8c8f96"
                    metalness: 0
                    roughness: 0.85
                    emissive: "#000000"
                    emissivePower: 0
                    image: "../assets/flagstones.png"
                    imageSize: 4
                    normalMap: "../assets/flagstones-relief.png"
                    crisp: false
                    body: "none"
                    mass: 1
                    bounce: 0.2
                    friction: 0.6
                }

                Shape {
                    name: "Halo"
                    position: Qt.vector3d(0, 0.02, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(0.9, 0.005, 0.9)
                    form: "cylinder"
                    colour: "#1a0c06"
                    metalness: 0
                    roughness: 0.6
                    emissive: "#ffb466"
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
                    name: "Delver"
                    position: Qt.vector3d(0, 0.32, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(0.44, 0.64, 0.44)
                    form: "cylinder"
                    colour: "#ffe7b1"
                    metalness: 0.6
                    roughness: 0.3
                    emissive: "#ffd08a"
                    emissivePower: 0.35
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
                    name: "Blade"
                    position: Qt.vector3d(0, 0.42, 0.55)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(0.07, 0.05, 0.62)
                    form: "cube"
                    colour: "#e8f4ff"
                    metalness: 0.9
                    roughness: 0.15
                    emissive: "#cffaff"
                    emissivePower: 0.35
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
                    name: "Whirl"
                    position: Qt.vector3d(0, -30, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(4.4, 0.01, 4.4)
                    form: "cylinder"
                    colour: "#1a0c06"
                    metalness: 0
                    roughness: 0.6
                    emissive: "#ff9a3c"
                    emissivePower: 0.35
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
                    position: Qt.vector3d(0, -30, 0)
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
                    name: "Stairs"
                    position: Qt.vector3d(0, -30, 0)
                    eulerRotation: Qt.vector3d(0, 0, 0)
                    scale: Qt.vector3d(1.3, 0.03, 1.3)
                    form: "cylinder"
                    colour: "#04181b"
                    metalness: 0.2
                    roughness: 0.4
                    emissive: "#74cdd3"
                    emissivePower: 0.7
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
                    name: "Rift motes"
                    position: Qt.vector3d(0, -30, 0)
                    colour: "#9fe7ee"
                    rate: 28
                    life: 1.6
                    size: 0.06
                    speed: 0.7
                    spread: 0.5
                }

                Lamp {
                    name: "Torch"
                    position: Qt.vector3d(0, 2.2, 0)
                    color: "#ffcf96"
                    brightness: 3.2
                    reach: 9
                }
            }
        }
    }

    Behaviour {
        name: "Rift Delve"
        source: "../logic/rift-delve.qml"
        target: ""
    }

    Music {
        name: "Music"
        source: "../assets/delve.wav"
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
        shade: 0.55
    }

    ScreenText {
        name: "Title"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: -190
        text: "RIFT DELVE"
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
        text: "Three floors down. Break the warden."
        size: 26
        colour: "#d6d1c6"
        typeface: "ui"
    }

    ScreenText {
        name: "How"
        screen: "title"
        anchor: "centre"
        insetX: 0
        insetY: 190
        text: "Move with W, A, S and D, the arrows, or the pad's stick or d-pad. Strike with Space or A, whirl with K or X, dash with L or B, drink with Q or Y."
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

    ScreenText {
        name: "Score"
        screen: "hud"
        anchor: "top-left"
        insetX: 40
        insetY: 32
        text: "SCORE 0"
        size: 28
        colour: "#f3ede1"
        typeface: "ui"
    }

    ScreenText {
        name: "Gold"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 72
        text: "GOLD 0"
        size: 18
        colour: "#ead39a"
        typeface: "ui"
    }

    ScreenBar {
        name: "Health"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 106
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
        insetY: 102
        text: "100 / 100"
        size: 16
        colour: "#f3ede1"
        typeface: "ui"
    }

    ScreenBar {
        name: "Fury"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 130
        width: 320
        height: 9
        value: 1
        maximum: 1
        colour: "#ff9a3c"
    }

    ScreenText {
        name: "Potions"
        screen: "hud"
        anchor: "top-left"
        insetX: 42
        insetY: 150
        text: "POTIONS 2  ·  RELICS 0"
        size: 16
        colour: "#9ba2ad"
        typeface: "ui"
    }

    ScreenText {
        name: "Depth"
        screen: "hud"
        anchor: "top"
        insetX: 0
        insetY: 28
        text: "FLOOR 1 OF 3"
        size: 30
        colour: "#ead39a"
        typeface: "display"
    }

    ScreenText {
        name: "Level"
        screen: "hud"
        anchor: "top"
        insetX: 0
        insetY: 72
        text: "LEVEL 1  ·  0 / 50"
        size: 16
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
        insetX: 130
        insetY: 130
        width: 96
        text: "Up"
        colour: "#74cdd3"
        action: "up"
    }

    ScreenButton {
        name: "Left"
        screen: "hud"
        anchor: "bottom-left"
        insetX: 28
        insetY: 72
        width: 96
        text: "Left"
        colour: "#74cdd3"
        action: "left"
    }

    ScreenButton {
        name: "Right"
        screen: "hud"
        anchor: "bottom-left"
        insetX: 232
        insetY: 72
        width: 96
        text: "Right"
        colour: "#74cdd3"
        action: "right"
    }

    ScreenButton {
        name: "Down"
        screen: "hud"
        anchor: "bottom-left"
        insetX: 130
        insetY: 14
        width: 96
        text: "Down"
        colour: "#74cdd3"
        action: "down"
    }

    ScreenButton {
        name: "Drink"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 130
        insetY: 130
        width: 96
        text: "Drink"
        colour: "#ff6a7a"
        action: "potion"
    }

    ScreenButton {
        name: "Whirl"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 232
        insetY: 72
        width: 96
        text: "Whirl"
        colour: "#ff9a3c"
        action: "whirl"
    }

    ScreenButton {
        name: "Dash"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 28
        insetY: 72
        width: 96
        text: "Dash"
        colour: "#74cdd3"
        action: "dash"
    }

    ScreenButton {
        name: "Strike"
        screen: "hud"
        anchor: "bottom-right"
        insetX: 130
        insetY: 14
        width: 96
        text: "Strike"
        colour: "#c9a45c"
        action: "attack"
    }

    ScreenText {
        name: "Foe"
        screen: "foe"
        anchor: "top"
        insetX: 0
        insetY: 104
        text: "HUSK"
        size: 15
        colour: "#ff9a8a"
        typeface: "ui"
    }

    ScreenBar {
        name: "Foe health"
        screen: "foe"
        anchor: "top"
        insetX: 0
        insetY: 128
        width: 360
        height: 8
        value: 1
        maximum: 1
        colour: "#d2483c"
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
        text: "RIFT CLEARED"
        size: 72
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
        name: "whirl"
        keys: "K, E"
        pad: "X"
    }

    InputAction {
        name: "dash"
        keys: "L, Shift"
        pad: "B"
    }

    InputAction {
        name: "potion"
        keys: "Q, H"
        pad: "Y"
    }

    InputAction {
        name: "pause"
        keys: "P"
        pad: "Start"
    }
}
