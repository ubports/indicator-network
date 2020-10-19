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

#include <nmofono/wifi/wifi-link-impl.h>
#include <nmofono/wifi/access-point-impl.h>
#include <nmofono/wifi/grouped-access-point.h>
#include <url-dispatcher-cpp/url-dispatcher.h>
#include <cassert>

#include <NetworkManagerActiveConnectionInterface.h>
#include <NetworkManagerDeviceWirelessInterface.h>
#include <NetworkManagerSettingsConnectionInterface.h>

#include <NetworkManager.h>
#include <iostream>
#include <QUrlQuery>

using namespace std;

namespace nmofono
{
namespace wifi
{

struct WifiLinkImpl::Private: public QObject
{
    Q_OBJECT

public:
    Private(WifiLinkImpl& parent,
            shared_ptr<OrgFreedesktopNetworkManagerDeviceInterface> dev,
            shared_ptr<OrgFreedesktopNetworkManagerInterface> nm,
            KillSwitch::Ptr killSwitch)
       : p(parent),
         m_dev(dev),
         m_wireless(NM_DBUS_SERVICE, dev->path(), dev->connection()),
         m_nm(nm),
         m_killSwitch(killSwitch),
         m_lastState(NM_STATE_UNKNOWN),
         m_connecting(false)
    {
    }

    WifiLinkImpl& p;

    uint32_t m_characteristics = Link::Characteristics::empty;
    Link::Status m_status = Status::disabled;
    bool m_isManaged = true;
    QSet<AccessPointImpl::Ptr> m_rawAccessPoints;
    QSet<AccessPoint::Ptr> m_groupedAccessPoints;
    AccessPoint::Ptr m_activeAccessPoint;
    Signal m_signal = Signal::disconnected;

    shared_ptr<OrgFreedesktopNetworkManagerDeviceInterface> m_dev;
    OrgFreedesktopNetworkManagerDeviceWirelessInterface m_wireless;
    shared_ptr<OrgFreedesktopNetworkManagerInterface> m_nm;

    KillSwitch::Ptr m_killSwitch;

    map<AccessPointImpl::Key, shared_ptr<GroupedAccessPoint>> m_grouper;
    uint32_t m_lastState = 0;
    QString m_name;
    shared_ptr<OrgFreedesktopNetworkManagerConnectionActiveInterface> m_activeConnection;
    unique_ptr<QMetaObject::Connection> m_signalStrengthConnection;
    bool m_connecting = false;
    bool m_disconnectWifi = false;

    void setStatus(Status status)
    {
        if (m_status == status)
        {
            return;
        }

        m_status = status;
        Q_EMIT p.statusUpdated(m_status);
    }

    void setIsManaged(bool value)
    {
        if (m_isManaged == value)
        {
            return;
        }

        m_isManaged = value;
        Q_EMIT p.isManagedChanged(m_isManaged);
    }

    void updateDeviceState(uint new_state)
    {
        m_lastState = new_state;

        if (new_state == NM_DEVICE_STATE_UNMANAGED) {
            // Mark unmanaged adapters as such.
            setIsManaged(false);
        }

        switch (new_state){
        case NM_DEVICE_STATE_DISCONNECTED:
        case NM_DEVICE_STATE_DEACTIVATING:
            if (m_connecting) {
                // ignore these while doing connect_to()
                break;
            }
            /* fallthrough */
        case NM_DEVICE_STATE_UNKNOWN:
        case NM_DEVICE_STATE_UNMANAGED:
        case NM_DEVICE_STATE_UNAVAILABLE:
        case NM_DEVICE_STATE_FAILED:
        {
            // make sure to set activeConnection before changing the status
            updateActiveConnection(QDBusObjectPath("/"));

            switch(m_killSwitch->state()) {
            case KillSwitch::State::hard_blocked:
            case KillSwitch::State::soft_blocked:
                setStatus(Status::disabled);
                break;
            case KillSwitch::State::not_available:
            case KillSwitch::State::unblocked:
                setStatus(Status::offline);
            }
            break;
        }
        case NM_DEVICE_STATE_PREPARE:
        case NM_DEVICE_STATE_CONFIG:
        case NM_DEVICE_STATE_NEED_AUTH:
        case NM_DEVICE_STATE_IP_CONFIG:
        case NM_DEVICE_STATE_IP_CHECK:
        {
            // make sure to set activeConnection before changing the status
            QDBusObjectPath path = m_dev->activeConnection();
            // for some reason the path is not always set on these
            // states. Let's not clear the active connection as obviously
            // we have one.
            if (path != QDBusObjectPath("/"))
            {
                updateActiveConnection(path);
            }
            setStatus(Status::connecting);
            break;
        }
        case NM_DEVICE_STATE_SECONDARIES:
        {
            // make sure to set activeConnection before changing the status
            updateActiveConnection(m_dev->activeConnection());
            setStatus(Status::connected);
            break;
        }
        case NM_DEVICE_STATE_ACTIVATED:
        {
            // make sure to set activeConnection before changing the status
            updateActiveConnection(m_dev->activeConnection());
            setStatus(Status::online);
            break;
        }}

    }


