/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Pete Woods <pete.woods@canonical.com>
 */

#pragma once

#include <nmofono/connection/active-connection-manager.h>
#include <nmofono/vpn/vpn-connection.h>
#include <QDBusConnection>

namespace nmofono
{
namespace vpn
{

class VpnManager: public QObject
{
    Q_OBJECT

public:
    UNITY_DEFINES_PTRS(VpnManager);

    VpnManager(connection::ActiveConnectionManager::SPtr activeConnectionManager, const QDBusConnection& systemConnection);

    ~VpnManager() = default;

    QList<VpnConnection::SPtr> connections() const;

    QSet<QDBusObjectPath> connectionPaths() const;

    VpnConnection::SPtr connection(const QDBusObjectPath& path) const;

    QString addConnection(VpnConnection::Type type);

    QString importConnection(QString filepath);

Q_SIGNALS:
    void connectionsChanged();

protected:
    class Priv;
    std::shared_ptr<Priv> d;
};

}
}
