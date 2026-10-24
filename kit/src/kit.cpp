// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "kit.h"

#include <QtCore/QLoggingCategory>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QGuiApplication>
#include <QtQml/QJSEngine>
#include <QtQml/QQmlEngine>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcKit, "qtos.kit")

namespace qtos {

namespace {

QString registerFont(const QString &resource)
{
    const int id = QFontDatabase::addApplicationFont(resource);
    if (id < 0) {
        qCWarning(lcKit) << "Failed to register font" << resource;
        return {};
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    return families.isEmpty() ? QString() : families.constFirst();
}

} // namespace

Kit::Kit(QObject *parent) : QObject(parent) { }

Kit *Kit::instance()
{
    static Kit *kit = new Kit(qApp);
    return kit;
}

Kit *Kit::create(QQmlEngine *, QJSEngine *)
{
    Kit *kit = instance();
    QJSEngine::setObjectOwnership(kit, QJSEngine::CppOwnership);
    return kit;
}

void Kit::initialize()
{
    Kit *kit = instance();
    const QString base = u":/qt/qml/QTOS/Kit/fonts/"_s;

    if (const QString ui = registerFont(base + u"InterVariable.ttf"_s); !ui.isEmpty())
        kit->m_uiFontFamily = ui;
    registerFont(base + u"InterVariable-Italic.ttf"_s);
    if (const QString mono = registerFont(base + u"JetBrainsMono-Variable.ttf"_s); !mono.isEmpty())
        kit->m_monoFontFamily = mono;

    QFont font(kit->m_uiFontFamily);
    font.setPixelSize(13);
    font.setHintingPreference(QFont::PreferNoHinting);
    QGuiApplication::setFont(font);

    qCDebug(lcKit) << "UI font" << kit->m_uiFontFamily << "mono font" << kit->m_monoFontFamily;
}

QString Kit::version() const { return QStringLiteral(QTOS_VERSION_STRING); }

void Kit::setReducedMotion(bool reduced)
{
    if (m_reducedMotion == reduced)
        return;
    m_reducedMotion = reduced;
    Q_EMIT reducedMotionChanged();
}

} // namespace qtos
