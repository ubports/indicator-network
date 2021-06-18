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

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <memory>

#include <nmofono/connection/active-connection-manager.h>
#include <nmofono/vpn/openvpn-connection.h>
#include <nmofono/vpn/pptp-connection.h>

#include <unity/util/DefinesPtrs.h>

namespace nmofono
{
namespace vpn
{

class VpnConnection: public QObject
{
    Q_OBJECT

public:
    UNITY_DEFINES_PTRS(VpnConnection);

    enum class Type
    {
        openvpn,
        pptp
    };

    VpnConnection(const QDBusObjectPath& path, connection::ActiveConnectionManager::SPtr activeConnectionManager, const QDBusConnection& systemConnection);

    ~VpnConnection() = default;

    QString uuid() const;

    QString id() const;

    bool neverDefault() const;

    QDBusObjectPath path() const;

    bool isActive() const;

    bool isValid() const;

    bool isBusy() const;

    bool isActivatable() const;

    Type type() const;

    OpenvpnConnection::SPtr openvpnConnection() const;

    PptpConnection::SPtr pptpConnection() const;

    static const char * TypeString(Type type) {
        switch (type) {
            case Type::openvpn:     return "openvpn";
            case Type::pptp:        return "pptp";
            default:                return "";
	}
    }

public Q_SLOTS:
    void setActive(bool active);

    void setId(const QString& id);

    void setNeverDefault(bool neverDefault);

    void setOtherConnectionIsBusy(bool otherConnectionIsBusy);

    void setActiveConnectionPath(const QDBusObjectPath& path);

    void updateSecrets();

    void remove();

Q_SIGNALS:
    void idChanged(const QString& id);

    void neverDefaultChanged(bool neverDefault);

    void activeChanged(bool active);

    void validChanged(bool valid);

    void busyChanged(bool busy);

    void activatableChanged(bool activatable);

    void activateConnection(const QDBusObjectPath& connection);

    void deactivateConnection(const QDBusObjectPath& activeConnection);

protected:
    class Priv;
    std::shared_ptr<Priv> d;
};

}
}

