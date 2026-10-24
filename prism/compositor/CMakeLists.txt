# prism — the QTOS Wayland compositor (Linux only, experimental).

find_package(Qt6 ${QTOS_QT_MIN_VERSION} QUIET COMPONENTS WaylandCompositor)
if(NOT Qt6WaylandCompositor_FOUND)
    message(WARNING "Qt Wayland Compositor not found: skipping the prism compositor. "
                    "Install the Qt Wayland Compositor module for your kit to build it.")
    return()
endif()

qt_add_executable(prism main.cpp)

qt_add_qml_module(prism
    URI QTOS.Prism
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        Compositor.qml
    DEPENDENCIES
        QtQuick
        QtWayland.Compositor
        QTOS.Kit
        QTOS.Shell
)

target_link_libraries(prism PRIVATE
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::WaylandCompositor
    qtos_kit
    qtos_services
    qtos_kitplugin
    qtos_lumen_styleplugin
    qtos_servicesplugin
    qtos_appsplugin
    qtos_shellplugin
    qtos_sim
    qtos::compiler_settings
)

install(TARGETS prism RUNTIME DESTINATION bin)
