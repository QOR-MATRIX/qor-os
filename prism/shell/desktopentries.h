// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace qor {

// QML singleton `DesktopEntries`: the identity of a Wayland app.
//
// Looks up the app's freedesktop.org desktop entry for its xdg_toplevel app
// id (`<appId>.desktop` in the XDG applications directories, else an entry
// whose StartupWMClass is the app id) and resolves its icon through the XDG
// icon theme directories. Results are cached. Without desktop entries (for
// example on Windows), every lookup reports `found: false`.
class DesktopEntries : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit DesktopEntries(QObject *parent = nullptr);

    // {found: bool, name: string, icon: url (empty if none)}
    Q_INVOKABLE QVariantMap lookup(const QString &appId);

private:
    QVariantMap resolve(const QString &appId) const;
    QStringList entriesFor(const QString &wmClass) const;

    QHash<QString, QVariantMap> m_cache;
    // Which desktop files name which StartupWMClass, read at most once a minute.
    mutable QHash<QString, QStringList> m_byWmClass;
    mutable QElapsedTimer m_scanned;
};

} // namespace qor
