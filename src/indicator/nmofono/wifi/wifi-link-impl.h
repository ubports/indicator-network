/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Antti Kaijanmäki <antti.kaijanmaki@canonical.com>
 */

#pragma once

#include <nmofono/kill-switch.h>
#include <nmofono/wifi/wifi-link.h>
#include <util/qhash-sharedptr.h>

#include <NetworkManagerInterface.h>
#include <NetworkManagerDeviceInterface.h>

namespace nmofono
{
namespace wifi
{

class WifiLinkImpl : public WifiLink
{
    Q_OBJECT

public:

    WifiLinkImpl(std::shared_ptr<OrgFreedesktopNetworkManagerDeviceInterface> dev,
         std::shared_ptr<OrgFreedesktopNetworkManagerInterface> nm,
         KillSwitch::Ptr killSwitch);
    ~WifiLinkImpl();

    // public API
    Type type() const override;
    Id id() const override;
    QString name() const override;

    std::uint32_t characteristics() const override;
    Status status() const override;
    bool isManaged() const override;

    QSet<AccessPoint::Ptr> accessPoints() const override;
    void connect_to(AccessPoint::Ptr accessPoint) override;
    AccessPoint::Ptr activeAccessPoint() override;

    QDBusObjectPath device_path() const;

    void setDisconnectWifi(bool) override;

    Mode mode() const override;

    Signal signal() const override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

}
}
