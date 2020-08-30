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

#pragma once

#include <connectivityqt/vpn-connection.h>

#include <QAbstractItemModel>
#include <unity/util/DefinesPtrs.h>

namespace connectivityqt
{
namespace internal
{
struct VpnConnectionsListModelParameters;
}

class Q_DECL_EXPORT VpnConnectionsListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_ENUMS(Roles)

public:
    UNITY_DEFINES_PTRS(VpnConnectionsListModel);

    enum Roles
    {
        RoleId,
        RoleActive,
        RoleActivatable,
        RoleType,
        RoleConnection
    };

    VpnConnectionsListModel(const internal::VpnConnectionsListModelParameters& parameters);

    ~VpnConnectionsListModel();

    int columnCount(const QModelIndex &parent) const override;

    int rowCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[RoleId] = "id";
        roles[RoleActive] = "active";
        roles[RoleActivatable] = "activatable";
        roles[RoleType] = "type";
        roles[RoleConnection] = "connection";
        return roles;
    }

public Q_SLOTS:
    void add(VpnConnection::Type type);

    void importFile(VpnConnection::Type type, const QString &filepath);

    void remove(VpnConnection* connection);

Q_SIGNALS:
    void addFinished(VpnConnection * connection);

protected:
    class Priv;
    std::shared_ptr<Priv> d;
};

}

Q_DECLARE_METATYPE(connectivityqt::VpnConnection*)