    void disconnectSignalStengthConnection()
    {
        if (m_signalStrengthConnection)
        {
            disconnect(*m_signalStrengthConnection);
            m_signalStrengthConnection.reset();
        }
    }

    /// '/' path means invalid.
    void updateActiveConnection(const QDBusObjectPath &path)
    {
        // clear the one we have.
        if (path == QDBusObjectPath("/")) {
            m_activeAccessPoint.reset();
            Q_EMIT p.activeAccessPointUpdated(m_activeAccessPoint);
            m_activeConnection.reset();
            disconnectSignalStengthConnection();
            strengthUpdated();
            return;
        }

        // already up-to-date
        if (m_activeConnection && m_activeConnection->path() == path.path())
        {
            return;
        }

        try {
            m_activeConnection = make_shared<
                    OrgFreedesktopNetworkManagerConnectionActiveInterface>(
                    NM_DBUS_SERVICE, path.path(), m_dev->connection());
            uint state = m_activeConnection->state();
            switch (state) {
            case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
            case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
            case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
            case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:
            case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
                ;

                // for Wi-Fi devices specific_object is the AccessPoint object.
                QDBusObjectPath ap_path = m_activeConnection->specificObject();
                for (auto &ap : m_groupedAccessPoints) {
                    auto shap =  dynamic_pointer_cast<GroupedAccessPoint>(ap);
                    if (shap->has_object(ap_path)) {
                        m_activeAccessPoint = ap;
                        disconnectSignalStengthConnection();
                        m_signalStrengthConnection = make_unique<
                                QMetaObject::Connection>(
                                connect(m_activeAccessPoint.get(),
                                        &AccessPoint::strengthUpdated, this,
                                        &Private::strengthUpdated));
                        Q_EMIT p.activeAccessPointUpdated(m_activeAccessPoint);
                        strengthUpdated();
                        break;
                    }
                }
            }
        } catch (exception &e) {
            qWarning() << "failed to get active connection:";
            qWarning() << "\tpath: " << path.path();
            qWarning() << "\t" << QString::fromStdString(e.what());
        }
    }


public Q_SLOTS:
    void ap_added(const QDBusObjectPath &path)
    {
        try {
            for (auto ap : m_rawAccessPoints) {
                if (dynamic_pointer_cast<AccessPoint>(ap)->object_path() == path) {
                    // already in the list
                    return;
                }
            }

            AccessPointImpl::Ptr shap;
            try {
                auto ap = make_shared<
                        OrgFreedesktopNetworkManagerAccessPointInterface>(
                        NM_DBUS_SERVICE, path.path(), m_dev->connection());
                shap = make_shared<AccessPointImpl>(ap);
            } catch(const exception &e) {
                qWarning() << "Failed to create AccessPoint proxy for "<< path.path() << ": ";
                qWarning() << "\t" << QString::fromStdString(e.what());
                qWarning() << "\tIgnoring.";
                return;
            }

            m_rawAccessPoints.insert(shap);

            auto k = AccessPointImpl::Key(shap);
            if(m_grouper.find(k) != m_grouper.end()) {
                m_grouper[k]->add_ap(shap);
            } else {
                m_grouper[k] = make_shared<GroupedAccessPoint>(shap);
            }
            update_grouped_access_points();
        } catch(const exception &e) {
            /// @bug dbus-cpp internal logic exploded
            // If this happens, indicator-network is in an unknown state with no clear way of
            // recovering. The only reasonable way out is a graceful exit.
            cerr << " Failed to run dbus service: " << e.what() << endl;
        }
    }

