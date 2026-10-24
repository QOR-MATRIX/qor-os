# QTOS.Kit — Lumen design tokens, theme, motion and material primitives.

set_source_files_properties(Tokens.qml Theme.qml Motion.qml Materials.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

# The icon set is defined once, in icons/icons.txt.
file(STRINGS icons/icons.txt _qtos_icon_names REGEX "^[a-z0-9-]+$")
set(QTOS_KIT_ICONS "")
foreach(_name IN LISTS _qtos_icon_names)
    list(APPEND QTOS_KIT_ICONS "icons/${_name}.svg")
endforeach()

qt_add_library(qtos_kit STATIC)

qt_add_qml_module(qtos_kit
    URI QTOS.Kit
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        Tokens.qml
        Theme.qml
        Motion.qml
        Materials.qml
        Surface.qml
        Glass.qml
        Backdrop.qml
        RoundMask.qml
        Wallpaper.qml
        Icon.qml
        AppIcon.qml
        Artwork.qml
        QtosMark.qml
        IconButton.qml
        SidebarItem.qml
        SectionHeader.qml
        Card.qml
        SegmentedControl.qml
        SearchField.qml
        Kbd.qml
        Divider.qml
    SOURCES
        src/kit.h src/kit.cpp
        src/spring.h src/spring.cpp
        src/oklchmath.h src/oklchmath.cpp
        src/oklch.h src/oklch.cpp
        src/scenerect.h src/scenerect.cpp
        src/framestats.h src/framestats.cpp
    RESOURCES
        fonts/InterVariable.ttf
        fonts/InterVariable-Italic.ttf
        fonts/JetBrainsMono-Variable.ttf
        ${QTOS_KIT_ICONS}
    DEPENDENCIES
        QtQuick
        QtQuick.Effects
)

qt_add_shaders(qtos_kit "qtos_kit_shaders"
    PREFIX "/qt/qml/QTOS/Kit"
    FILES
        shaders/surface.frag
        shaders/glass.frag
        shaders/icon.frag
        shaders/roundmask.frag
        shaders/wallpaper.frag
)

target_include_directories(qtos_kit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(qtos_kit
    PUBLIC
        Qt6::Quick
        Qt6::QuickEffects
        Qt6::Svg
    PRIVATE
        qtos::compiler_settings
)

add_subdirectory(style)
