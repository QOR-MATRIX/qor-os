# QOR.Shell — the Prism shell UI, shared by qor-shell (simulation) and the
# prism Wayland compositor (device).

# Imported by relative path from QML; not exported as a module member.
set_source_files_properties(search.js PROPERTIES QT_QML_SKIP_QMLDIR_ENTRY TRUE)

qt_add_library(qor_shell STATIC)

qt_add_qml_module(qor_shell
    URI QOR.Shell
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
        WidgetLayer.qml
        WidgetGallery.qml
        DisplayDesktop.qml
        Home.qml
        HomeCard.qml
        HomeShelf.qml
        search.js
    SOURCES
        desktopentries.h desktopentries.cpp
        producthost.h producthost.cpp
        library.h library.cpp
    DEPENDENCIES
        QtQuick
        QtQuick.Controls
        QOR.Kit
        QOR.Services
        QOR.Apps
        QOR.Input
        QOR.Store
)

# The --benchmark and --script drivers shared by qor-shell and prism. The
# script runner injects input through QWindowSystemInterface (Qt GUI private).
target_sources(qor_shell PRIVATE shellbenchmark.h shellbenchmark.cpp shellscript.h shellscript.cpp)
target_link_libraries(qor_shell PRIVATE Qt6::GuiPrivate)
target_include_directories(qor_shell PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qor_shell
    PUBLIC
        Qt6::Quick
        qor_kit
        qor_services
        qor_apps
        qor_input
        qor_store
    PRIVATE
        # The products' channels (producthost.cpp).
        Qt6::Network
        # The Library reads the person's folders on a worker thread (library.cpp).
        Qt6::Concurrent
        qor::compiler_settings
)
