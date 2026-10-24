// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "kit.h"

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QtEnvironmentVariables>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuick/QQuickView>
#include <QtQuickControls2/QQuickStyle>
#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <cmath>

Q_IMPORT_QML_PLUGIN(QTOS_KitPlugin)
Q_IMPORT_QML_PLUGIN(LumenPlugin)

using namespace Qt::StringLiterals;

// Golden-image tests for QTOS.Kit and the Lumen controls.
//
// Every scene in golden/scenes is rendered on the GPU in each variant
// (appearance × contrast, plus right-to-left) and compared with
// golden/references/<scene>-<variant>.png. The comparison is perceptual:
// per-pixel distance in OKLab, where about 0.02 is one just-noticeable
// difference. Antialiasing and blur vary slightly between GPUs, drivers and
// graphics APIs, so a case passes while only a few pixels differ visibly.
// One reference set serves every platform (Direct3D 11 on Windows and OpenGL
// on Linux differ on under 0.01% of pixels); a platform that renders a case
// differently on purpose can override it in golden/references/<os>/.
//
//   QTOS_GOLDEN_UPDATE=1          rewrite the shared references
//   QTOS_GOLDEN_UPDATE=platform   write overrides for this platform only
//
// A failing case writes <case>-actual.png and <case>-diff.png to QTOS_GOLDEN_OUT.

namespace {

struct Variant
{
    const char *name;
    bool dark;
    bool highContrast;
    bool rtl;
};

constexpr std::array kVariants {
    Variant { "dark", true, false, false },    Variant { "light", false, false, false },
    Variant { "dark-hc", true, true, false },  Variant { "light-hc", false, true, false },
    Variant { "dark-rtl", true, false, true },
};
constexpr std::array kScenes { "controls", "kit", "materials" };

// A pixel differs visibly beyond ~1.5 just-noticeable differences.
constexpr double kPixelThreshold = 0.03;
// A case fails when more than this fraction of its pixels differ visibly.
constexpr double kMaxDifferingFraction = 0.003;

QString platformKey()
{
#if defined(Q_OS_WIN)
    return u"windows"_s;
#elif defined(Q_OS_MACOS)
    return u"macos"_s;
#else
    return u"linux"_s;
#endif
}

struct Lab
{
    float l, a, b;
};

Lab toOklab(QRgb rgb)
{
    static const std::array<float, 256> decode = [] {
        std::array<float, 256> table {};
        for (int i = 0; i < 256; ++i) {
            const double x = i / 255.0;
            table[i] = float(x <= 0.04045 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4));
        }
        return table;
    }();
    const float r = decode[qRed(rgb)];
    const float g = decode[qGreen(rgb)];
    const float b = decode[qBlue(rgb)];
    const float l = std::cbrt(0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b);
    const float m = std::cbrt(0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b);
    const float s = std::cbrt(0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b);
    return { 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s,
             1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s,
             0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s };
}

struct Diff
{
    double differingFraction = 0;
    double meanDelta = 0;
    double maxDelta = 0;
    QImage heatmap;
};

// Both images must be the same size and Format_RGB32.
Diff compare(const QImage &actual, const QImage &expected)
{
    Diff diff;
    diff.heatmap = QImage(actual.size(), QImage::Format_RGB32);
    qsizetype differing = 0;
    double total = 0;
    for (int y = 0; y < actual.height(); ++y) {
        const auto *a = reinterpret_cast<const QRgb *>(actual.constScanLine(y));
        const auto *e = reinterpret_cast<const QRgb *>(expected.constScanLine(y));
        auto *out = reinterpret_cast<QRgb *>(diff.heatmap.scanLine(y));
        for (int x = 0; x < actual.width(); ++x) {
            const Lab p = toOklab(a[x]);
            const Lab q = toOklab(e[x]);
            const double delta = std::sqrt(double(p.l - q.l) * (p.l - q.l) + double(p.a - q.a) * (p.a - q.a)
                                           + double(p.b - q.b) * (p.b - q.b));
            total += delta;
            diff.maxDelta = std::max(diff.maxDelta, delta);
            // The heatmap shows the actual image dimmed, with visible
            // differences in red (brighter for larger differences).
            const int gray = qGray(a[x]) / 3;
            if (delta > kPixelThreshold) {
                ++differing;
                const int red = 120 + int(std::min(1.0, delta / (4 * kPixelThreshold)) * 135);
                out[x] = qRgb(red, gray / 2, gray / 2);
            } else {
                out[x] = qRgb(gray, gray, gray);
            }
        }
    }
    const double pixels = double(actual.width()) * actual.height();
    diff.differingFraction = differing / pixels;
    diff.meanDelta = total / pixels;
    return diff;
}

