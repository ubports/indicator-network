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

#include <nmofono/manager.h>
#include <nmofono/kill-switch.h>
#include <nmofono/hotspot-manager.h>

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QVariantMap>

namespace notify
{
class NotificationManager;
}

namespace nmofono {
    class Manager;

class ManagerImpl : public Manager
{
Q_OBJECT
    class Private;
    struct State;
    std::shared_ptr<Private> d;

    void updateNetworkingStatus(uint state);

public:
    typedef std::shared_ptr<ManagerImpl> Ptr;

    ManagerImpl(
            std::shared_ptr<notify::NotificationManager> notificationManager,
            KillSwitch::Ptr killSwitch,
            HotspotManager::SPtr hotspotManager,
            const QDBusConnection& systemBus);

    // Public API
    void setFlightMode(bool) override;
    bool flightMode() const override;

    bool unstoppableOperationHappening() const override;

    bool hasWifi() const override;
    bool wifiEnabled() const override;

    void setWifiEnabled(bool) override;

    bool roaming() const override;

    QSet<Link::Ptr> links() const override;
    QSet<wifi::WifiLink::Ptr> wifiLinks() const override;
    QSet<wwan::Modem::Ptr> modemLinks() const override;

    Manager::NetworkingStatus status() const override;
    std::uint32_t characteristics() const override;

    void unlockModem(wwan::Modem::Ptr modem) override;
    void unlockAllModems() override;
    void unlockModemByName(const QString &name) override;

    bool modemAvailable() const override;

    bool hotspotEnabled() const override;

    bool hotspotStored() const override;

    QByteArray hotspotSsid() const override;

    QString hotspotPassword() const override;

    QString hotspotMode() const override;

    QString hotspotAuth() const override;

    bool mobileDataEnabled() const override;

    wwan::Sim::Ptr simForMobileData() const override;

    QList<wwan::Modem::Ptr> modems() const override;

    QList<wwan::Sim::Ptr> sims() const override;

    void setHotspotEnabled(bool) override;

    void setHotspotSsid(const QByteArray&) override;

    void setHotspotPassword(const QString&) override;

    void setHotspotMode(const QString&) override;

    void setHotspotAuth(const QString&) override;

    void setMobileDataEnabled(bool) override;

    void setSimForMobileData(wwan::Sim::Ptr) override;

private Q_SLOTS:
    void device_added(const QDBusObjectPath &path);
    void device_removed(const QDBusObjectPath &path);
    void nm_properties_changed(const QVariantMap &properties);

private:
    void setMtkWifiEnabled(bool);
};

}
