# QTOS.Apps — first-party applications.
#
# Each app is a QML component the shell hosts inside a Prism window. Apps only
# talk to the system through QTOS.Services (replicas + Intents), never to the
# shell directly, so the same code ships as standalone Qt Application Manager
# packages on device.

set_source_files_properties(AppRegistry.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
# Imported by relative path from QML; not exported as module members.
set_source_files_properties(FileIndex.js PROPERTIES QT_QML_SKIP_QMLDIR_ENTRY TRUE)

qt_add_library(qtos_apps STATIC)

qt_add_qml_module(qtos_apps
    URI QTOS.Apps
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        AppRegistry.qml
        FileIndex.js
        FrostSidebar.qml
        FilesApp.qml
        MusicApp.qml
        NotesApp.qml
        TerminalApp.qml
        MonitorApp.qml
        SettingsApp.qml
    DEPENDENCIES
        QtQuick
        QtQuick.Controls
        QtQuick.Shapes
        QtGraphs
        QtCore
        QTOS.Kit
        QTOS.Services
        QTOS.Input
)

target_link_libraries(qtos_apps
    PUBLIC
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickShapes
        Qt6::Graphs
        qtos_kit
        qtos_services
        qtos_input
    PRIVATE
        qtos::compiler_settings
)
