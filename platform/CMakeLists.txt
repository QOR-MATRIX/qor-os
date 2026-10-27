# Platform integration: Lumen for every Qt app, not just QOR OS's own.
#
#   common/   palette + fonts from the generated Lumen tokens (Qt Gui only)
#   style/    "Lumen" QStyle plugin for Qt Widgets apps
#   theme/    "qor" QPA platform theme plugin (loaded on Linux devices)
#   gallery/  lumen-widgets-gallery, a preview of the QStyle

# The platform theme implements QPlatformTheme, a QPA (semi-private) interface.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 ${QOR_QT_MIN_VERSION} REQUIRED COMPONENTS Widgets GuiPrivate)

set(QOR_PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugins")

# ── Shared Lumen palette ────────────────────────────────────────────────────
qt_add_library(qor_lumen_common STATIC
    common/lumentokens.h
    common/lumenpalette.h common/lumenpalette.cpp
    ${CMAKE_SOURCE_DIR}/kit/src/oklchmath.h ${CMAKE_SOURCE_DIR}/kit/src/oklchmath.cpp
)
target_include_directories(qor_lumen_common PUBLIC common "${CMAKE_SOURCE_DIR}/kit/src")
target_link_libraries(qor_lumen_common PUBLIC Qt6::Gui PRIVATE qor::compiler_settings)
set_target_properties(qor_lumen_common PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── Lumen QStyle plugin ─────────────────────────────────────────────────────
qt_add_plugin(qor_lumen_widgets_style
    CLASS_NAME LumenStylePlugin
    PLUGIN_TYPE styles
)
target_sources(qor_lumen_widgets_style PRIVATE
    style/lumenstyle.h style/lumenstyle.cpp style/lumenstyleplugin.cpp
)
target_include_directories(qor_lumen_widgets_style PRIVATE style)
target_link_libraries(qor_lumen_widgets_style PRIVATE Qt6::Widgets qor_lumen_common qor::compiler_settings)
set_target_properties(qor_lumen_widgets_style PROPERTIES
    OUTPUT_NAME lumenstyle
    LIBRARY_OUTPUT_DIRECTORY "${QOR_PLUGIN_DIR}/styles"
    RUNTIME_OUTPUT_DIRECTORY "${QOR_PLUGIN_DIR}/styles"
)
install(TARGETS qor_lumen_widgets_style
    LIBRARY DESTINATION plugins/styles
    RUNTIME DESTINATION plugins/styles
)

# ── qor platform theme plugin ──────────────────────────────────────────────
qt_add_plugin(qor_platformtheme
    CLASS_NAME QorPlatformThemePlugin
    PLUGIN_TYPE platformthemes
)
target_sources(qor_platformtheme PRIVATE theme/qorplatformtheme.h theme/qorplatformtheme.cpp)
target_link_libraries(qor_platformtheme PRIVATE Qt6::Gui Qt6::GuiPrivate qor_lumen_common qor::compiler_settings)
set_target_properties(qor_platformtheme PROPERTIES
    OUTPUT_NAME qorplatformtheme
    LIBRARY_OUTPUT_DIRECTORY "${QOR_PLUGIN_DIR}/platformthemes"
    RUNTIME_OUTPUT_DIRECTORY "${QOR_PLUGIN_DIR}/platformthemes"
)
install(TARGETS qor_platformtheme
    LIBRARY DESTINATION plugins/platformthemes
    RUNTIME DESTINATION plugins/platformthemes
)

# ── Gallery ─────────────────────────────────────────────────────────────────
qt_add_executable(lumen-widgets-gallery
    gallery/main.cpp
    style/lumenstyle.h style/lumenstyle.cpp
)
target_include_directories(lumen-widgets-gallery PRIVATE style)
target_link_libraries(lumen-widgets-gallery PRIVATE Qt6::Widgets qor_lumen_common qor::compiler_settings)
