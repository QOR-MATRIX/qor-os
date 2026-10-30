#include "look.h"

#include "tokens.h"

#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace qq {

namespace {

/// Register the faces in `files` that are there (the runtime's resources, or the Studio's), and answer with `family`,
/// the look's own name for them. Not the name a loaded file reports: on Windows Qt names Barlow Semi Condensed "Barlow",
/// the family of every width, and with Barlow itself loaded too (the Studio) "Barlow" draws the wider face. By its own
/// name each face is found, and each weight in it (measured 21 October 2026).
QString loadFace(const QStringList &files, const char *family)
{
    for (const QString &file : files) {
        if (QFile::exists(file))
            QFontDatabase::addApplicationFont(file);
    }
    return QString::fromLatin1(family);
}

/// The colours a host passed on (QOR_THEME_COLORS: a JSON object of the look's colours by name, "#rrggbb" or
/// "#aarrggbb"): Prism's, Lumen's roles in QOR OS (QOR-0016). A colour it leaves out or misspells stays the look's own.
QHash<QString, QColor> hostColors()
{
    QHash<QString, QColor> colors;
    const QJsonObject asked = QJsonDocument::fromJson(qgetenv("QOR_THEME_COLORS")).object();
    for (auto it = asked.constBegin(); it != asked.constEnd(); ++it) {
        const QColor color = QColor::fromString(it.value().toString());
        if (color.isValid())
            colors.insert(it.key(), color);
    }
    return colors;
}

}  // namespace

Look::Look(QObject *parent)
    : QObject(parent)
    , m_display(loadFace({QStringLiteral(":/qq/fonts/Cinzel.ttf")}, tokens::fontDisplay))
    , m_ui(loadFace({QStringLiteral(":/qq/fonts/BarlowSemiCondensed-Regular.ttf"),
                     QStringLiteral(":/qq/fonts/BarlowSemiCondensed-SemiBold.ttf")},
                    tokens::fontInterface))
    // Carried by the Studio only (studio/CMakeLists.txt): in the Player these are names, and text falls back.
    , m_body(loadFace({QStringLiteral(":/qq/studio-fonts/Barlow-Regular.ttf"),
                       QStringLiteral(":/qq/studio-fonts/Barlow-Medium.ttf")},
                      tokens::fontBody))
    , m_mono(loadFace({QStringLiteral(":/qq/studio-fonts/IBMPlexMono-Regular.ttf"),
                       QStringLiteral(":/qq/studio-fonts/IBMPlexMono-Medium.ttf")},
                      tokens::fontMono))
    , m_reducedMotion(systemPrefersReducedMotion())
    , m_colors(hostColors())
{
}

QColor Look::color(const QString &name, QRgb own) const
{
    return m_colors.value(name, QColor(own));
}

Look *Look::create(QQmlEngine *, QJSEngine *)
{
    return new Look;
}

QColor Look::deepest() const { return color(QStringLiteral("void"), tokens::colorVoid); }
QColor Look::base() const { return color(QStringLiteral("base"), tokens::colorBase); }
QColor Look::well() const { return color(QStringLiteral("well"), tokens::colorWell); }
QColor Look::surface() const { return color(QStringLiteral("surface"), tokens::colorSurface); }
QColor Look::raised() const { return color(QStringLiteral("raised"), tokens::colorRaised); }
QColor Look::edge() const { return color(QStringLiteral("edge"), tokens::colorEdge); }
QColor Look::edgeSoft() const { return color(QStringLiteral("edge-soft"), tokens::colorEdgeSoft); }
QColor Look::ink() const { return color(QStringLiteral("ink"), tokens::colorInk); }
QColor Look::inkBody() const { return color(QStringLiteral("ink-body"), tokens::colorInkBody); }
QColor Look::inkMuted() const { return color(QStringLiteral("ink-muted"), tokens::colorInkMuted); }
QColor Look::inkFaint() const { return color(QStringLiteral("ink-faint"), tokens::colorInkFaint); }
QColor Look::accent() const { return color(QStringLiteral("accent"), tokens::colorAccent); }
QColor Look::accentBright() const { return color(QStringLiteral("accent-bright"), tokens::colorAccentBright); }
QColor Look::accentDim() const { return color(QStringLiteral("accent-dim"), tokens::colorAccentDim); }
QColor Look::onAccent() const { return color(QStringLiteral("on-accent"), tokens::colorOnAccent); }
QColor Look::gilt() const { return color(QStringLiteral("gilt"), tokens::colorGilt); }
QColor Look::giltBright() const { return color(QStringLiteral("gilt-bright"), tokens::colorGiltBright); }
QColor Look::giltDim() const { return color(QStringLiteral("gilt-dim"), tokens::colorGiltDim); }
QColor Look::ok() const { return color(QStringLiteral("ok"), tokens::colorOk); }
QColor Look::warn() const { return color(QStringLiteral("warn"), tokens::colorWarn); }
QColor Look::bad() const { return color(QStringLiteral("bad"), tokens::colorBad); }

int Look::sizeMicro() const { return tokens::sizeMicro; }
int Look::sizeCaption() const { return tokens::sizeCaption; }
int Look::sizeUi() const { return tokens::sizeUi; }
int Look::sizeBody() const { return tokens::sizeBody; }
int Look::sizeTitle() const { return tokens::sizeTitle; }
int Look::sizeHeading() const { return tokens::sizeHeading; }
int Look::sizeDisplay() const { return tokens::sizeDisplay; }
int Look::sizeFigure() const { return tokens::sizeFigure; }
double Look::trackingHeading() const { return tokens::trackingHeading; }
double Look::trackingLabel() const { return tokens::trackingLabel; }
double Look::trackingEyebrow() const { return tokens::trackingEyebrow; }
int Look::radiusControl() const { return tokens::radiusControl; }
int Look::radiusPanel() const { return tokens::radiusPanel; }
int Look::quick() const { return m_reducedMotion ? 0 : tokens::motionQuick; }
int Look::standard() const { return m_reducedMotion ? 0 : tokens::motionStandard; }
int Look::enter() const { return m_reducedMotion ? 0 : tokens::motionEnter; }
int Look::sheen() const { return m_reducedMotion ? 0 : tokens::motionSheen; }
int Look::focusWidth() const { return tokens::focusWidth; }
int Look::focusHalo() const { return tokens::focusHalo; }
QColor Look::focus() const { return color(QStringLiteral("accent-bright"), tokens::focusColor); }

QVariantList Look::ease() const
{
    // Easing.BezierSpline takes the two control points and then the end point.
    return {tokens::easeBezier[0], tokens::easeBezier[1], tokens::easeBezier[2], tokens::easeBezier[3], 1.0, 1.0};
}

void Look::setReducedMotion(bool reduced)
{
    if (reduced == m_reducedMotion)
        return;
    m_reducedMotion = reduced;
    emit reducedMotionChanged();
}

bool Look::systemPrefersReducedMotion()
{
    // The launcher passes on the person's own Motion setting to what it starts (ADR-108 decision 5).
    const QByteArray asked = qgetenv("QOR_REDUCED_MOTION").trimmed().toLower();
    if (asked == "1" || asked == "true")
        return true;
    if (asked == "0" || asked == "false")
        return false;
#ifdef Q_OS_WIN
    // Settings > Accessibility > Visual effects > Animation effects: off means the person wants less motion.
    BOOL animate = TRUE;
    if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animate, 0))
        return !animate;
#endif
    return false;
}

}  // namespace qq
