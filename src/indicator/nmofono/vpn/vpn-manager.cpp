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

#include <glib.h>
#include <nmofono/vpn/vpn-manager.h>
#include <stdexcept>
#include <util/localisation.h>
#include <NetworkManager.h>
#include <QMap>
#include <QString>

#include <NetworkManagerInterface.h>
#include <NetworkManagerSettingsInterface.h>

using namespace std;

namespace nmofono
{
namespace vpn
{

class VpnManager::Priv: public QObject
{
    Q_OBJECT

public:
    Priv(VpnManager& parent) :
        p(parent)
    {
    }

    void _newConnection(const QDBusObjectPath &path, bool shouldUpdateActiveAndBusy)
    {
        auto connection = make_shared<VpnConnection>(path, m_activeConnectionManager, m_settingsInterface->connection());
        if (connection->isValid())
        {
            m_connections[path] = connection;
            connection->setOtherConnectionIsBusy(m_busy);
            connection->setActiveConnectionPath(m_activeConnectionPath);
            connect(connection.get(), &VpnConnection::activateConnection, this, &Priv::activateConnection);
            connect(connection.get(), &VpnConnection::deactivateConnection, m_nmInterface.get(), &OrgFreedesktopNetworkManagerInterface::DeactivateConnection);
            connect(connection.get(), &VpnConnection::activeChanged, this, &Priv::updateActiveAndBusy);
            connect(connection.get(), &VpnConnection::busyChanged, this, &Priv::updateActiveAndBusy);
            connect(this, &Priv::busyChanged, connection.get(), &VpnConnection::setOtherConnectionIsBusy);
            connect(this, &Priv::activeConnectionPathChanged, connection.get(), &VpnConnection::setActiveConnectionPath);
            Q_EMIT p.connectionsChanged();
            if (shouldUpdateActiveAndBusy)
            {
                updateActiveAndBusy();
            }
        }
    }

    QSet<QString> connectionIds()
    {
        QSet<QString> ids;
        QMapIterator<QDBusObjectPath, VpnConnection::SPtr> it(m_connections);
        while (it.hasNext())
        {
            it.next();
            ids << it.value()->id();
        }
        return ids;
    }

    QString newConnectionName()
    {
        static const QString NAME_FORMAT(_("VPN connection %1"));
        auto ids = connectionIds();
        int i = 1;
        QString name = NAME_FORMAT.arg(i);
        while (ids.contains(name))
        {
            name = NAME_FORMAT.arg(++i);
        }
        return name;
    }

Q_SIGNALS:
    void busyChanged(bool busy);

    void activeConnectionPathChanged(const QDBusObjectPath& path);

public Q_SLOTS:
    void connectionRemoved(const QDBusObjectPath &path)
    {
        auto connection = m_connections.take(path);
        if (connection)
        {
            Q_EMIT p.connectionsChanged();
            updateActiveAndBusy();
        }
    }

    void newConnection(const QDBusObjectPath &path)
    {
        _newConnection(path, true);
    }

    void activateConnection(const QDBusObjectPath& connection)
    {
        auto reply = m_nmInterface->ActivateConnection(connection, QDBusObjectPath("/"), QDBusObjectPath("/"));
        auto watcher(new QDBusPendingCallWatcher(reply, this));
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &Priv::activateConnectionFinished);
    }

    void activateConnectionFinished(QDBusPendingCallWatcher *call)
    {
        QDBusPendingReply<QDBusObjectPath> reply = *call;
        if (reply.isError())
        {
            qWarning() << reply.error().message();
        }
        call->deleteLater();
    }

    void updateActiveAndBusy()
    {
        bool busy = false;
        for (const auto& connection: m_connections)
        {
            if (connection->isBusy())
            {
                busy = true;
                break;
            }
        }
        setBusy(busy);

        QDBusObjectPath activeConnectionPath;
        for (const auto& connection: m_connections)
        {
            if (connection->isActive())
            {
                activeConnectionPath = connection->path();
                break;
            }
        }
        setActiveConnectionPath(activeConnectionPath);
    }

    void setBusy(bool busy)
    {
        if (m_busy == busy)
        {
            return;
        }

        m_busy = busy;
        Q_EMIT busyChanged(m_busy);
    }

    void setActiveConnectionPath(const QDBusObjectPath& activeConnectionPath)
    {
        if (m_activeConnectionPath == activeConnectionPath)
        {
            return;
        }

        m_activeConnectionPath = activeConnectionPath;
        Q_EMIT activeConnectionPathChanged(m_activeConnectionPath);
    }

public:
    VpnManager& p;

    connection::ActiveConnectionManager::SPtr m_activeConnectionManager;

    shared_ptr<OrgFreedesktopNetworkManagerInterface> m_nmInterface;

    shared_ptr<OrgFreedesktopNetworkManagerSettingsInterface> m_settingsInterface;

