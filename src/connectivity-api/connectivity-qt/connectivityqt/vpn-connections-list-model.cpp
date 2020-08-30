/*
 * Copyright © 2014 Canonical Ltd.
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
 *     Pete Woods <pete.woods@canonical.com>
 */

#include <connectivityqt/internal/vpn-connection-list-model-parameters.h>
#include <connectivityqt/openvpn-connection.h>
#include <connectivityqt/pptp-connection.h>
#include <connectivityqt/vpn-connections-list-model.h>

#include <VpnConnectionInterface.h>
#include <dbus-types.h>

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDebug>
#include <QList>

using namespace std;

namespace connectivityqt
{

using namespace internal;

class VpnConnectionsListModel::Priv: public QObject
{
    Q_OBJECT

public:
    Priv(VpnConnectionsListModel& parent) :
        p(parent)
    {
    }

    void updatePaths(const QVariant& value)
    {
        QList<QDBusObjectPath> tmp;
        qvariant_cast<QDBusArgument>(value) >> tmp;
        auto paths = tmp.toSet();

        QSet<QDBusObjectPath> current;
        for (const auto& connection: m_vpnConnections)
        {
            current << connection->path();
        }

        auto toRemove(current);
        toRemove.subtract(paths);

        auto toAdd(paths);
        toAdd.subtract(current);

        QMutableListIterator<VpnConnection::SPtr> i(m_vpnConnections);
        int idx = 0;
        while (i.hasNext())
        {
            auto vpnConnection(i.next());
            if (toRemove.contains(vpnConnection->path()))
            {
                p.beginRemoveRows(QModelIndex(), idx, idx);
                i.remove();
                p.endRemoveRows();
            }
            else
            {
                ++idx;
            }
        }

        if (!toAdd.isEmpty())
        {
            p.beginInsertRows(QModelIndex(), m_vpnConnections.size(), m_vpnConnections.size() + toAdd.size() - 1);
            for (const auto& path: toAdd)
            {
                ComUbuntuConnectivity1VpnVpnConnectionInterface vpnInterface(
                        DBusTypes::DBUS_NAME, path.path(), m_propertyCache->connection());

                VpnConnection::SPtr vpnConnection;
                switch(vpnInterface.type())
                {
                    case VpnConnection::Type::OPENVPN:
                        vpnConnection.reset(new OpenvpnConnection(path, m_propertyCache->connection()),
                                [](QObject* self){self->deleteLater();});
                        break;
                    default:
                        vpnConnection.reset(new PptpConnection(path, m_propertyCache->connection()),
                                [](QObject* self){self->deleteLater();});
                        break;
                }
                if (vpnConnection)
                {
                    m_objectOwner(vpnConnection.get());
                    m_vpnConnections << vpnConnection;
                    connect(vpnConnection.get(), &VpnConnection::idChanged, this, &Priv::connectionIdChanged);
                    connect(vpnConnection.get(), &VpnConnection::activeChanged, this, &Priv::connectionActiveChanged);
                    connect(vpnConnection.get(), &VpnConnection::activatableChanged, this, &Priv::connectionActivatableChanged);
                    connect(vpnConnection.get(), &VpnConnection::remove, this, &Priv::removeRequested);
                }
            }
            p.endInsertRows();
        }
    }

    QModelIndex findVpnConnection(QObject* o)
    {
        auto vpnConnection = qobject_cast<VpnConnection*>(o);
        for (auto i = m_vpnConnections.constBegin(); i != m_vpnConnections.constEnd(); ++i)
        {
            if (i->get() == vpnConnection)
            {
                return p.index(std::distance(m_vpnConnections.constBegin(), i));
            }
        }
        return QModelIndex();
    }

    void remove(const VpnConnection& connection)
    {
        auto reply = m_writeInterface->RemoveVpnConnection(connection.path());
        auto watcher(new QDBusPendingCallWatcher(reply, this));
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &Priv::removeConnectionFinished);
    }

public Q_SLOTS:
    void connectionIdChanged(const QString&)
    {
        auto idx = findVpnConnection(sender());
        p.dataChanged(idx, idx, {VpnConnectionsListModel::Roles::RoleId});
    }

    void connectionActiveChanged(bool)
    {
        auto idx = findVpnConnection(sender());
        p.dataChanged(idx, idx, {VpnConnectionsListModel::Roles::RoleActive});
    }

    void connectionActivatableChanged(bool)
    {
        auto idx = findVpnConnection(sender());
        p.dataChanged(idx, idx, {VpnConnectionsListModel::Roles::RoleActivatable});
    }

    void propertyChanged(const QString& name, const QVariant& value)
    {
        if (name == "VpnConnections")
        {
            updatePaths(value);
        }
    }

