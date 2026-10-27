# Lumen — the QOR OS Qt Quick Controls 2 style.
#
# Selected with QQuickStyle::setStyle("Lumen") (or QT_QUICK_CONTROLS_STYLE=Lumen),
# so any app that imports plain `QtQuick.Controls` renders in Lumen. Controls
# not implemented here fall back to the Basic style.

qt_add_library(qor_lumen_style STATIC)

qt_add_qml_module(qor_lumen_style
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
        RangeSlider.qml
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
        QOR.Kit
)

target_link_libraries(qor_lumen_style
    PUBLIC
        Qt6::QuickControls2
        qor_kit
    PRIVATE
        qor::compiler_settings
)
