# QTOS.Shell — the Prism shell UI, shared by qtos-shell (simulation) and the
# prism Wayland compositor (device).

# Imported by relative path from QML; not exported as a module member.
set_source_files_properties(search.js PROPERTIES QT_QML_SKIP_QMLDIR_ENTRY TRUE)

qt_add_library(qtos_shell STATIC)

qt_add_qml_module(qtos_shell
    URI QTOS.Shell
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        Main.qml
        Desktop.qml
        WindowLayer.qml
        AppWindow.qml
        Rail.qml
        LiveActivity.qml
        MediaPopover.qml
        Shelf.qml
        ShelfButton.qml
        Beam.qml
        ControlCenter.qml
        NotificationStack.qml
        NotificationCard.qml
        LockScreen.qml
        SpaceSwitcher.qml
        VolumeHud.qml
        ControllerCursor.qml
        ControllerHints.qml
        ControllerKeyboard.qml
        ControllerPrompt.qml
        ShellToast.qml
        search.js
    SOURCES
        desktopentries.h desktopentries.cpp
    DEPENDENCIES
        QtQuick
        QtQuick.Controls
        QTOS.Kit
        QTOS.Services
        QTOS.Apps
        QTOS.Input
)

# The --benchmark and --script drivers shared by qtos-shell and prism. The
# script runner injects input through QWindowSystemInterface (Qt GUI private).
target_sources(qtos_shell PRIVATE shellbenchmark.h shellbenchmark.cpp shellscript.h shellscript.cpp)
target_link_libraries(qtos_shell PRIVATE Qt6::GuiPrivate)
target_include_directories(qtos_shell PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qtos_shell
    PUBLIC
        Qt6::Quick
        qtos_kit
        qtos_services
        qtos_apps
        qtos_input
    PRIVATE
        qtos::compiler_settings
)
