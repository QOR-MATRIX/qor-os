// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "kit.h"

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuickTest/quicktest.h>

Q_IMPORT_QML_PLUGIN(QTOS_KitPlugin)

class Setup : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void applicationAvailable() { qtos::Kit::initialize(); }
};

QUICK_TEST_MAIN_WITH_SETUP(kit, Setup)
#include "tst_kit_qml.moc"
