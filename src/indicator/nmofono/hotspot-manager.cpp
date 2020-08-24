/*
 * Copyright (C) 2014, 2015 Canonical, Ltd.
 *
 * Authors:
 *    Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *    Jonas G. Drange <jonas.drange@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <nmofono/hotspot-manager.h>
#include <qpowerd/qpowerd.h>
#include <NetworkManagerActiveConnectionInterface.h>
#include <NetworkManagerDeviceInterface.h>
#include <NetworkManagerInterface.h>
#include <NetworkManagerSettingsInterface.h>
#include <NetworkManagerSettingsConnectionInterface.h>
#include <URfkillInterface.h>

#include <QStringList>
#include <QDBusReply>
#include <QtDebug>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QRegularExpression>
#include <NetworkManager.h>

using namespace std;

namespace nmofono
{

struct ApDevice
{
    ApDevice (const QDBusObjectPath& path, const QString& interface) :
            m_path (path), m_interface (interface)
    {
    }

    QDBusObjectPath m_path;
    QString m_interface;
};

class HotspotManager::Priv: public QObject
{

public:
    Priv(HotspotManager& parent) :
        p(parent)
    {
    }

    void addConnection()
    {
        qDebug() << "Adding new hotspot connection";
        QVariantDictMap connection = createConnectionSettings(m_ssid, m_password,
                                                              m_mode, m_auth);

        auto add_connection_reply = m_settings->AddConnection(connection);
        add_connection_reply.waitForFinished();

        if (add_connection_reply.isError())
        {
            qCritical() << "Failed to add connection: "
                    << add_connection_reply.error().message();
            Q_EMIT p.reportError(0);
            m_hotspot.reset();

            setStored(false);
            return;
        }

        QDBusObjectPath connectionPath(add_connection_reply);

        m_hotspot = make_shared<
                OrgFreedesktopNetworkManagerSettingsConnectionInterface>(
                NM_DBUS_SERVICE, connectionPath.path(), m_manager->connection());

        setStored(true);
    }

    void updateConnection()
    {
        qDebug() << "Updating hotspot connection";
        // Get new settings
        QVariantDictMap new_settings = createConnectionSettings(m_ssid,
                                                                m_password,
                                                                m_mode, m_auth);
        auto updating = m_hotspot->Update(new_settings);
        updating.waitForFinished();
        if (!updating.isValid())
        {
            qCritical()
                    << "Could not update connection:"
                    << updating.error().message();
        }
    }

    bool activateConnection(const QDBusObjectPath& device)
    {
        auto reply = m_manager->ActivateConnection(
                        QDBusObjectPath(m_hotspot->path()), device,
                        QDBusObjectPath("/"));
        reply.waitForFinished();
        if (reply.isError())
        {
            qCritical() << "Could not activate hotspot connection"
                    << reply.error().message();
            return false;
        }

        QDBusObjectPath activeConnectionPath(reply);

        OrgFreedesktopNetworkManagerConnectionActiveInterface activeConnection (
                    NM_DBUS_SERVICE, activeConnectionPath.path (),
                    m_manager->connection());

        int count = 0;
        // Wait for connection to activate
        while (count < 20 && activeConnection.state() != NM_ACTIVE_CONNECTION_STATE_ACTIVATED)
        {
            qDebug() << "Waiting for hotspot to connect";
            QThread::msleep(100);
            ++count;
        }

        return (activeConnection.state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED);
    }

    /**
     * Enables a hotspot.
     */
    void enable(const QDBusObjectPath& device)
    {
        if (!m_hotspot)
        {
            qWarning() << "Could not find a hotspot setup to enable";
            return;
        }

        qDebug() << "Activating hotspot on device" << device.path();
        bool success = activateConnection(device);
        setEnable(success);
        if (success)
        {
            // If our connection gets booted, reconnect
            connect(m_activeConnectionManager.get(),
                    &connection::ActiveConnectionManager::connectionsUpdated, this,
                    &Priv::reactivateConnection,
                    Qt::QueuedConnection);
        }
    }

    /**
     * Disables a hotspot.
     */
    void disable()
    {
        disconnect(m_activeConnectionManager.get(),
                   &connection::ActiveConnectionManager::connectionsUpdated,
                   this, &Priv::reactivateConnection);

        auto activeConnection = getActiveConnection();
        if (activeConnection)
        {
            m_activeConnectionManager->deactivate(activeConnection);
        }

        setInterfaceFirmware("/", "sta");

        setEnable(false);
    }

    void setStored(bool value)
    {
        if (m_stored != value)
        {
            m_stored = value;
            Q_EMIT p.storedChanged(value);
        }
    }

    void setEnable(bool value)
    {
        if (m_enabled != value)
        {
            m_enabled = value;
            // Request or clear the wakelock, depending on the hotspot state
            if (value)
            {
                m_wakelock = m_powerd->requestSysState(
                        "connectivity-service", QPowerd::SysPowerState::active);
            }
            else
            {
                m_wakelock.reset();
            }
            Q_EMIT p.enabledChanged(value);
        }
    }

    void updateSettingsFromDbus()
    {
        setEnable(isHotspotActive());
        setDisconnectWifi(m_enabled);

        QVariantDictMap settings = getConnectionSettings(*m_hotspot);
        const char wifi_key[] = "802-11-wireless";
        const char security_key[] = "802-11-wireless-security";

        if (settings.find(wifi_key) != settings.end())
        {
            QByteArray ssid = settings[wifi_key]["ssid"].toByteArray();
            if (!ssid.isEmpty())
            {
                p.setSsid(ssid);
            }

            QString mode = settings[wifi_key]["mode"].toString();
            if (!mode.isEmpty())
            {
                p.setMode(mode);
            }
        }

        QVariantDictMap secrets = getConnectionSecrets(*m_hotspot,
                                                       security_key);

        if (secrets.find(security_key) != secrets.end())
        {
            QString pwd = secrets[security_key]["psk"].toString();
            if (!pwd.isEmpty())
            {
                p.setPassword(pwd);
            }
        } else {
            p.setAuth("none");
        }
    }

    // wpa_supplicant interaction

    QString getTetheringInterface()
    {
        QString program("getprop");
        QStringList arguments;
        arguments << "wifi.tethering.interface";

        QProcess getprop;
        getprop.start(program, arguments);

        if (!getprop.waitForFinished())
        {
            qCritical() << "getprop process failed:" << getprop.errorString();
            return QString();
        }

        QString output = getprop.readAllStandardOutput();
        // Take just the first line
        return output.split("\n").first();
    }

    /**
     * True if changed successfully, or there was no need. Otherwise false.
     * Supported modes are 'p2p', 'sta' and 'ap'.
     */
    bool setInterfaceFirmware(const QString& interface, const QString& mode)
    {
        // Not supported.
        if (mode == "adhoc")
        {
            return true;
        }

        QDBusInterface wpasIface(DBusTypes::WPASUPPLICANT_DBUS_NAME,
                                 DBusTypes::WPASUPPLICANT_DBUS_PATH,
                                 DBusTypes::WPASUPPLICANT_DBUS_INTERFACE,
                                 m_manager->connection());

        auto set_interface = wpasIface.call(
                "SetInterfaceFirmware", QVariant::fromValue(QDBusObjectPath(interface)),
                QVariant(mode));

        if (set_interface.type() == QDBusMessage::ErrorMessage)
        {
            qCritical() << "Failed to change interface firmware:"
                    << set_interface.errorMessage();
            return false;
        }

        return true;
    }

    void findApDevice()
    {
        m_device.release();
        QString tetherIface = getTetheringInterface();

        auto devices = QList<QDBusObjectPath>(m_manager->GetDevices()).toStdList();
        // Iterate in reverse to attempt to minimise dbus calls (new device is likely at the end)
        for (auto path = devices.rbegin(); path != devices.rend(); ++path)
        {
            OrgFreedesktopNetworkManagerDeviceInterface device(NM_DBUS_SERVICE, path->path(), m_manager->connection());

            QString interface = device.interface();

            if (!tetherIface.isEmpty())
            {
                if (tetherIface.compare(interface) != 0)
                {
                    continue;
                }
            }

            if (device.deviceType() != NM_DEVICE_TYPE_WIFI)
            {
                continue;
            }

            if (device.state() <= NM_DEVICE_STATE_UNAVAILABLE)
            {
                continue;
            }

            qDebug() << "Using AP interface " << interface;
            m_device = make_unique<ApDevice>(*path, interface);
            break;
        }
    }

    void createApDevice()
    {
        setInterfaceFirmware("/", m_mode);

        m_device.release();

        int count = 0;
        // Wait for AP device to appear
        while (count < 20 && !m_device)
        {
            QThread::msleep(100);
            findApDevice();
            qDebug() << "Searching for AP device";
            ++count;
        }
    }

    // wpa_supplicant interaction

    /**
     * Helper that maps QStrings to other QVariantMaps, i.e.
     * QMap<QString, QVariantMap>. QVariantMap is an alias for
     * QMap<QString, QVariant>.
     * See http://doc.qt.io/qt-5/qvariant.html#QVariantMap-typedef and
     * https://developer.gnome.org/NetworkoManager/0.9/spec.html
     *     #type-String_String_Variant_Map_Map
     */
    QVariantDictMap createConnectionSettings(
        const QByteArray &ssid, const QString &password,
        QString mode, QString auth)
    {
        bool autoConnect = false;

        QVariantDictMap connection;

        QString s_ssid = QString::fromLatin1(ssid);

        if (m_uuid.isEmpty()) {
            m_uuid = QUuid().createUuid().toString();
            // Remove {} from the generated uuid.
            m_uuid.remove(0, 1);
            m_uuid.remove(m_uuid.size() - 1, 1);
        }

        QVariantMap wireless;

        if (auth != "none")
        {
            wireless[QStringLiteral("security")] = QVariant(QStringLiteral("802-11-wireless-security"));
        }
        wireless[QStringLiteral("ssid")] = QVariant(ssid);
        wireless[QStringLiteral("mode")] = QVariant(mode);

        connection["802-11-wireless"] = wireless;

        QVariantMap connsettings;
        connsettings[QStringLiteral("autoconnect")] = QVariant(autoConnect);
        connsettings[QStringLiteral("id")] = QVariant(s_ssid);
        connsettings[QStringLiteral("uuid")] = QVariant(m_uuid);
        connsettings[QStringLiteral("type")] = QVariant(QStringLiteral("802-11-wireless"));
        connection["connection"] = connsettings;

        QVariantMap ipv4;
        ipv4[QStringLiteral("method")] = QVariant(QStringLiteral("shared"));
        connection["ipv4"] = ipv4;

        QVariantMap ipv6;
        ipv6[QStringLiteral("method")] = QVariant(QStringLiteral("ignore"));
        connection["ipv6"] = ipv6;

        if (auth != "none")
        {
            QVariantMap security;
            security[QStringLiteral("proto")] = QVariant(QStringList{ "rsn" });
            security[QStringLiteral("pairwise")] = QVariant(QStringList{ "ccmp" });
            security[QStringLiteral("group")] = QVariant(QStringList{ "ccmp" });
            security[QStringLiteral("key-mgmt")] = QVariant(auth);
            security[QStringLiteral("psk")] = QVariant(password);
            connection["802-11-wireless-security"] = security;
        }

        return connection;
    }

    /**
     * Helper that returns a QMap<QString, QVariantMap> given a QDBusObjectPath.
     * See https://developer.gnome.org/NetworkManager/0.9/spec.html
     *     #org.freedesktop.NetworkManager.Settings.Connection.GetSettings
     */
    QVariantDictMap getConnectionSettings (OrgFreedesktopNetworkManagerSettingsConnectionInterface& conn) {
        auto connection_settings = conn.GetSettings();
        connection_settings.waitForFinished();
        return connection_settings.value();
    }


    /**
     * Helper that returns a QMap<QString, QVariantMap> given a QDBusObjectPath.
     * See https://developer.gnome.org/NetworkManager/0.9/spec.html
     *     #org.freedesktop.NetworkManager.Settings.Connection.GetSettings
     */
    QVariantDictMap getConnectionSecrets (OrgFreedesktopNetworkManagerSettingsConnectionInterface& conn,
        const QString key)
    {
        auto connection_secrets = conn.GetSecrets(key);
        connection_secrets.waitForFinished();
        return connection_secrets.value();
    }

    /**
     * Returns a QDBusObjectPath of a hotspot given a mode.
     * Valid modes are 'p2p', 'ap' and 'adhoc'.
     */
    void getHotspot()
    {
        const char wifi_key[] = "802-11-wireless";

        auto listed_connections = m_settings->ListConnections();
        listed_connections.waitForFinished();

        for (const auto &connection : listed_connections.value())
        {
            auto conn = make_shared<OrgFreedesktopNetworkManagerSettingsConnectionInterface>(
                    NM_DBUS_SERVICE, connection.path(),
                    m_manager->connection());

            auto connection_settings = getConnectionSettings(*conn);

            if (connection_settings.find(wifi_key) != connection_settings.end())
            {
                auto wifi_setup = connection_settings[wifi_key];
                QString wifi_mode = wifi_setup["mode"].toString();

                if (wifi_mode == m_mode)
                {
                    m_hotspot = conn;
                    m_uuid = connection_settings["connection"]["uuid"].toString();
                    return;
                }
            }
        }
        m_hotspot.reset();
        m_uuid = QString();
    }

    connection::ActiveConnection::SPtr getActiveConnection()
    {
        connection::ActiveConnection::SPtr activeConnection;

        if (m_hotspot)
        {
            for (const auto &active_connection : m_activeConnectionManager->connections())
            {
                if (active_connection->connectionPath().path() == m_hotspot->path())
                {
                    activeConnection = active_connection;
                    break;
                }
            }
        }

        return activeConnection;
    }

    /**
     * Helper to check if the hotspot on a given QDBusObjectPath is active
     * or not. It checks if the Connection.Active [1] for the given
     * path is in NetworkManager's ActiveConnections property [2].
     * [1] https://developer.gnome.org/NetworkManager/0.9/spec.html
     *     #org.freedesktop.NetworkManager.Connection.Active
     * [2] https://developer.gnome.org/NetworkManager/0.9/spec.html
     *     #org.freedesktop.NetworkManager
     */
    bool isHotspotActive ()
    {
        return bool(getActiveConnection());
    }

    void generatePassword()
    {
        static const std::string items("abcdefghijklmnopqrstuvwxyz01234567890");
        const int password_length = 8;
        std::string result;

        for (int i = 0; i < password_length; i++)
        {
            result.push_back(items[std::rand() % items.length()]);
        }

        m_password = QString::fromStdString(result);
    }

    void setDisconnectWifi(bool disconnect)
    {
        if (m_disconnectWifi == disconnect)
        {
            return;
        }

        m_disconnectWifi = disconnect;
        Q_EMIT p.disconnectWifiChanged(m_disconnectWifi);
    }

