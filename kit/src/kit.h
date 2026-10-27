// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace qor {

// Process-wide QOR.Kit state, exposed to QML as the `Kit` singleton.
//
// Call Kit::initialize() once after constructing QGuiApplication: it registers
// the bundled Inter / JetBrains Mono variable fonts and makes Inter the
// application font so every Qt control — QML or Widgets — renders in Lumen type.
class Kit : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString version READ version CONSTANT FINAL)
    Q_PROPERTY(QString uiFontFamily READ uiFontFamily CONSTANT FINAL)
    Q_PROPERTY(QString monoFontFamily READ monoFontFamily CONSTANT FINAL)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY reducedMotionChanged FINAL)

public:
    static Kit *instance();
    static Kit *create(QQmlEngine *, QJSEngine *);
    static void initialize();

    QString version() const;
    QString uiFontFamily() const { return m_uiFontFamily; }
    QString monoFontFamily() const { return m_monoFontFamily; }

    bool reducedMotion() const { return m_reducedMotion; }
    void setReducedMotion(bool reduced);

Q_SIGNALS:
    void reducedMotionChanged();

private:
    explicit Kit(QObject *parent = nullptr);

    QString m_uiFontFamily = QStringLiteral("Inter Variable");
    QString m_monoFontFamily = QStringLiteral("JetBrains Mono");
    bool m_reducedMotion = false;
};

} // namespace qor
