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
 *     Antti Kaijanmäki <antti.kaijanmaki@canonical.com>
 *     Pete Woods <pete.woods@canonical.com>
 */

#pragma once

#include <nmofono/manager.h>
#include <nmofono/vpn/vpn-manager.h>

#include <QDBusContext>
#include <QDBusConnection>
#include <QObject>

class NetworkingStatusAdaptor;
class PrivateAdaptor;

namespace connectivity_service
{
class PrivateService;

class ConnectivityService: public QObject, protected QDBusContext
{
    Q_OBJECT

    friend NetworkingStatusAdaptor;
    friend PrivateService;

public:
    ConnectivityService(nmofono::Manager::Ptr manager,
                        nmofono::vpn::VpnManager::SPtr vpnManager,
                        const QDBusConnection& connection);
    virtual ~ConnectivityService();

public:
    Q_PROPERTY(QStringList Limitations READ limitations)
    QStringList limitations() const;

    Q_PROPERTY(QString Status READ status)
    QString status() const;

    Q_PROPERTY(bool WifiEnabled READ wifiEnabled)
    bool wifiEnabled() const;

    Q_PROPERTY(bool FlightMode READ flightMode)
    bool flightMode() const;

    Q_PROPERTY(bool FlightModeSwitchEnabled READ flightModeSwitchEnabled)
    bool flightModeSwitchEnabled() const;

    Q_PROPERTY(bool WifiSwitchEnabled READ wifiSwitchEnabled)
    bool wifiSwitchEnabled() const;

    Q_PROPERTY(bool HotspotSwitchEnabled READ hotspotSwitchEnabled)
    bool hotspotSwitchEnabled() const;

    Q_PROPERTY(bool ModemAvailable READ modemAvailable)
    bool modemAvailable() const;

    Q_PROPERTY(bool HotspotEnabled READ hotspotEnabled)
    bool hotspotEnabled() const;

    Q_PROPERTY(QByteArray HotspotSsid READ hotspotSsid)
    QByteArray hotspotSsid() const;

    Q_PROPERTY(bool HotspotStored READ hotspotStored)
    bool hotspotStored() const;

    Q_PROPERTY(QString HotspotMode READ hotspotMode)
    QString hotspotMode() const;

private:
    class Private;
    std::shared_ptr<Private> d;
};

class PrivateService : public QObject, protected QDBusContext
{
    Q_OBJECT

    friend PrivateAdaptor;

public:
    PrivateService(ConnectivityService& parent);

    ~PrivateService() = default;

    Q_PROPERTY(QString HotspotPassword READ hotspotPassword)
    QString hotspotPassword() const;

    Q_PROPERTY(QString HotspotAuth READ hotspotAuth)
    QString hotspotAuth() const;

    Q_PROPERTY(QList<QDBusObjectPath> VpnConnections READ vpnConnections)
    QList<QDBusObjectPath> vpnConnections() const;

    Q_PROPERTY(bool MobileDataEnabled READ mobileDataEnabled WRITE setMobileDataEnabled)
    bool mobileDataEnabled() const;

    Q_PROPERTY(QDBusObjectPath SimForMobileData READ simForMobileData WRITE setSimForMobileData)
    QDBusObjectPath simForMobileData() const;

    Q_PROPERTY(QList<QDBusObjectPath> Modems READ modems)
    QList<QDBusObjectPath> modems() const;

    Q_PROPERTY(QList<QDBusObjectPath> Sims READ sims)
    QList<QDBusObjectPath> sims() const;

protected Q_SLOTS:
    void UnlockAllModems();

    void UnlockModem(const QString &modem);

    void SetFlightMode(bool enabled);

    void SetWifiEnabled(bool enabled);

    void SetHotspotEnabled(bool enabled);

    void SetHotspotSsid(const QByteArray &ssid);

    void SetHotspotPassword(const QString &password);

    void SetHotspotMode(const QString &mode);

    void SetHotspotAuth(const QString &auth);

    QDBusObjectPath AddVpnConnection(int type);

    QDBusObjectPath ImportVpnConnection(int type, const QString &filepath);

    void RemoveVpnConnection(const QDBusObjectPath &path);

    void setMobileDataEnabled(bool enabled);

    void setSimForMobileData(const QDBusObjectPath &path);


Q_SIGNALS:
    void ReportError(int reason);

protected:
    ConnectivityService& p;
};

}
