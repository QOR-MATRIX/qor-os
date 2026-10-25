// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// qtosctl — inspect and drive a running QTOS session from the command line.
//
// Talks to the system services over Qt Remote Objects, exactly like the shell
// does, so it works against qtos-shell (embedded services), qtos-simd, and —
// for the non-simulation commands — the real daemons on a device.
//
//   qtosctl status
//   qtosctl theme light
//   qtosctl accent 205
//   qtosctl notify "Build finished" "All 42 tests passed"
//   qtosctl wifi off
//   qtosctl battery 12            # simulation only
//   qtosctl watch                 # stream notifications and state changes
//   qtosctl spectrum              # live view of what the audio engine hears

#include "rep_audio_replica.h"
#include "rep_media_replica.h"
#include "rep_network_replica.h"
#include "rep_notifications_replica.h"
#include "rep_power_replica.h"
#include "rep_settings_replica.h"
#include "rep_simcontrol_replica.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtRemoteObjects/QRemoteObjectNode>

#include <algorithm>
#include <cmath>
#include <memory>

using namespace Qt::StringLiterals;

namespace {

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

QUrl defaultUrl()
{
#if defined(Q_OS_LINUX)
    return QUrl(u"localabstract:qtos-services"_s);
#else
    return QUrl(u"local:qtos-services"_s);
#endif
}

template<typename Replica>
std::unique_ptr<Replica> acquire(QRemoteObjectNode &node, int timeoutMs = 2000)
{
    std::unique_ptr<Replica> replica(node.acquire<Replica>());
    if (!replica->waitForSource(timeoutMs)) {
        err() << "qtosctl: service " << Replica::staticMetaObject.className()
              << " is not available (is qtos-shell or qtos-simd running?)\n";
        err().flush();
        return nullptr;
    }
    return replica;
}

bool parseOnOff(const QString &value, bool *ok)
{
    *ok = value == u"on"_s || value == u"off"_s;
    return value == u"on"_s;
}

// Gives queued remote calls and property pushes time to reach the source.
int flushAndExit(QCoreApplication &app, int code = 0)
{
    QTimer::singleShot(250, &app, [code] { QCoreApplication::exit(code); });
    return app.exec();
}

QString formatTime(double seconds)
{
    if (!(seconds > 0))
        return u"--:--"_s;
    const int s = int(seconds);
    return QString::number(s / 60) + u':' + QString::number(s % 60).rightJustified(2, u'0');
}

// One line of block characters for the 48 analysis bands, low to high.
QString spectrumLine(const QVariantList &bands)
{
    static const QString blocks = u" ▁▂▃▄▅▆▇█"_s;
    QString line;
    for (const QVariant &band : bands)
        line += blocks.at(std::clamp(qRound(band.toDouble() * 8), 0, 8));
    return line;
}

int usage(const QCommandLineParser &parser)
{
    err() << parser.helpText();
    err().flush();
    return 2;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"qtosctl"_s);
    QCoreApplication::setApplicationVersion(QStringLiteral(QTOS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(uR"(Inspect and drive a running QTOS session.

Commands:
  status                         Print system state
  theme light|dark               Set the appearance
  accent <hue>                   Set the accent hue (0-360, OKLCH degrees)
  motion on|off                  Reduce motion
  transparency on|off            Reduce transparency
  contrast on|off                Increase contrast
  wifi on|off                    Toggle Wi-Fi
  connect <ssid>                 Join a Wi-Fi network
  volume <0-100>                 Set output volume
  media play|pause|next|prev     Control the media session
  media seek <seconds>           Seek in the current track
  eq <preset>                    Equaliser preset (Flat, "Bass Boost", ...)
  sounds on|off                  Interface sounds
  reactive on|off                Audio-reactive interface
  reactive intensity <0-100>     How strongly it reacts
  reactive range <low> <high>    The frequencies it reacts to, in Hz
  spectrum [seconds]             Live spectrum, level and beats of what plays
  dnd on|off                     Do Not Disturb
  notify <title> [body]          Post a notification
  watch                          Stream notifications and state changes
  battery <0-100> [charging]     (simulation) Set the battery level
  scenario replay                (simulation) Replay scenario notifications)"_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption urlOption(u"url"_s, u"Services endpoint (default: platform default)."_s, u"url"_s);
    const QCommandLineOption appOption(u"app"_s, u"Sender app id for notify (default: system)."_s, u"id"_s,
                                       u"system"_s);
    parser.addOptions({ urlOption, appOption });
    parser.addPositionalArgument(u"command"_s, u"Command to run."_s);
    parser.addPositionalArgument(u"args"_s, u"Command arguments."_s, u"[args...]"_s);
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty())
        return usage(parser);
    const QString command = args.at(0);
    const QString arg1 = args.value(1);

    QRemoteObjectNode node;
    const QUrl url = parser.isSet(urlOption) ? QUrl(parser.value(urlOption)) : defaultUrl();
    if (!node.connectToNode(url)) {
        err() << "qtosctl: cannot connect to " << url.toString() << '\n';
        return 1;
    }

    bool ok = false;

    if (command == u"status"_s) {
        auto settings = acquire<SettingsReplica>(node);
        auto network = acquire<NetworkReplica>(node);
        auto power = acquire<PowerReplica>(node);
        auto audio = acquire<AudioReplica>(node);
        auto media = acquire<MediaReplica>(node);
        auto notifications = acquire<NotificationsReplica>(node);
        if (!settings || !network || !power || !audio || !media || !notifications)
            return 1;
        out() << "QTOS session at " << url.toString() << '\n'
              << "  user          " << settings->userName() << '@' << settings->hostName() << '\n'
              << "  appearance    " << (settings->darkMode() ? "dark" : "light") << ", accent " << settings->accentHue()
              << "°" << (settings->reduceMotion() ? ", reduced motion" : "")
              << (settings->reduceTransparency() ? ", reduced transparency" : "")
              << (settings->increaseContrast() ? ", increased contrast" : "") << '\n'
              << "  wi-fi         "
              << (!network->wifiEnabled() ? u"off"_s
                  : network->online()
                      ? network->activeSsid() + u" ("_s + QString::number(network->signalBars()) + u"/3)"_s
                      : u"disconnected"_s)
              << '\n'
              << "  battery       " << power->batteryLevel() << '%' << (power->charging() ? " charging" : "")
              << ", profile " << power->profile() << '\n'
              << "  audio         " << qRound(audio->volume() * 100) << '%' << (audio->muted() ? " muted" : "") << " → "
              << audio->outputDevice() << (audio->outputAvailable() ? "" : " (silent)") << ", EQ "
              << audio->equalizerPreset() << ", interface sounds " << (audio->systemSounds() ? "on" : "off") << '\n'
              << "  reactive      "
              << (settings->audioReactive()
                      ? u"on, intensity %1%, %2-%3 Hz"_s.arg(qRound(settings->reactiveIntensity() * 100))
                            .arg(qRound(settings->reactiveLowHz()))
                            .arg(qRound(settings->reactiveHighHz()))
                      : u"off"_s)
              << '\n'
              << "  media         " << (media->playing() ? "▶ " : "⏸ ") << media->title() << " — " << media->artist()
              << "  " << formatTime(media->position()) << " / " << formatTime(media->duration()) << '\n'
              << "  notifications " << notifications->unreadCount() << " unread"
              << (notifications->doNotDisturb() ? ", Do Not Disturb" : "") << '\n';
        out().flush();
        return 0;
    }

    if (command == u"theme"_s || command == u"accent"_s || command == u"motion"_s || command == u"transparency"_s
        || command == u"contrast"_s) {
        auto settings = acquire<SettingsReplica>(node);
        if (!settings)
            return 1;
        if (command == u"theme"_s) {
            if (arg1 != u"light"_s && arg1 != u"dark"_s)
                return usage(parser);
            settings->setDarkMode(arg1 == u"dark"_s);
        } else if (command == u"accent"_s) {
            const double hue = arg1.toDouble(&ok);
            if (!ok)
                return usage(parser);
            settings->setAccentHue(std::fmod(std::fmod(hue, 360.0) + 360.0, 360.0));
        } else {
            const bool on = parseOnOff(arg1, &ok);
            if (!ok)
                return usage(parser);
            if (command == u"motion"_s)
                settings->setReduceMotion(on);
            else if (command == u"transparency"_s)
                settings->setReduceTransparency(on);
            else
                settings->setIncreaseContrast(on);
        }
        return flushAndExit(app);
    }

    if (command == u"wifi"_s || command == u"connect"_s) {
        auto network = acquire<NetworkReplica>(node);
        if (!network)
            return 1;
        if (command == u"connect"_s) {
            if (arg1.isEmpty())
                return usage(parser);
            network->connectTo(arg1);
        } else {
            const bool on = parseOnOff(arg1, &ok);
            if (!ok)
                return usage(parser);
            network->setWifiEnabled(on);
        }
        return flushAndExit(app);
    }

    if (command == u"volume"_s) {
        auto audio = acquire<AudioReplica>(node);
        const int level = arg1.toInt(&ok);
        if (!audio || !ok)
            return audio ? usage(parser) : 1;
        audio->setVolume(std::clamp(level, 0, 100) / 100.0);
        return flushAndExit(app);
    }

    if (command == u"media"_s) {
        auto media = acquire<MediaReplica>(node);
        if (!media)
            return 1;
        if (arg1 == u"play"_s || arg1 == u"pause"_s)
            media->setPlaying(arg1 == u"play"_s);
        else if (arg1 == u"next"_s)
            media->next();
        else if (arg1 == u"prev"_s)
            media->previous();
        else if (arg1 == u"seek"_s && args.size() > 2)
            media->seek(args.at(2).toDouble());
        else
            return usage(parser);
        return flushAndExit(app);
    }

    if (command == u"eq"_s || command == u"sounds"_s) {
        auto audio = acquire<AudioReplica>(node);
        if (!audio)
            return 1;
        if (command == u"eq"_s) {
            if (!audio->equalizerPresets().contains(arg1)) {
                err() << "qtosctl: presets are " << audio->equalizerPresets().join(u", "_s) << '\n';
                return 2;
            }
            audio->applyEqualizerPreset(arg1);
        } else {
            const bool on = parseOnOff(arg1, &ok);
            if (!ok)
                return usage(parser);
            audio->setSystemSounds(on);
        }
        return flushAndExit(app);
    }

    if (command == u"reactive"_s) {
        auto settings = acquire<SettingsReplica>(node);
        if (!settings)
            return 1;
        if (arg1 == u"intensity"_s) {
            const int level = args.value(2).toInt(&ok);
            if (!ok)
                return usage(parser);
            settings->setReactiveIntensity(std::clamp(level, 0, 100) / 100.0);
        } else if (arg1 == u"range"_s) {
            bool highOk = false;
            const double low = args.value(2).toDouble(&ok);
            const double high = args.value(3).toDouble(&highOk);
            if (!ok || !highOk || low <= 0 || high <= low)
                return usage(parser);
            settings->setReactiveLowHz(low);
            settings->setReactiveHighHz(high);
        } else {
            const bool on = parseOnOff(arg1, &ok);
            if (!ok)
                return usage(parser);
            settings->setAudioReactive(on);
        }
        return flushAndExit(app);
    }

    if (command == u"spectrum"_s) {
        auto audio = acquire<AudioReplica>(node);
        if (!audio)
            return 1;
        out() << "30 Hz" << QString(48 - 11, u' ') << "16 kHz   level  range  beats\n";
        out().flush();
        int frames = 0;
        QObject::connect(audio.get(), &AudioReplica::analysis, &app,
                         [&frames](const QVariantList &bands, double level, double reactive, int beat, double) {
                             // About 20 lines a second is plenty for a terminal.
                             if (frames++ % 3 != 0)
                                 return;
                             out() << '\r' << spectrumLine(bands) << "  "
                                   << QString::number(qRound(level * 100)).rightJustified(4) << "%  "
                                   << QString::number(qRound(reactive * 100)).rightJustified(4) << "%  " << beat
                                   << "   ";
                             out().flush();
                         });
        if (const int seconds = arg1.toInt(&ok); ok && seconds > 0) {
            QTimer::singleShot(seconds * 1000, &app, [&frames] {
                out() << '\n' << frames << " analysis frames\n";
                out().flush();
                QCoreApplication::exit(frames > 0 ? 0 : 3);
            });
        }
        return app.exec();
    }

    if (command == u"dnd"_s || command == u"notify"_s) {
        auto notifications = acquire<NotificationsReplica>(node);
        if (!notifications)
            return 1;
        if (command == u"dnd"_s) {
            const bool on = parseOnOff(arg1, &ok);
            if (!ok)
                return usage(parser);
            notifications->setDoNotDisturb(on);
        } else {
            if (arg1.isEmpty())
                return usage(parser);
            notifications->notify(parser.value(appOption), arg1, args.value(2), {});
        }
        return flushAndExit(app);
    }

    if (command == u"battery"_s || command == u"scenario"_s) {
        auto control = acquire<SimControlReplica>(node);
        if (!control) {
            err() << "qtosctl: '" << command << "' is only available in simulation\n";
            return 1;
        }
        if (command == u"battery"_s) {
            const int level = arg1.toInt(&ok);
            if (!ok)
                return usage(parser);
            control->setBattery(level, args.value(2) == u"charging"_s);
        } else if (arg1 == u"replay"_s) {
            control->replayScenario();
        } else {
            return usage(parser);
        }
        return flushAndExit(app);
    }

    if (command == u"watch"_s) {
        auto notifications = acquire<NotificationsReplica>(node);
        auto settings = acquire<SettingsReplica>(node);
        auto media = acquire<MediaReplica>(node);
        if (!notifications || !settings || !media)
            return 1;
        out() << "Watching " << url.toString() << " (Ctrl+C to stop)\n";
        out().flush();
        QObject::connect(notifications.get(), &NotificationsReplica::posted, &app,
                         [](int id, const QString &appId, const QString &title, const QString &body, const QString &) {
                             out() << "[notify #" << id << "] " << appId << ": " << title
                                   << (body.isEmpty() ? QString() : u" — "_s + body) << '\n';
                             out().flush();
                         });
        QObject::connect(settings.get(), &SettingsReplica::darkModeChanged, &app, [](bool dark) {
            out() << "[settings] appearance → " << (dark ? "dark" : "light") << '\n';
            out().flush();
        });
        QObject::connect(settings.get(), &SettingsReplica::accentHueChanged, &app, [](double hue) {
            out() << "[settings] accent → " << hue << "°\n";
            out().flush();
        });
        QObject::connect(media.get(), &MediaReplica::titleChanged, &app, [&media](const QString &title) {
            out() << "[media] now playing " << title << " — " << media->artist() << '\n';
            out().flush();
        });
        return app.exec();
    }

    err() << "qtosctl: unknown command '" << command << "'\n\n";
    return usage(parser);
}
