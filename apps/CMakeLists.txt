# QOR.Apps — first-party applications.
#
# Each app is a QML component the shell hosts inside a Prism window. Apps only
# talk to the system through QOR.Services (replicas + Intents), never to the
# shell directly, so the same code ships as standalone Qt Application Manager
# packages on device.

set_source_files_properties(AppRegistry.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
# Imported by relative path from QML; not exported as module members.
set_source_files_properties(FileIndex.js PROPERTIES QT_QML_SKIP_QMLDIR_ENTRY TRUE)

qt_add_library(qor_apps STATIC)

qt_add_qml_module(qor_apps
    URI QOR.Apps
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
        StoreApp.qml
    DEPENDENCIES
        QtQuick
        QtQuick.Controls
        QtQuick.Shapes
        QtGraphs
        QtCore
        QOR.Kit
        QOR.Services
        QOR.Input
        QOR.Store
)

target_link_libraries(qor_apps
    PUBLIC
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickShapes
        Qt6::Graphs
        qor_kit
        qor_services
        qor_input
        qor_store
    PRIVATE
        qor::compiler_settings
)