    QMap<QDBusObjectPath, VpnConnection::SPtr> m_connections;

    bool m_busy = false;

    QDBusObjectPath m_activeConnectionPath;
};

VpnManager::VpnManager(connection::ActiveConnectionManager::SPtr activeConnectionManager, const QDBusConnection& systemConnection) :
        d(new Priv(*this))
{
    d->m_activeConnectionManager = activeConnectionManager;
    d->m_nmInterface = make_shared<OrgFreedesktopNetworkManagerInterface>(
                NM_DBUS_SERVICE, NM_DBUS_PATH, systemConnection);
    d->m_settingsInterface = make_shared<OrgFreedesktopNetworkManagerSettingsInterface>(
                NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS, systemConnection);

    for (const auto& path : d->m_settingsInterface->connections())
    {
        d->_newConnection(path, false);
    }
    d->updateActiveAndBusy();
    connect(d->m_settingsInterface.get(), &OrgFreedesktopNetworkManagerSettingsInterface::NewConnection, d.get(), &Priv::newConnection);
    connect(d->m_settingsInterface.get(), &OrgFreedesktopNetworkManagerSettingsInterface::ConnectionRemoved, d.get(), &Priv::connectionRemoved);
}

QList<VpnConnection::SPtr> VpnManager::connections() const
{
    return d->m_connections.values();
}

QSet<QDBusObjectPath> VpnManager::connectionPaths() const
{
    return d->m_connections.keys().toSet();
}

VpnConnection::SPtr VpnManager::connection(const QDBusObjectPath& path) const
{
    return d->m_connections.value(path);
}

QString VpnManager::addConnection(VpnConnection::Type type)
{
    static const QMap<VpnConnection::Type, QString> typeMap
    {
        {VpnConnection::Type::openvpn, "org.freedesktop.NetworkManager.openvpn"},
        {VpnConnection::Type::pptp, "org.freedesktop.NetworkManager.pptp"}
    };

    QString uuid = QUuid::createUuid().toString().mid(1,36);

    QStringMap vpnData;
    switch (type)
    {
        case VpnConnection::Type::openvpn:
            vpnData["connection-type"] = "tls";
            break;
        case VpnConnection::Type::pptp:
            vpnData["password-flags"] = "1";
            break;
    };


    QVariantDictMap connection;
    connection["connection"] = QVariantMap
    {
        {"type", "vpn"},
        {"id", d->newConnectionName()},
        {"uuid", uuid},
        {"autoconnect", false}

    };
    connection["vpn"] = QVariantMap
    {
        {"service-type", typeMap[type]},
        {"data", QVariant::fromValue(vpnData)}
    };
    connection["ipv4"] = QVariantMap
    {
        {"method", "auto"}
    };
    connection["ipv6"] = QVariantMap
    {
        {"method", "auto"}
    };

    auto reply = d->m_settingsInterface->AddConnection(connection);
    reply.waitForFinished();
    if (reply.isError())
    {
        throw domain_error(reply.error().message().toStdString());
    }

    return uuid;
}


QString VpnManager::importConnection(VpnConnection::Type type, const QString& filepath)
{
    if (filepath.isEmpty())
    {
        throw std::runtime_error("Import VPN connection failed: "
            "didn't get a filename back from the chooser!");
    }

    // Wrap the plugins list with shared_ptr to ensure clearing on dereference
    std::shared_ptr<GSList> plugins
    {
        nm_vpn_plugin_info_list_load(),
        [](auto list)
        {
            g_slist_free_full(list,
                              g_object_unref);
        }
    };
    // plugin is [[transfer:none]] from libnm so will be freed with %plugins
    auto plugin = nm_vpn_plugin_info_list_find_by_name(
        plugins.get(),
        VpnConnection::TypeString(type));

    GError* error = nullptr;

    // editor is [[transfer:none]] from libnm so will be freed with  %plugins
    auto editor = nm_vpn_plugin_info_load_editor_plugin(plugin, &error);
    if (error)
    {
        std::string msg{error->message};
        g_clear_error(&error);
        throw std::runtime_error(msg);
    }

    // Wrap the connection with shared_ptr to ensure clearing on dereference
    std::shared_ptr<NMConnection> conn
    {
        nm_vpn_editor_plugin_import(editor,
                                    filepath.toStdString().c_str(),
                                    &error),
        [](auto connection)
        {
            g_clear_object(&connection);
        }
    };
    if (error)
    {
        std::string msg{error->message};
        g_clear_error(&error);
        throw std::runtime_error(msg);
    }

    if (!nm_connection_normalize(conn.get(), NULL, NULL, &error))
    {
        std::string msg{error->message};
        g_clear_error(&error);
        throw std::runtime_error(msg);
    }

    return QString::fromUtf8(nm_connection_get_id(conn.get()));
}

}
}

#include "vpn-manager.moc"