    void ap_removed(const QDBusObjectPath &path)
    {
        AccessPointImpl::Ptr shap;

        auto list = m_rawAccessPoints;
        for (const auto &ap : list) {
            if (ap->object_path() == path) {
                shap = ap;
                list.remove(ap);
                break;
            }
        }
        if (!shap) {
            qWarning() << "Tried to remove access point " << path.path() << " that has not been added.";
            return;
        }
        m_rawAccessPoints = list;

        AccessPointImpl::Key k(shap);
        auto it = m_grouper.find(k);
        if (it != m_grouper.end())
        {
            it->second->remove_ap(shap);
            if (it->second->num_aps() == 0)
            {
                m_grouper.erase(it);
            }
        }
        update_grouped_access_points();
    }

    void update_grouped_access_points()
    {
        m_groupedAccessPoints.clear();
        for (auto &i : m_grouper)
        {
            m_groupedAccessPoints.insert(i.second);
        }

        if (m_disconnectWifi)
        {
            Q_EMIT p.accessPointsUpdated(QSet<AccessPoint::Ptr>());
        }
        else
        {
            Q_EMIT p.accessPointsUpdated(m_groupedAccessPoints);
        }
    }

    void state_changed(uint new_state, uint, uint)
    {
        updateDeviceState(new_state);
    }

    void kill_switch_updated(KillSwitch::State)
    {
        updateDeviceState(m_lastState);
    }

