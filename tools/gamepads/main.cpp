// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// qtos-gamepads — lists the game controllers QTOS sees and streams their state,
// in the standard QTOS layout. The first thing to run when a controller does
// not behave.
//
//   qtos-gamepads                 list, then stream until Ctrl+C
//   qtos-gamepads --seconds 5     stream for five seconds
//   qtos-gamepads --rumble        a short vibration on every controller
//   qtos-gamepads --light ff6b00  set the light bar (PlayStation)

#include "gamepadmanager.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

using namespace Qt::StringLiterals;
using namespace qtos::input;

namespace {

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

QString familyName(Family family)
{
    switch (family) {
    case Family::Xbox: return u"Xbox"_s;
    case Family::PlayStation: return u"PlayStation"_s;
    case Family::Nintendo: return u"Nintendo"_s;
    case Family::Generic: break;
    }
    return u"generic"_s;
}

QString describe(const GamepadInfo &info)
{
    QString text = info.name + u" · "_s + familyName(info.family) + u" · "_s + info.connection;
    if (info.vendor || info.product)
        text += u" · %1:%2"_s.arg(info.vendor, 4, 16, QChar(u'0')).arg(info.product, 4, 16, QChar(u'0'));
    if (info.battery >= 0)
        text += u" · battery %1%%2"_s.arg(info.battery).arg(info.charging ? u", charging"_s : QString());
    if (info.rumble)
        text += u" · rumble"_s;
    if (info.lightbar)
        text += u" · light bar"_s;
    return text;
}

QString describe(const GamepadState &state)
{
    static const char *names[] = { "South", "East", "West",  "North",    "LB",  "RB", "LT",
                                   "RT",    "Back", "Start", "Guide",    "LS",  "RS", "Up",
                                   "Down",  "Left", "Right", "Touchpad", "Misc" };
    QStringList pressed;
    for (int b = 0; b < int(Button::Count); ++b)
        if (state.pressed(Button(b)))
            pressed.append(QString::fromLatin1(names[b]));
    const auto f = [](float v) { return QString::asprintf("%+.2f", double(v)); };
    return u"L(%1 %2) R(%3 %4) LT %5 RT %6  %7"_s.arg(
        f(state.axis(Axis::LeftX)), f(state.axis(Axis::LeftY)), f(state.axis(Axis::RightX)),
        f(state.axis(Axis::RightY)), f(state.axis(Axis::LeftTrigger)), f(state.axis(Axis::RightTrigger)),
        pressed.isEmpty() ? u"-"_s : pressed.join(u' '));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"qtos-gamepads"_s);
    QCoreApplication::setApplicationVersion(QStringLiteral(QTOS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Lists the game controllers QTOS sees and streams their state."_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption seconds(u"seconds"_s, u"Stream for this many seconds, then exit."_s, u"n"_s);
    const QCommandLineOption rumble(u"rumble"_s, u"Vibrate every controller briefly."_s);
    const QCommandLineOption light(u"light"_s, u"Light bar colour, e.g. ff6b00."_s, u"rrggbb"_s);
    parser.addOptions({ seconds, rumble, light });
    parser.process(app);

    GamepadManager *manager = GamepadManager::instance();
    out() << "Backends: " << manager->backendNames().join(u", "_s) << '\n';
    out().flush();

    QObject::connect(manager, &GamepadManager::connected, &app, [](const GamepadInfo &info) {
        out() << "+ " << describe(info) << '\n';
        out().flush();
    });
    QObject::connect(manager, &GamepadManager::infoChanged, &app, [](const GamepadInfo &info) {
        out() << "~ " << describe(info) << '\n';
        out().flush();
    });
    QObject::connect(manager, &GamepadManager::disconnected, &app, [](const GamepadInfo &info) {
        out() << "- " << info.name << '\n';
        out().flush();
    });
    QObject::connect(
        manager, &GamepadManager::stateChanged, &app, [manager](const QString &id, const GamepadState &state) {
            out() << "  " << manager->gamepad(id).value_or(GamepadInfo {}).name << ": " << describe(state) << '\n';
            out().flush();
        });

    // Give the backends a moment to find what is already connected.
    QTimer::singleShot(400, &app, [&] {
        if (manager->gamepads().isEmpty())
            out() << "No controllers found. Connect one; this keeps watching.\n";
        out().flush();
        if (parser.isSet(light))
            manager->setLightColor(QColor(u"#"_s + parser.value(light)));
        if (parser.isSet(rumble))
            manager->rumble(0.7f, 0.5f, 400);
    });
    if (parser.isSet(seconds))
        QTimer::singleShot(int(parser.value(seconds).toDouble() * 1000) + 400, &app, &QCoreApplication::quit);
    return app.exec();
}
