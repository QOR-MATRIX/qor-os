# qtos_input — game controllers and the controller interface (QTOS.Input).
#
# Native controller backends with no third-party dependencies: XInput and raw
# HID (DualShock 4, DualSense, generic HID game pads) on Windows, evdev on
# Linux, and virtual controllers for tests. ControllerNavigator turns a
# controller into pointer, scroll and key input for the shell. See
# docs/CONTROLLERS.md.

set_source_files_properties(qml/ControllerGlyph.qml PROPERTIES QT_RESOURCE_ALIAS ControllerGlyph.qml)

qt_add_library(qtos_input STATIC)

qt_add_qml_module(qtos_input
    URI QTOS.Input
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/ControllerGlyph.qml
    SOURCES
        qml/gamepads.h qml/gamepads.cpp
        qml/navigator.h qml/navigator.cpp
        src/backends.h
        src/gamepad.h
        src/gamepadbackend.h
        src/gamepadmanager.h src/gamepadmanager.cpp
        src/injection.h src/injection.cpp
        src/keycodes.h src/keycodes.cpp
        src/mapping.h src/mapping.cpp
        src/playstation.h src/playstation.cpp
    DEPENDENCIES
        QtQuick
        QtQuick.Shapes
        QTOS.Kit
)

if(WIN32)
    target_sources(qtos_input PRIVATE src/backends_windows.cpp)
    target_link_libraries(qtos_input PRIVATE hid setupapi cfgmgr32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(qtos_input PRIVATE src/backends_linux.cpp)
else()
    target_sources(qtos_input PRIVATE src/backends_other.cpp)
endif()

target_include_directories(qtos_input PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml"
)
target_link_libraries(qtos_input
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
    PRIVATE
        Qt6::GuiPrivate
        qtos::compiler_settings
)