public Q_SLOTS:
    void reactivateConnection()
    {
        if (!m_hotspot)
        {
            qWarning() << "Could not find a hotspot setup to enable";
            return;
        }

        auto activeConnection = getActiveConnection();
        if (activeConnection)
        {
            return;
        }


        findApDevice();

        if (m_device)
        {
            qDebug() << "Reactivating hotspot connection on device" << m_device->m_path.path();
            activateConnection(m_device->m_path);
        }
        else
        {
            qWarning() << "Could not get device when reactivating hotspot connection";
        }
    }

public:
    HotspotManager& p;

    QString m_mode = "ap";
    QString m_auth = "wpa-psk";
    bool m_enabled = false;
    bool m_stored = false;
    QString m_password;
    QByteArray m_ssid = "Ubuntu";

    unique_ptr<ApDevice> m_device;

    QPowerd::UPtr m_powerd;
    QPowerd::RequestSPtr m_wakelock;

    bool m_disconnectWifi = false;

    /**
     * NetworkManager dbus interface proxy we will use to query
     * against NetworkManager. See
     * https://developer.gnome.org/NetworkManager/0.9/spec.html
     *     #org.freedesktop.NetworkManager
     */
    unique_ptr<OrgFreedesktopNetworkManagerInterface> m_manager;

    /**
     * NetworkManager Settings interface proxy we use to get
     * the list of connections, as well as adding connections.
     * See https://developer.gnome.org/NetworkManager/0.9/spec.html
     *     #org.freedesktop.NetworkManager.Settings
     */
    unique_ptr<OrgFreedesktopNetworkManagerSettingsInterface> m_settings;

    shared_ptr<OrgFreedesktopNetworkManagerSettingsConnectionInterface> m_hotspot;

    QString m_uuid;

    connection::ActiveConnectionManager::SPtr m_activeConnectionManager;
};

