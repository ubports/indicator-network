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

#include <connectivity-service/connectivity-service.h>
#include <connectivity-service/dbus-modem.h>
#include <connectivity-service/dbus-sim.h>
#include <connectivity-service/dbus-vpn-connection.h>
#include <connectivity-service/dbus-openvpn-connection.h>
#include <connectivity-service/dbus-pptp-connection.h>
#include <NetworkingStatusAdaptor.h>
#include <NetworkingStatusPrivateAdaptor.h>
#include <dbus-types.h>
#include <util/dbus-utils.h>

using namespace nmofono;
using namespace nmofono::vpn;
using namespace std;

namespace connectivity_service
{

class ConnectivityService::Private : public QObject
{
    Q_OBJECT
public:
    ConnectivityService& p;

    QDBusConnection m_connection;

    Manager::Ptr m_manager;

    vpn::VpnManager::SPtr m_vpnManager;

    QMap<QDBusObjectPath, DBusVpnConnection::SPtr> m_vpnConnections;

    QMap<QString, DBusModem::SPtr> m_modems;
    QMap<QString, DBusSim::SPtr> m_sims;

    shared_ptr<PrivateService> m_privateService;

    QStringList m_limitations;

    QString m_status;

    QMap<QString, QDBusMessage> m_addQueue;

    Private(ConnectivityService& parent, const QDBusConnection& connection) :
        p(parent), m_connection(connection)
    {
    }

    void notifyProperties(const QStringList& propertyNames)
    {
        DBusUtils::notifyPropertyChanged(
            m_connection,
            p,
            DBusTypes::SERVICE_PATH,
            DBusTypes::SERVICE_INTERFACE,
            propertyNames
        );
    }

    void flushProperties()
    {
        DBusUtils::flushPropertyChanges();
    }

    void notifyPrivateProperties(const QStringList& propertyNames)
    {
        DBusUtils::notifyPropertyChanged(
            m_connection,
            *m_privateService,
            DBusTypes::PRIVATE_PATH,
            DBusTypes::PRIVATE_INTERFACE,
            propertyNames
        );
    }

public Q_SLOTS:
    void flightModeUpdated()
    {
        notifyProperties({
            "FlightMode",
            "HotspotSwitchEnabled"
        });
    }

    void wifiEnabledUpdated()
    {
        notifyProperties({
            "WifiEnabled",
            "HotspotSwitchEnabled"
        });
    }

    void unstoppableOperationHappeningUpdated()
    {
        notifyProperties({
            "FlightModeSwitchEnabled",
            "WifiSwitchEnabled",
            "HotspotSwitchEnabled"
        });
        flushProperties();
    }

    void hotspotSsidUpdated()
    {
        notifyProperties({
            "HotspotSsid"
        });
    }

    void modemAvailableUpdated()
    {
        notifyProperties({
            "ModemAvailable"
        });
    }

    void hotspotEnabledUpdated()
    {
        notifyProperties({
            "HotspotEnabled"
        });
    }

    void hotspotPasswordUpdated()
    {
        // Note that this is on the private object
        notifyPrivateProperties({
            "HotspotPassword"
        });
    }

    void hotspotModeUpdated()
    {
        notifyProperties({
            "HotspotMode"
        });
    }

    void hotspotAuthUpdated()
    {
        // Note that this is on the private object
        notifyPrivateProperties({
            "HotspotAuth"
        });
    }

    void hotspotStoredUpdated()
    {
        notifyProperties({
            "HotspotStored"
        });
    }

    void mobileDataEnabledUpdated(bool value)
    {
        Q_UNUSED(value)
        notifyPrivateProperties({
            "MobileDataEnabled"
        });
    }

    void simForMobileDataUpdated()
    {
        notifyPrivateProperties({
            "SimForMobileData"
        });
    }