// Grabs the window once two consecutive frames are identical, so images,
// shaders and springs have settled.
QImage settledGrab(QQuickView &view)
{
    QImage previous = view.grabWindow();
    int stable = 0;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000) {
        QTest::qWait(60);
        QImage next = view.grabWindow();
        stable = next == previous ? stable + 1 : 0;
        previous = std::move(next);
        if (stable >= 2)
            break;
    }
    return previous.convertToFormat(QImage::Format_RGB32);
}

} // namespace

class tst_Golden : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void render_data()
    {
        QTest::addColumn<QString>("scene");
        QTest::addColumn<int>("variant");
        for (const char *scene : kScenes)
            for (int v = 0; v < int(kVariants.size()); ++v)
                QTest::addRow("%s-%s", scene, kVariants[v].name) << QString::fromLatin1(scene) << v;
    }

    void render()
    {
        QFETCH(QString, scene);
        QFETCH(int, variant);
        const Variant &v = kVariants[variant];
        const QString name = scene + u'-' + QString::fromLatin1(v.name);

        QQuickView view;
        view.setFlags(Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
        view.setResizeMode(QQuickView::SizeViewToRootObject);

        // A fresh engine per case: the Theme singleton starts clean, and with
        // Reduce Motion its springs jump straight to the variant's values.
        auto *theme = view.engine()->singletonInstance<QObject *>("QTOS.Kit", "Theme");
        QVERIFY(theme);
        theme->setProperty("reduceMotion", true);
        theme->setProperty("dark", v.dark);
        theme->setProperty("increaseContrast", v.highContrast);

        view.setInitialProperties({ { u"rtl"_s, v.rtl } });
        view.setSource(QUrl::fromLocalFile(QStringLiteral(QTOS_GOLDEN_DIR) + u"/scenes/"_s + scene + u".qml"_s));
        if (view.status() != QQuickView::Ready) {
            QStringList errors;
            for (const QQmlError &error : view.errors())
                errors << error.toString();
            QFAIL(qPrintable(errors.join(u'\n')));
        }
        view.setPosition(40, 40);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        const QImage actual = settledGrab(view);
        QVERIFY(!actual.isNull());

        const QString references = QStringLiteral(QTOS_GOLDEN_DIR) + u"/references/"_s;
        const QString sharedPath = references + name + u".png"_s;
        const QString platformPath = references + platformKey() + u'/' + name + u".png"_s;

        if (const QByteArray update = qgetenv("QTOS_GOLDEN_UPDATE"); update == "1" || update == "platform") {
            const QString path = update == "platform" ? platformPath : sharedPath;
            QDir().mkpath(QFileInfo(path).absolutePath());
            QVERIFY(actual.save(path));
            qInfo("Updated %s", qPrintable(QDir::toNativeSeparators(path)));
            return;
        }

        const QString referencePath = QFileInfo::exists(platformPath) ? platformPath : sharedPath;
        const QImage expected = QImage(referencePath).convertToFormat(QImage::Format_RGB32);
        if (expected.isNull())
            QSKIP(qPrintable(u"No reference %1. Run with QTOS_GOLDEN_UPDATE=1 (see docs/DEVELOPMENT.md)."_s.arg(
                QDir::toNativeSeparators(sharedPath))));

        const auto writeFailure = [&](const QImage *heatmap) {
            const QString out = QStringLiteral(QTOS_GOLDEN_OUT);
            QDir().mkpath(out);
            actual.save(out + u'/' + name + u"-actual.png"_s);
            if (heatmap)
                heatmap->save(out + u'/' + name + u"-diff.png"_s);
            return QDir::toNativeSeparators(out);
        };

        if (actual.size() != expected.size()) {
            const QString out = writeFailure(nullptr);
            QFAIL(qPrintable(u"Size %1x%2 differs from the reference %3x%4 (device pixel ratio?). Actual image in %5"_s
                                 .arg(actual.width())
                                 .arg(actual.height())
                                 .arg(expected.width())
                                 .arg(expected.height())
                                 .arg(out)));
        }

        const Diff diff = compare(actual, expected);
        const QString summary = u"%1% of pixels differ visibly (limit %2%), mean dE %3, max dE %4"_s
                                    .arg(diff.differingFraction * 100, 0, 'f', 3)
                                    .arg(kMaxDifferingFraction * 100, 0, 'f', 1)
                                    .arg(diff.meanDelta, 0, 'f', 4)
                                    .arg(diff.maxDelta, 0, 'f', 3);
        if (diff.differingFraction > kMaxDifferingFraction) {
            const QString out = writeFailure(&diff.heatmap);
            QFAIL(qPrintable(summary + u". Actual and diff images in "_s + out));
        }
        qInfo("%s: %s", qPrintable(name), qPrintable(summary));
    }
};

int main(int argc, char **argv)
{
    // References are captured at 1:1; display scaling must not change the size.
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    QGuiApplication app(argc, argv);
    qtos::Kit::initialize();
    QQuickStyle::setStyle(u"Lumen"_s);
    tst_Golden test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "tst_golden.moc"