HotspotManager::HotspotManager(connection::ActiveConnectionManager::SPtr activeConnectionManager,
                               const QDBusConnection& connection,
                               QObject *parent) :
        QObject(parent), d(new Priv(*this))
{
    d->m_activeConnectionManager = activeConnectionManager;

    d->m_manager = make_unique<OrgFreedesktopNetworkManagerInterface>(
            NM_DBUS_SERVICE, NM_DBUS_PATH, connection);
    d->m_settings = make_unique<OrgFreedesktopNetworkManagerSettingsInterface>(
            NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS, connection);

    d->m_powerd = make_unique<QPowerd>(connection);

    d->generatePassword();

    // Stored is false if hotspot path is empty.
    d->getHotspot();
    d->setStored(bool(d->m_hotspot));

    if (d->m_stored)
    {
        d->updateSettingsFromDbus();
    }
}

void HotspotManager::setEnabled(bool value)
{
    if (enabled() == value)
    {
        return;
    }

    // We are enabling a hotspot
    if (value)
    {
        // If the SSID is empty, we report an error.
        if (d->m_ssid.isEmpty())
        {
            qWarning() << "  SSID was empty";
            Q_EMIT reportError(1);
            d->setEnable(false);
            return;
        }

        d->setDisconnectWifi(true);

        setMtkTetheringEnabled(true);

        // We use Hybris to load the new device firmware
        d->createApDevice();

        if (!d->m_device)
        {
            qWarning() << "Failed to create AP device";
            Q_EMIT reportError(1);
            d->setDisconnectWifi(false);
            return;
        }

        if (d->m_stored)
        {
            d->updateConnection();
        }
        else
        {
            d->addConnection();
        }
        d->enable(d->m_device->m_path);
    }
    else
    {
        // Disabling the hotspot.
        d->disable();

        setMtkTetheringEnabled(false);

        d->setDisconnectWifi(false);
    }

}