    void updateSims()
    {
        auto current_iccids = m_sims.keys().toSet();

        QMap<QString, nmofono::wwan::Sim::Ptr> sims;
        for (auto i : m_manager->sims())
        {
            sims[i->iccid()] = i;
        }
        auto toAdd(sims.keys().toSet());
        toAdd.subtract(current_iccids);

        auto toRemove(current_iccids);
        toRemove.subtract(sims.keys().toSet());

        for (auto iccid : toRemove)
        {
            m_sims.remove(iccid);
        }

        for (auto iccid : toAdd)
        {
            DBusSim::SPtr dbussim = make_shared<DBusSim>(sims[iccid], m_connection);
            m_sims[iccid] = dbussim;
        }

        if (!toRemove.isEmpty() || !toAdd.isEmpty())
        {
            notifyPrivateProperties({
                "Sims"
            });
            flushProperties();
        }
    }

    void updateModems()
    {
        auto current_serials = m_modems.keys().toSet();
        QMap<QString, nmofono::wwan::Modem::Ptr> modems;
        for (auto i : m_manager->modems())
        {
            modems[i->serial()] = i;
        }
        auto toAdd(modems.keys().toSet());
        toAdd.subtract(current_serials);

        auto toRemove(current_serials);
        toRemove.subtract(modems.keys().toSet());

        for (auto serial : toRemove)
        {
            m_modems.remove(serial);
        }

        for (auto serial : toAdd)
        {
            wwan::Modem::Ptr m = modems[serial];

            DBusModem::SPtr dbusmodem = make_shared<DBusModem>(m, m_connection);
            m_modems[serial] = dbusmodem;
            updateModemSimPath(dbusmodem, m->sim());
            connect(m.get(), &wwan::Modem::simUpdated, this, &Private::modemSimUpdated);
        }

        if (!toRemove.isEmpty() || !toAdd.isEmpty())
        {
            notifyPrivateProperties({
                "Modems"
            });
            flushProperties();
        }
    }

    void modemSimUpdated()
    {
        auto modem_raw = qobject_cast<wwan::Modem*>(sender());
        updateModemSimPath(m_modems[modem_raw->serial()], modem_raw->sim());
    }

    void updateModemSimPath(DBusModem::SPtr modem, wwan::Sim::Ptr sim)
    {
        if (!sim)
        {
            modem->setSim(QDBusObjectPath ("/"));
        }
        else
        {
            if (m_sims.contains (sim->iccid()))
            {
                auto dbusSim = m_sims[sim->iccid()];
                modem->setSim(dbusSim->path());
            }
            else
            {
                qWarning() << "Could not find SIM with ICCID:"
                           << sim->iccid();
            }
        }
    }

    void updateNetworkingStatus()
    {
        QStringList changed;

        QStringList old_limitations = m_limitations;
        QString old_status = m_status;

        switch (m_manager->status())
        {
            case Manager::NetworkingStatus::offline:
                m_status = "offline";
                break;
            case Manager::NetworkingStatus::connecting:
                m_status = "connecting";
                break;
            case Manager::NetworkingStatus::online:
                m_status = "online";
        }
        if (old_status != m_status)
        {
            changed << "Status";
        }

        QStringList limitations;
        auto characteristics = m_manager->characteristics();
        if ((characteristics
                & Link::Characteristics::is_bandwidth_limited) != 0)
        {
            // FIXME KNOWN TYPO
            limitations.push_back("bandwith");
        }
        m_limitations = limitations;
        if (old_limitations != m_limitations)
        {
            changed << "Limitations";
        }

        if (!changed.empty())
        {
            notifyProperties(changed);
        }
    }