    void addConnectionFinished(QDBusPendingCallWatcher *call)
    {
        QDBusPendingReply<QDBusObjectPath> reply = *call;
        if (reply.isError())
        {
            qWarning() << __PRETTY_FUNCTION__ << reply.error().message();
        }
        else
        {
            QDBusObjectPath path(reply);
            VpnConnection::SPtr connection;
            for (const auto& tmp : m_vpnConnections)
            {
                if (tmp->path() == path)
                {
                    connection = tmp;
                    break;
                }
            }
            if (connection)
            {
                Q_EMIT p.addFinished(connection.get());
            }
            else
            {
                qWarning() << __PRETTY_FUNCTION__ << "New connection with path:" << path.path() << " could not be found";
            }
        }
        call->deleteLater();
    }

    void removeConnectionFinished(QDBusPendingCallWatcher *call)
    {
        QDBusPendingReply<> reply = *call;
        if (reply.isError())
        {
            qWarning() << __PRETTY_FUNCTION__ << reply.error().message();
        }
        call->deleteLater();
    }

    void removeRequested()
    {
        auto connection = qobject_cast<VpnConnection*>(sender());
        if (connection)
        {
            remove(*connection);
        }
    }

public:
    VpnConnectionsListModel& p;

    function<void(QObject*)> m_objectOwner;

    shared_ptr<ComUbuntuConnectivity1PrivateInterface> m_writeInterface;

    DBusPropertyCache::SPtr m_propertyCache;

    QList<VpnConnection::SPtr> m_vpnConnections;
};

VpnConnectionsListModel::VpnConnectionsListModel(const internal::VpnConnectionsListModelParameters& parameters) :
        d(new Priv(*this))
{
    d->m_objectOwner = parameters.objectOwner;
    d->m_writeInterface = parameters.writeInterface;
    d->m_propertyCache = parameters.propertyCache;
    d->updatePaths(d->m_propertyCache->get("VpnConnections"));
    connect(d->m_propertyCache.get(), &DBusPropertyCache::propertyChanged, d.get(), &Priv::propertyChanged);
}

VpnConnectionsListModel::~VpnConnectionsListModel()
{
}

int VpnConnectionsListModel::columnCount(const QModelIndex &) const
{
    return 1;
}

int VpnConnectionsListModel::rowCount(const QModelIndex &) const
{
    return d->m_vpnConnections.size();
}

QVariant VpnConnectionsListModel::data(const QModelIndex &index, int role) const
{
    int row(index.row());
    if (row < 0 || row >= d->m_vpnConnections.size())
    {
        return QVariant();
    }

    auto vpnConnection = d->m_vpnConnections.value(row);

    switch (role)
    {
        case Roles::RoleId:
            return vpnConnection->id();
            break;
        case Roles::RoleActive:
            return vpnConnection->active();
            break;
        case Roles::RoleActivatable:
            return vpnConnection->activatable();
            break;
        case Roles::RoleType:
            return static_cast<int>(vpnConnection->type());
            break;
        case Roles::RoleConnection:
            return QVariant::fromValue(qobject_cast<QObject*>(vpnConnection.get()));
            break;
        default:
            break;
    }

    return QVariant();
}

bool VpnConnectionsListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row(index.row());
    if (row < 0 || row >= d->m_vpnConnections.size())
    {
        return false;
    }

    auto vpnConnection = d->m_vpnConnections.value(row);

    switch (role)
    {
        case Roles::RoleId:
            vpnConnection->setId(value.toString());
            break;
        case Roles::RoleActive:
            vpnConnection->setActive(value.toBool());
            break;
        default:
            return false;
        return true;
    }

    return false;
}

Qt::ItemFlags VpnConnectionsListModel::flags(const QModelIndex & index) const
{
    int row(index.row());
    if (row < 0 || row >= d->m_vpnConnections.size())
    {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void VpnConnectionsListModel::add(VpnConnection::Type type)
{
    auto reply = d->m_writeInterface->AddVpnConnection(static_cast<int>(type));
    auto watcher(new QDBusPendingCallWatcher(reply, this));
    connect(watcher, &QDBusPendingCallWatcher::finished, d.get(), &Priv::addConnectionFinished);
}

void VpnConnectionsListModel::importFile(VpnConnection::Type type, const QString &filepath)
{
    auto reply = d->m_writeInterface->ImportVpnConnection(static_cast<int>(type), filepath);
    auto watcher(new QDBusPendingCallWatcher(reply, this));
    connect(watcher, &QDBusPendingCallWatcher::finished, d.get(), &Priv::addConnectionFinished);
}

void VpnConnectionsListModel::remove(VpnConnection* connection)
{
    d->remove(*connection);
}

}

#include "vpn-connections-list-model.moc"
