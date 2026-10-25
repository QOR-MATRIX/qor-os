# prism — the QTOS Wayland compositor (Linux only, experimental).

find_package(Qt6 ${QTOS_QT_MIN_VERSION} QUIET COMPONENTS WaylandCompositor WaylandScannerTools)
if(NOT Qt6WaylandCompositor_FOUND)
    message(WARNING "Qt Wayland Compositor not found: skipping the prism compositor. "
                    "Install the Qt Wayland Compositor module for your kit to build it.")
    return()
endif()

# wayland-scanner emits the protocol interface tables as C.
enable_language(C)

qt_add_executable(prism main.cpp)

# Protocols Prism implements itself (see extensions.h). The XML is vendored
# from wayland-protocols 1.47; tablet-v2 only provides an interface that
# cursor-shape refers to.
qt6_generate_wayland_protocol_server_sources(prism PRIVATE_CODE FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/fractional-scale-v1.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/cursor-shape-v1.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/tablet-v2.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-activation-v1.xml
)
# Scanner output is not ours to fix; keep its warnings out of the build log.
foreach(protocol IN ITEMS fractional-scale-v1 cursor-shape-v1 tablet-v2 xdg-activation-v1)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-${protocol}.cpp"
                                "${CMAKE_CURRENT_BINARY_DIR}/wayland-${protocol}-protocol.c"
                                PROPERTIES COMPILE_OPTIONS "-w")
endforeach()

qt_add_qml_module(prism
    URI QTOS.Prism
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        Compositor.qml
    SOURCES
        extensions.h extensions.cpp
    DEPENDENCIES
        QtQuick
        QtWayland.Compositor
        QTOS.Kit
        QTOS.Shell
)

target_include_directories(prism PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
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
    qtos_inputplugin
    qtos_shell
    qtos_shellplugin
    qtos_sim
    qtos::compiler_settings
)

install(TARGETS prism RUNTIME DESTINATION bin)