    void strengthUpdated()
    {
        Signal signal = Signal::disconnected;

        if (m_activeAccessPoint && !m_disconnectWifi)
        {
            double strength = m_activeAccessPoint->strength();
            bool secured  = m_activeAccessPoint->secured();

            if (strength >= 80.0)
            {
                signal = secured ? Signal::signal_100_secure : Signal::signal_100;
            }
            else if (strength >= 60.0)
            {
                signal = secured ? Signal::signal_75_secure : Signal::signal_75;
            }
            else if (strength >= 40.0)
            {
                signal = secured ? Signal::signal_50_secure : Signal::signal_50;
            }
            else if (strength >= 20.0)
            {
                signal = secured ? Signal::signal_25_secure : Signal::signal_25;
            }
            else
            {
                signal = secured ? Signal::signal_0_secure : Signal::signal_0;
            }
        }

        if (m_signal == signal)
        {
            return;
        }

        m_signal = signal;
        Q_EMIT p.signalUpdated(m_signal);
    }
};

WifiLinkImpl::WifiLinkImpl(shared_ptr<OrgFreedesktopNetworkManagerDeviceInterface> dev,
           shared_ptr<OrgFreedesktopNetworkManagerInterface> nm,
           KillSwitch::Ptr killSwitch)
    : d(new Private(*this, dev, nm, killSwitch)) {
    d->m_name = d->m_dev->interface();

    connect(&d->m_wireless, &OrgFreedesktopNetworkManagerDeviceWirelessInterface::AccessPointAdded, d.get(), &Private::ap_added);
    connect(&d->m_wireless, &OrgFreedesktopNetworkManagerDeviceWirelessInterface::AccessPointRemoved, d.get(), &Private::ap_removed);
    QList<QDBusObjectPath> aps = d->m_wireless.GetAccessPoints();
    for (const auto& path : aps) {
        d->ap_added(path);
    }

    connect(d->m_dev.get(), &OrgFreedesktopNetworkManagerDeviceInterface::StateChanged, d.get(), &Private::state_changed);
    d->updateDeviceState(d->m_dev->state());

    connect(d->m_killSwitch.get(), &KillSwitch::stateChanged, d.get(), &Private::kill_switch_updated);

    d->strengthUpdated();
}

WifiLinkImpl::~WifiLinkImpl()
{}

Link::Type
WifiLinkImpl::type() const
{
    return Type::wifi;
}

uint32_t
WifiLinkImpl::characteristics() const
{
    return d->m_characteristics;
}

Link::Status
WifiLinkImpl::status() const
{
    return d->m_status;
}

bool
WifiLinkImpl::isManaged() const
{
    return d->m_isManaged;
}

Link::Id
WifiLinkImpl::id() const
{
    return 0;
}

QString
WifiLinkImpl::name() const
{
    return d->m_name;
}

WifiLink::Mode WifiLinkImpl::mode() const
{
    static const map<uint, WifiLink::Mode> modeMap
    {
        {NM_802_11_MODE_UNKNOWN, Mode::unknown},
        {NM_802_11_MODE_ADHOC, Mode::adhoc},
        {NM_802_11_MODE_INFRA, Mode::infra},
        {NM_802_11_MODE_AP, Mode::ap}
    };
    return modeMap.find(d->m_wireless.mode())->second;
}

WifiLink::Signal WifiLinkImpl::signal() const
{
    return d->m_signal;
}

QSet<AccessPoint::Ptr>
WifiLinkImpl::accessPoints() const {
    if (d->m_disconnectWifi)
    {
        return QSet<AccessPoint::Ptr>();
    }

    return d->m_groupedAccessPoints;
}

void
WifiLinkImpl::connect_to(AccessPoint::Ptr accessPoint)
{
    qDebug() << "Connecting to:" << accessPoint->ssid();

    try {
        d->m_connecting = true;
        QByteArray ssid = accessPoint->raw_ssid();

        shared_ptr<OrgFreedesktopNetworkManagerSettingsConnectionInterface> found;
        QList<QDBusObjectPath> connections = d->m_dev->availableConnections();
        for (auto &path : connections) {
            auto con = make_shared<OrgFreedesktopNetworkManagerSettingsConnectionInterface>(
                    NM_DBUS_SERVICE, path.path(), d->m_dev->connection());
            QVariantDictMap settings = con->GetSettings();
            auto wirelessIt = settings.find("802-11-wireless");
            if (wirelessIt != settings.cend())
            {
                auto ssidIt = wirelessIt->find("ssid");
                if (ssidIt != wirelessIt->cend())
                {
                    if (ssidIt->toByteArray() == ssid)
                    {
                        found = con;
                        break;
                    }
                }
            }
        }

        /// @todo check the timestamps as there might be multiple ones that are suitable.
        /// @todo oh, and check more parameters than just the ssid

        QDBusObjectPath ac("/");
        if (found) {
            qDebug() << "Connecting to known access point";
            ac = d->m_nm->ActivateConnection(QDBusObjectPath(found->path()),
                                           QDBusObjectPath(d->m_dev->path()),
                                           accessPoint->object_path());
        } else {
            if (accessPoint->enterprise()) {
                qDebug() << "New connection to enterprise access point";
                // activate system settings URI
                QUrlQuery q;
                q.addQueryItem("ssid", accessPoint->raw_ssid());
                q.addQueryItem("bssid", accessPoint->bssid());
                QString url = "settings:///system/wifi?" + q.query(QUrl::FullyEncoded);

                UrlDispatcher::send(url.toStdString(), [](string url, bool success) {
                    if (!success) {
                        cerr << "URL Dispatcher failed on " << url << endl;
                    }
                });
            } else {
                qDebug() << "New connection to regular access point";
                QVariantDictMap conf;

                /// @todo getting the ssid multiple times over dbus is stupid.

                QVariantMap wireless_conf;
                wireless_conf["ssid"] = ssid;

                conf["802-11-wireless"] = wireless_conf;
                auto ret = d->m_nm->AddAndActivateConnection(
                        conf, QDBusObjectPath(d->m_dev->path()), accessPoint->object_path());
                ret.waitForFinished();
                ac = ret.argumentAt<1>();
            }
        }
        // For enterprise access points, the system settings app will perform the connection
        if (!accessPoint->enterprise()) {
            d->updateActiveConnection(ac);
        }
        d->m_connecting = false;
    } catch(const exception &e) {
        // @bug default timeout expired: LP(#1361642)
        // If this happens, indicator-network is in an unknown state with no clear way of
        // recovering. The only reasonable way out is a graceful exit.
        qWarning() << " Failed to activate connection: " << e.what();
    }
}

AccessPoint::Ptr
WifiLinkImpl::activeAccessPoint()
{
    return d->m_activeAccessPoint;
}

QDBusObjectPath WifiLinkImpl::device_path() const {
    return QDBusObjectPath(d->m_dev->path());
}

void
WifiLinkImpl::setDisconnectWifi(bool disconnect)
{
    if (disconnect == d->m_disconnectWifi)
    {
        return;
    }

    d->m_disconnectWifi = disconnect;

    d->m_dev->setAutoconnect(!disconnect);
    if (disconnect && d->m_activeConnection)
    {
        // Disconnect from the current network
        d->m_nm->DeactivateConnection(
                QDBusObjectPath(d->m_activeConnection->path()));
    }

    d->update_grouped_access_points();
    d->strengthUpdated();
}

}
}

#include "wifi-link-impl.moc"
