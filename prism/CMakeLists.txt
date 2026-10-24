# Prism — the QTOS shell and compositor.
#
#   shell/       QTOS.Shell QML module (Rail, Shelf, Beam, Spaces, windows …)
#   host/        qtos-shell: runs the shell in a desktop window with simulated
#                services (Windows, macOS, Linux)
#   compositor/  prism: the Wayland compositor that hosts the same shell on
#                device (Linux only, Qt Wayland Compositor)

add_subdirectory(shell)

qt_add_executable(qtos-shell WIN32 MACOSX_BUNDLE
    host/main.cpp
    host/gpupreference.h host/gpupreference.cpp
)

if(WIN32)
    target_link_libraries(qtos-shell PRIVATE dxgi)
endif()

target_link_libraries(qtos-shell PRIVATE
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
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

set_target_properties(qtos-shell PROPERTIES
    OUTPUT_NAME qtos-shell
    MACOSX_BUNDLE_GUI_IDENTIFIER dev.qor-matrix.qtos.shell
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

install(TARGETS qtos-shell
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

qt_generate_deploy_qml_app_script(
    TARGET qtos-shell
    OUTPUT_SCRIPT _qtos_shell_deploy
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${_qtos_shell_deploy})

if(QTOS_BUILD_COMPOSITOR)
    add_subdirectory(compositor)
endif()
