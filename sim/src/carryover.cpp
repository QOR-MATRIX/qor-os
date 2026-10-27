// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "carryover.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

Q_LOGGING_CATEGORY(lcCarryOver, "qor.sim.carryover")

namespace qor::sim {

void carryOverFrom(const QString &oldApplicationName)
{
    QSettings now;
    if (now.allKeys().isEmpty()) {
        const QSettings before(QCoreApplication::organizationName(), oldApplicationName);
        const QStringList keys = before.allKeys();
        for (const QString &key : keys)
            now.setValue(key, before.value(key));
        if (!keys.isEmpty())
            qCInfo(lcCarryOver) << "Carried" << keys.size() << "settings over from" << oldApplicationName;
    }

    // AppDataLocation ends in the application's name, so the old folder is its sibling.
    const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString old = QFileInfo(data).dir().filePath(oldApplicationName);
    if (!QFileInfo::exists(data) && QFileInfo(old).isDir()) {
        if (QDir().rename(old, data))
            qCInfo(lcCarryOver) << "Moved" << old << "to" << data;
        else
            qCWarning(lcCarryOver) << "Could not move" << old << "to" << data;
    }
}

} // namespace qor::sim
