# Platform integration: Lumen for every Qt app, not just QTOS's own.
#
#   common/   palette + fonts from the generated Lumen tokens (Qt Gui only)
#   style/    "Lumen" QStyle plugin for Qt Widgets apps
#   theme/    "qtos" QPA platform theme plugin (loaded on Linux devices)
#   gallery/  lumen-widgets-gallery, a preview of the QStyle

# The platform theme implements QPlatformTheme, a QPA (semi-private) interface.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 ${QTOS_QT_MIN_VERSION} REQUIRED COMPONENTS Widgets GuiPrivate)

set(QTOS_PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugins")

# ── Shared Lumen palette ────────────────────────────────────────────────────
qt_add_library(qtos_lumen_common STATIC
    common/lumentokens.h
    common/lumenpalette.h common/lumenpalette.cpp
    ${CMAKE_SOURCE_DIR}/kit/src/oklchmath.h ${CMAKE_SOURCE_DIR}/kit/src/oklchmath.cpp
)
target_include_directories(qtos_lumen_common PUBLIC common "${CMAKE_SOURCE_DIR}/kit/src")
target_link_libraries(qtos_lumen_common PUBLIC Qt6::Gui PRIVATE qtos::compiler_settings)
set_target_properties(qtos_lumen_common PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── Lumen QStyle plugin ─────────────────────────────────────────────────────
qt_add_plugin(qtos_lumen_widgets_style
    CLASS_NAME LumenStylePlugin
    PLUGIN_TYPE styles
)
target_sources(qtos_lumen_widgets_style PRIVATE
    style/lumenstyle.h style/lumenstyle.cpp style/lumenstyleplugin.cpp
)
target_include_directories(qtos_lumen_widgets_style PRIVATE style)
target_link_libraries(qtos_lumen_widgets_style PRIVATE Qt6::Widgets qtos_lumen_common qtos::compiler_settings)
set_target_properties(qtos_lumen_widgets_style PROPERTIES
    OUTPUT_NAME lumenstyle
    LIBRARY_OUTPUT_DIRECTORY "${QTOS_PLUGIN_DIR}/styles"
    RUNTIME_OUTPUT_DIRECTORY "${QTOS_PLUGIN_DIR}/styles"
)
install(TARGETS qtos_lumen_widgets_style
    LIBRARY DESTINATION plugins/styles
    RUNTIME DESTINATION plugins/styles
)

# ── qtos platform theme plugin ──────────────────────────────────────────────
qt_add_plugin(qtos_platformtheme
    CLASS_NAME QtosPlatformThemePlugin
    PLUGIN_TYPE platformthemes
)
target_sources(qtos_platformtheme PRIVATE theme/qtosplatformtheme.h theme/qtosplatformtheme.cpp)
target_link_libraries(qtos_platformtheme PRIVATE Qt6::Gui Qt6::GuiPrivate qtos_lumen_common qtos::compiler_settings)
set_target_properties(qtos_platformtheme PROPERTIES
    OUTPUT_NAME qtosplatformtheme
    LIBRARY_OUTPUT_DIRECTORY "${QTOS_PLUGIN_DIR}/platformthemes"
    RUNTIME_OUTPUT_DIRECTORY "${QTOS_PLUGIN_DIR}/platformthemes"
)
install(TARGETS qtos_platformtheme
    LIBRARY DESTINATION plugins/platformthemes
    RUNTIME DESTINATION plugins/platformthemes
)

# ── Gallery ─────────────────────────────────────────────────────────────────
qt_add_executable(lumen-widgets-gallery
    gallery/main.cpp
    style/lumenstyle.h style/lumenstyle.cpp
)
target_include_directories(lumen-widgets-gallery PRIVATE style)
target_link_libraries(lumen-widgets-gallery PRIVATE Qt6::Widgets qtos_lumen_common qtos::compiler_settings)