    void updateVpnList()
    {
        auto current = m_vpnConnections.keys().toSet();
        auto connections = m_vpnManager->connectionPaths();

        auto toAdd(connections);
        toAdd.subtract(current);

        auto toRemove(current);
        toRemove.subtract(connections);

        for (const auto& con: toRemove)
        {
            m_vpnConnections.remove(con);
        }

        QList<QPair<QDBusMessage, QDBusObjectPath>> addReplies;

        for (const auto& path: toAdd)
        {
            auto vpn = m_vpnManager->connection(path);
            DBusVpnConnection::SPtr vpnConnection;
            switch(vpn->type())
            {
                case VpnConnection::Type::openvpn:
                    vpnConnection = make_shared<DBusOpenvpnConnection>(vpn, m_connection);
                    break;
                case VpnConnection::Type::pptp:
                    vpnConnection = make_shared<DBusPptpConnection>(vpn, m_connection);
                    break;
            }
            if (vpnConnection)
            {
                m_vpnConnections[path] = vpnConnection;
            }

            QString uuid = vpn->uuid();
            if (m_addQueue.contains(uuid))
            {
                addReplies << QPair<QDBusMessage, QDBusObjectPath>(m_addQueue.take(uuid), vpnConnection->path());
            }
        }

        if (!toRemove.isEmpty() || !toAdd.isEmpty())
        {
            notifyPrivateProperties({"VpnConnections"});
            flushProperties();
        }

        for (const auto& reply: addReplies)
        {
            m_connection.send(
                    reply.first.createReply(QVariant::fromValue(reply.second)));
        }
    }
};

ConnectivityService::ConnectivityService(Manager::Ptr manager,
                                         vpn::VpnManager::SPtr vpnManager,
                                         const QDBusConnection& connection)
    : d{new Private(*this, connection)}
{
    d->m_manager = manager;
    d->m_vpnManager = vpnManager;
    d->m_privateService = make_shared<PrivateService>(*this);

    // Memory is managed by Qt parent ownership
    new NetworkingStatusAdaptor(this);

    connect(d->m_manager.get(), &Manager::characteristicsUpdated, d.get(), &Private::updateNetworkingStatus);
    connect(d->m_manager.get(), &Manager::statusUpdated, d.get(), &Private::updateNetworkingStatus);
    connect(d->m_manager.get(), &Manager::flightModeUpdated, d.get(), &Private::flightModeUpdated);
    connect(d->m_manager.get(), &Manager::wifiEnabledUpdated, d.get(), &Private::wifiEnabledUpdated);
    connect(d->m_manager.get(), &Manager::unstoppableOperationHappeningUpdated, d.get(), &Private::unstoppableOperationHappeningUpdated);

    connect(d->m_manager.get(), &Manager::modemAvailableChanged, d.get(), &Private::modemAvailableUpdated);

    connect(d->m_manager.get(), &Manager::hotspotEnabledChanged, d.get(), &Private::hotspotEnabledUpdated);
    connect(d->m_manager.get(), &Manager::hotspotSsidChanged, d.get(), &Private::hotspotSsidUpdated);
    connect(d->m_manager.get(), &Manager::hotspotPasswordChanged, d.get(), &Private::hotspotPasswordUpdated);
    connect(d->m_manager.get(), &Manager::hotspotModeChanged, d.get(), &Private::hotspotModeUpdated);
    connect(d->m_manager.get(), &Manager::hotspotAuthChanged, d.get(), &Private::hotspotAuthUpdated);
    connect(d->m_manager.get(), &Manager::hotspotStoredChanged, d.get(), &Private::hotspotStoredUpdated);

    connect(d->m_manager.get(), &Manager::mobileDataEnabledChanged, d.get(), &Private::mobileDataEnabledUpdated);
    connect(d->m_manager.get(), &Manager::simsChanged, d.get(), &Private::updateSims);
    connect(d->m_manager.get(), &Manager::modemsChanged, d.get(), &Private::updateModems);
    connect(d->m_manager.get(), &Manager::simForMobileDataChanged, d.get(), &Private::simForMobileDataUpdated);

    connect(d->m_manager.get(), &Manager::reportError, d->m_privateService.get(), &PrivateService::ReportError);

    connect(d->m_vpnManager.get(), &vpn::VpnManager::connectionsChanged, d.get(), &Private::updateVpnList);

    d->updateSims();
    d->updateModems();
    d->updateNetworkingStatus();
    d->updateVpnList();

    if (!d->m_connection.registerObject(DBusTypes::SERVICE_PATH, this))
    {
        throw logic_error(
                "Unable to register NetworkingStatus object on DBus");
    }
    if (!d->m_connection.registerObject(DBusTypes::PRIVATE_PATH, d->m_privateService.get()))
    {
        throw logic_error(
                "Unable to register NetworkingStatus private object on DBus");
    }
    if (!d->m_connection.registerService(DBusTypes::DBUS_NAME))
    {
        throw logic_error(
                "Unable to register Connectivity service on DBus");
    }
}

ConnectivityService::~ConnectivityService()
{
    d->m_connection.unregisterService(DBusTypes::DBUS_NAME);
}

QStringList ConnectivityService::limitations() const
{
    return d->m_limitations;
}

QString ConnectivityService::status() const
{
    return d->m_status;
}

bool ConnectivityService::wifiEnabled() const
{
    return d->m_manager->wifiEnabled();
}

bool ConnectivityService::flightMode() const
{
    return d->m_manager->flightMode();
}

bool ConnectivityService::flightModeSwitchEnabled() const
{
    return !d->m_manager->unstoppableOperationHappening();
}

bool ConnectivityService::wifiSwitchEnabled() const
{
    return !d->m_manager->unstoppableOperationHappening();
}

bool ConnectivityService::hotspotSwitchEnabled() const
{
    return !d->m_manager->unstoppableOperationHappening()
            && !d->m_manager->flightMode();
}

bool ConnectivityService::modemAvailable() const
{
    return d->m_manager->modemAvailable();
}

bool ConnectivityService::hotspotEnabled() const
{
    return d->m_manager->hotspotEnabled();
}

QByteArray ConnectivityService::hotspotSsid() const
{
    return d->m_manager->hotspotSsid();
}

QString ConnectivityService::hotspotMode() const
{
    return d->m_manager->hotspotMode();
}

bool ConnectivityService::hotspotStored() const
{
    return d->m_manager->hotspotStored();
}

PrivateService::PrivateService(ConnectivityService& parent) :
        p(parent)
{
    // Memory is managed by Qt parent ownership
    new PrivateAdaptor(this);
}

void PrivateService::UnlockAllModems()
{
    p.d->m_manager->unlockAllModems();
}

void PrivateService::UnlockModem(const QString &modem)
{
    p.d->m_manager->unlockModemByName(modem);
}

void PrivateService::SetFlightMode(bool enabled)
{
    p.d->m_manager->setFlightMode(enabled);
}

void PrivateService::SetWifiEnabled(bool enabled)
{
    p.d->m_manager->setWifiEnabled(enabled);
}

void PrivateService::SetHotspotEnabled(bool enabled)
{
    p.d->m_manager->setHotspotEnabled(enabled);
}

void PrivateService::SetHotspotSsid(const QByteArray &ssid)
{
    p.d->m_manager->setHotspotSsid(ssid);
}

void PrivateService::SetHotspotPassword(const QString &password)
{
    p.d->m_manager->setHotspotPassword(password);
}

void PrivateService::SetHotspotMode(const QString &mode)
{
    p.d->m_manager->setHotspotMode(mode);
}

void PrivateService::SetHotspotAuth(const QString &auth)
{
    p.d->m_manager->setHotspotAuth(auth);
}

QDBusObjectPath PrivateService::AddVpnConnection(int type)
{
    setDelayedReply(true);

    if (type < 0 || type > 1)
    {
        p.d->m_connection.send(
                message().createErrorReply(QDBusError::InvalidArgs,
                                           "Invalid VPN type"));
    }
    else
    {
        try
        {
            QString uuid = p.d->m_vpnManager->addConnection(
                    static_cast<VpnConnection::Type>(type));
            p.d->m_addQueue[uuid] = message();
        }
        catch (domain_error& e)
        {
            p.d->m_connection.send(
                    message().createErrorReply(QDBusError::InvalidArgs,
                                               e.what()));
        }
    }

    return QDBusObjectPath();
}

QDBusObjectPath PrivateService::ImportVpnConnection(int type, const QString &filepath)
{
    setDelayedReply(true);

    if (type < 0 || type > 1)
    {
        p.d->m_connection.send(
                message().createErrorReply(QDBusError::InvalidArgs,
                                           "Invalid VPN type"));
    }
    else
    {
        try
        {
            qDebug() << "Import VPN connection, type=" << type << " and filepath=" << filepath;
            QString uuid = p.d->m_vpnManager->importConnection(
                    static_cast<VpnConnection::Type>(type),
                    filepath
            );
            qDebug() << "Import VPN connection OK uuid=" << uuid;
            p.d->m_addQueue[uuid] = message();
        }
        catch (domain_error& e)
        {
            qDebug() << "Import VPN connection KO error=" << e.what();
            p.d->m_connection.send(
                    message().createErrorReply(QDBusError::InvalidArgs,
                                               e.what()));
        }
    }

    return QDBusObjectPath();
}

void PrivateService::RemoveVpnConnection(const QDBusObjectPath &path)
{
    DBusVpnConnection::SPtr vpnConnection;
    for (const auto& tmp: p.d->m_vpnConnections)
    {
        if (tmp->path() == path)
        {
            vpnConnection = tmp;
            break;
        }
    }

    if (vpnConnection)
    {
        vpnConnection->remove();
    }
    else
    {
        setDelayedReply(true);
                p.d->m_connection.send(
                        message().createErrorReply(QDBusError::InvalidArgs, "Unknown VPN connection: " + path.path()));
    }
}

QString PrivateService::hotspotPassword() const
{
    return p.d->m_manager->hotspotPassword();
}

QString PrivateService::hotspotAuth() const
{
    return p.d->m_manager->hotspotAuth();
}

QList<QDBusObjectPath> PrivateService::vpnConnections() const
{
    QList<QDBusObjectPath> paths;
    for (const auto& vpnConnection: p.d->m_vpnConnections)
    {
        paths << vpnConnection->path();
    }
    return paths;
}

bool PrivateService::mobileDataEnabled() const
{
    return p.d->m_manager->mobileDataEnabled();
}

void PrivateService::setMobileDataEnabled(bool enabled)
{
    p.d->m_manager->setMobileDataEnabled(enabled);
}

QDBusObjectPath PrivateService::simForMobileData() const
{
    wwan::Sim::Ptr sim = p.d->m_manager->simForMobileData();
    if (!sim)
    {
        return QDBusObjectPath("/");
    }

    if (p.d->m_sims.contains(sim->iccid()))
    {
        return p.d->m_sims[sim->iccid()]->path();
    }
    else
    {
        qWarning() << "Could not find SIM for ICCID:" << sim->iccid();
        return QDBusObjectPath("/");
    }
}

void PrivateService::setSimForMobileData(const QDBusObjectPath &path)
{
    if (path.path() == "/")
    {
        p.d->m_manager->setSimForMobileData(wwan::Sim::Ptr());
        return;
    }

    bool found = false;
    for (DBusSim::SPtr dbussim: p.d->m_sims)
    {
        if (dbussim->path() == path)
        {
            found = true;
            p.d->m_manager->setSimForMobileData(dbussim->sim());
        }
    }

    if (!found)
    {
        sendErrorReply(QDBusError::UnknownObject);
    }
}

QList<QDBusObjectPath> PrivateService::modems() const
{
    auto list = QList<QDBusObjectPath>();
    for (DBusModem::SPtr modem : p.d->m_modems)
    {
        list << modem->path();
    }
    return list;
}

QList<QDBusObjectPath> PrivateService::sims() const
{
    QList<QDBusObjectPath> paths;

    for (auto dbussim : p.d->m_sims)
    {
        paths.append(dbussim->path());
    }
    return paths;
}

}

#include "connectivity-service.moc"
