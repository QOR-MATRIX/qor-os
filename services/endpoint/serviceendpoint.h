// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Where the service bus listens: the one place that says so, for the hosts and
// for every client.
//
// Qt Remote Objects has no authentication of its own, and takes what it is sent
// on trust, so whoever can reach the socket can do what the shell can, and can
// crash the process hosting it. On Linux the socket is therefore a file in the
// person's runtime folder, which only they can enter. An abstract socket, which
// it was until QOR-0018, takes no permissions at all: any user on the machine,
// and any app sharing its network namespace, could connect.
#pragma once

#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QUrl>

namespace qor::services {

inline QUrl endpoint(const QString &name)
{
    QUrl url;
    url.setScheme(QStringLiteral("local"));
#if defined(Q_OS_LINUX)
    // Qt checks the folder is this user's, and not open to others, before it
    // gives it. Without one there is no endpoint, and hosting one fails.
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (folder.isEmpty())
        return {};
    url.setPath(folder + QLatin1Char('/') + name);
#else
    url.setPath(name);
#endif
    return url;
}

inline QUrl defaultEndpoint() { return endpoint(QStringLiteral("qor-services")); }

} // namespace qor::services
