# Lumen — the QTOS Qt Quick Controls 2 style.
#
# Selected with QQuickStyle::setStyle("Lumen") (or QT_QUICK_CONTROLS_STYLE=Lumen),
# so any app that imports plain `QtQuick.Controls` renders in Lumen. Controls
# not implemented here fall back to the Basic style.

qt_add_library(qtos_lumen_style STATIC)

qt_add_qml_module(qtos_lumen_style
    URI Lumen
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        Button.qml
        CheckBox.qml
        Frame.qml
        ItemDelegate.qml
        Label.qml
        Menu.qml
        MenuItem.qml
        MenuSeparator.qml
        Pane.qml
        Popup.qml
        ProgressBar.qml
        RadioButton.qml
        ScrollBar.qml
        ScrollIndicator.qml
        ScrollView.qml
        Slider.qml
        Switch.qml
        TabBar.qml
        TabButton.qml
        TextArea.qml
        TextField.qml
        ToolButton.qml
        ToolTip.qml
    IMPORTS
        QtQuick.Controls.Basic
    DEPENDENCIES
        QtQuick
        QtQuick.Templates
        QTOS.Kit
)

target_link_libraries(qtos_lumen_style
    PUBLIC
        Qt6::QuickControls2
        qtos_kit
    PRIVATE
        qtos::compiler_settings
)
