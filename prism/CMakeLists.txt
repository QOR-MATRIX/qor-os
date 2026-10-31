# Prism — the QOR OS shell and compositor.
#
#   shell/       QOR.Shell QML module (Rail, Shelf, Beam, Spaces, windows …)
#   host/        qor-shell: runs the shell in a desktop window with simulated
#                services (Windows, macOS, Linux)
#   compositor/  prism: the Wayland compositor that hosts the same shell on
#                device (Linux only, Qt Wayland Compositor)

add_subdirectory(shell)

# The simulated shell; a device's image is built without it (QOR_BUILD_SIMULATOR).
if(QOR_BUILD_SIMULATOR)
    qt_add_executable(qor-shell WIN32 MACOSX_BUNDLE
        host/main.cpp
        host/gpupreference.h host/gpupreference.cpp
    )

    if(WIN32)
        target_link_libraries(qor-shell PRIVATE dxgi)
    endif()

    target_link_libraries(qor-shell PRIVATE
        Qt6::Gui
        Qt6::Quick
        Qt6::QuickControls2
        qor_kit
        qor_services
        qor_kitplugin
        qor_lumen_styleplugin
        qor_servicesplugin
        qor_appsplugin
        qor_inputplugin
        qor_storeplugin
        qor_shell
        qor_shellplugin
        qor_sim
        qor::compiler_settings
    )

    set_target_properties(qor-shell PROPERTIES
        OUTPUT_NAME qor-shell
        # The QML modules are static and embedded, so tell the deployment import
        # scanner where their sources live; otherwise it only scans prism/ and
        # misses imports such as QtCore (Notes) and QtGraphs (Monitor).
        QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS
            "${CMAKE_SOURCE_DIR}/kit;${CMAKE_SOURCE_DIR}/apps;${CMAKE_SOURCE_DIR}/services;${CMAKE_SOURCE_DIR}/store"
        MACOSX_BUNDLE_GUI_IDENTIFIER dev.qor-matrix.qor.shell
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )

    install(TARGETS qor-shell
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )

    qt_generate_deploy_qml_app_script(
        TARGET qor-shell
        OUTPUT_SCRIPT _qor_shell_deploy
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${_qor_shell_deploy})
endif()

if(QOR_BUILD_COMPOSITOR)
    add_subdirectory(compositor)
endif()
