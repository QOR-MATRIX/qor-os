# qor_input — game controllers and the controller interface (QOR.Input).
#
# Native controller backends with no third-party dependencies: XInput and raw
# HID (DualShock 4, DualSense, generic HID game pads) on Windows, evdev on
# Linux, and virtual controllers for tests. ControllerNavigator turns a
# controller into pointer, scroll and key input for the shell. See
# docs/CONTROLLERS.md. SystemGestures recognises three-finger swipes on
# touchpads (precision touchpads read through Raw Input on Windows) and
# touchscreens.

set_source_files_properties(qml/ControllerGlyph.qml PROPERTIES QT_RESOURCE_ALIAS ControllerGlyph.qml)

qt_add_library(qor_input STATIC)

qt_add_qml_module(qor_input
    URI QOR.Input
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/ControllerGlyph.qml
    SOURCES
        qml/gamepads.h qml/gamepads.cpp
        qml/navigator.h qml/navigator.cpp
        qml/systemgestures.h qml/systemgestures.cpp
        src/backends.h
        src/gamepad.h
        src/gamepadbackend.h
        src/gamepadmanager.h src/gamepadmanager.cpp
        src/injection.h src/injection.cpp
        src/keycodes.h src/keycodes.cpp
        src/mapping.h src/mapping.cpp
        src/playstation.h src/playstation.cpp
        src/touchpad.h src/touchpad.cpp
    DEPENDENCIES
        QtQuick
        QtQuick.Shapes
        QOR.Kit
)

if(WIN32)
    target_sources(qor_input PRIVATE src/backends_windows.cpp src/touchpad_windows.cpp)
    target_link_libraries(qor_input PRIVATE hid setupapi cfgmgr32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(qor_input PRIVATE src/backends_linux.cpp src/touchpad_other.cpp)
else()
    target_sources(qor_input PRIVATE src/backends_other.cpp src/touchpad_other.cpp)
endif()

target_include_directories(qor_input PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml"
)
target_link_libraries(qor_input
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
    PRIVATE
        Qt6::GuiPrivate
        qor::compiler_settings
)