bool HotspotManager::enabled() const {
    return d->m_enabled;
}

bool HotspotManager::stored() const {
    return d->m_stored;
}

QByteArray HotspotManager::ssid() const {
    return d->m_ssid;
}

void HotspotManager::setSsid(const QByteArray& value) {
    if (d->m_ssid != value)
    {
        d->m_ssid = value;
        Q_EMIT ssidChanged(value);
    }
}

QString HotspotManager::password() const {
    return d->m_password;
}

void HotspotManager::setPassword(const QString& value) {
    if (d->m_password != value)
    {
        d->m_password = value;
        Q_EMIT passwordChanged(value);
    }
}

QString HotspotManager::mode() const {
    return d->m_mode;
}

QString HotspotManager::auth() const {
    return d->m_auth;
}

QString HotspotManager::interface() const
{
    if (enabled() && d->m_device)
        return d->m_device->m_interface;
    else
        return QString();
}

void HotspotManager::setMode(const QString& value) {
    if (d->m_mode != value)
    {
        d->m_mode = value;
        Q_EMIT modeChanged(value);
    }
}

void HotspotManager::setAuth(const QString& value) {
    if (d->m_auth != value)
    {
        d->m_auth = value;
        Q_EMIT authChanged(value);
    }
}

bool HotspotManager::disconnectWifi() const
{
    return d->m_disconnectWifi;
}

void HotspotManager::setMtkTetheringEnabled(bool value)
{
    // Switch MediaTek /dev/wmtWifi adapter mode between AP/STA when hotspot
    // enabled state is changed.
    QFile wmtWifi_file("/dev/wmtWifi");
    if (wmtWifi_file.exists())
    {
        if (wmtWifi_file.open(QIODevice::WriteOnly)) {
            // Configure new adapter mode
            const char newAdapterMode = (value ? 'A' : 'S');
            qDebug() << "setMtkTetheringEnabled() -> Opened the wmtWifi device for writing, configuring new mode =" << newAdapterMode;
            if (!wmtWifi_file.putChar(newAdapterMode))
                qWarning() << "setMtkTetheringEnabled() -> An error occured while changing the wmtWifi adapter operating mode!";
            wmtWifi_file.close();
            QThread::msleep(1000); // wait 1s for the interface(s) to become available again
        } else
            qWarning() << "setMtkTetheringEnabled() -> Couldn't open /dev/wmtWifi for writing! (insufficient permissions?)";
    }
}

}
