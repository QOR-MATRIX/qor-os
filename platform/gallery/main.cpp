// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// lumen-widgets-gallery — previews the Lumen QStyle on classic Qt Widgets.
//
//   lumen-widgets-gallery                  # dark
//   lumen-widgets-gallery --light --accent 205
//   lumen-widgets-gallery --screenshot gallery.png

#include "lumenstyle.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QTimer>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(u"Lumen Widgets Gallery"_s);

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption light(u"light"_s, u"Light appearance."_s);
    const QCommandLineOption accent(u"accent"_s, u"Accent hue in degrees."_s, u"hue"_s);
    const QCommandLineOption screenshot(u"screenshot"_s, u"Save a screenshot and exit."_s, u"file"_s);
    const QCommandLineOption fonts(u"fonts"_s, u"Directory with Inter / JetBrains Mono to register."_s, u"dir"_s);
    parser.addOptions({ light, accent, screenshot, fonts });
    parser.process(app);

    if (parser.isSet(fonts)) {
        for (const QString &file : { u"InterVariable.ttf"_s, u"JetBrainsMono-Variable.ttf"_s })
            QFontDatabase::addApplicationFont(parser.value(fonts) + u'/' + file);
    }

    qor::lumen::Appearance appearance;
    appearance.dark = !parser.isSet(light);
    if (parser.isSet(accent))
        appearance.accentHue = parser.value(accent).toDouble();
    auto *style = new qor::LumenStyle(appearance);
    QApplication::setStyle(style);
    QApplication::setPalette(style->standardPalette());

    QWidget window;
    window.setWindowTitle(u"Lumen for Qt Widgets"_s);
    auto *root = new QVBoxLayout(&window);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    auto *title = new QLabel(u"Lumen for Qt Widgets"_s);
    QFont titleFont = title->font();
    titleFont.setPixelSize(qor::lumen::kTypeTitle1Size);
    titleFont.setWeight(QFont::Bold);
    title->setFont(titleFont);
    root->addWidget(title);
    auto *subtitle = new QLabel(
        u"Classic widgets rendered by the Lumen QStyle: same palette, type and geometry as the QML shell."_s);
    subtitle->setForegroundRole(QPalette::PlaceholderText);
    root->addWidget(subtitle);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(16);
    root->addLayout(grid);

    auto *buttons = new QGroupBox(u"Buttons"_s);
    auto *buttonsLayout = new QVBoxLayout(buttons);
    auto *primary = new QPushButton(u"Install update"_s);
    primary->setDefault(true);
    buttonsLayout->addWidget(primary);
    buttonsLayout->addWidget(new QPushButton(u"Secondary"_s));
    auto *disabled = new QPushButton(u"Disabled"_s);
    disabled->setEnabled(false);
    buttonsLayout->addWidget(disabled);
    auto *tool = new QToolButton;
    tool->setText(u"Tool button"_s);
    tool->setAutoRaise(true);
    buttonsLayout->addWidget(tool);
    grid->addWidget(buttons, 0, 0);

    auto *choices = new QGroupBox(u"Choices"_s);
    auto *choicesLayout = new QVBoxLayout(choices);
    auto *check = new QCheckBox(u"Sync over Wi-Fi only"_s);
    check->setChecked(true);
    choicesLayout->addWidget(check);
    auto *tristate = new QCheckBox(u"Partially selected"_s);
    tristate->setTristate(true);
    tristate->setCheckState(Qt::PartiallyChecked);
    choicesLayout->addWidget(tristate);
    choicesLayout->addWidget(new QCheckBox(u"Send diagnostics"_s));
    auto *radioA = new QRadioButton(u"Balanced"_s);
    radioA->setChecked(true);
    choicesLayout->addWidget(radioA);
    choicesLayout->addWidget(new QRadioButton(u"Performance"_s));
    grid->addWidget(choices, 0, 1);

    auto *inputs = new QGroupBox(u"Inputs"_s);
    auto *inputsLayout = new QVBoxLayout(inputs);
    auto *edit = new QLineEdit;
    edit->setPlaceholderText(u"Search projects"_s);
    inputsLayout->addWidget(edit);
    auto *combo = new QComboBox;
    combo->addItems({ u"Studio Speakers"_s, u"Halo Buds"_s, u"HDMI Display"_s });
    inputsLayout->addWidget(combo);
    auto *spin = new QSpinBox;
    spin->setRange(0, 100);
    spin->setValue(62);
    spin->setSuffix(u" %"_s);
    inputsLayout->addWidget(spin);
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setValue(62);
    inputsLayout->addWidget(slider);
    auto *progress = new QProgressBar;
    progress->setValue(72);
    inputsLayout->addWidget(progress);
    grid->addWidget(inputs, 0, 2);

    auto *list = new QListWidget;
    list->addItems({ u"Documents"_s, u"Pictures"_s, u"Music"_s, u"Downloads"_s, u"Projects"_s });
    list->setCurrentRow(1);
    grid->addWidget(list, 1, 0);

    auto *text = new QTextEdit;
    text->setPlainText(u"Widgets apps such as Krita, OBS Studio, Wireshark and KeePassXC pick up Lumen "
                       "automatically through the qor platform theme."_s);
    grid->addWidget(text, 1, 1, 1, 2);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    root->addWidget(box);

    window.resize(960, 620);
    window.show();

    if (parser.isSet(screenshot)) {
        QTimer::singleShot(600, &window, [&window, path = parser.value(screenshot)] {
            QCoreApplication::exit(window.grab().save(path) ? 0 : 3);
        });
    }
    return app.exec();
}
