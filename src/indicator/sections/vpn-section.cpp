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
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include <sections/vpn-section.h>
#include <menuitems/vpn-item.h>
#include <menuitems/switch-item.h>
#include <menuitems/text-item.h>

#include "menumodel-cpp/action-group-merger.h"
#include "menumodel-cpp/menu-merger.h"

#include "url-dispatcher-cpp/url-dispatcher.h"

#include <util/qhash-sharedptr.h>
#include <util/localisation.h>

#include <QDebug>

using namespace std;
using namespace nmofono::vpn;

class VpnSection::Private: public QObject
{
    Q_OBJECT

public:
    VpnManager::SPtr m_manager;

    ActionGroupMerger::Ptr m_actionGroupMerger;

    Menu::Ptr m_topMenu;
    MenuItem::Ptr m_topItem;

    MenuItem::Ptr m_vpnSettingsItem;
    Menu::Ptr m_vpnSettingsMenu;
    TextItem::Ptr m_openVpnSettings;

    Menu::Ptr m_connectionsMenu;
    MenuItem::Ptr m_connectionsItem;

    MenuMerger::Ptr m_menuMerger;

    QMap<VpnConnection::SPtr, VpnItem::SPtr> m_items;

public Q_SLOTS:
    void vpnConnectionsChanged()
    {
        auto connectionsList = m_manager->connections();
        auto connections = connectionsList.toSet();
        auto current(m_items.keys().toSet());

        auto removed(current);
        removed.subtract(connections);

        auto added(connections);
        added.subtract(current);

        for (auto connection : removed)
        {
            m_actionGroupMerger->remove(m_items[connection]->actionGroup());
            m_items.remove(connection);
        }

        for (auto connection : added)
        {
            auto item = make_shared<VpnItem>(connection);
            m_items[connection] = item;
            m_actionGroupMerger->add(item->actionGroup());
        }

        // for now just throw everything away and rebuild
        /// @todo add MenuMerger::insert() and ::find()
        m_connectionsMenu->clear();

        multimap<QString, VpnItem::SPtr> sorted;
        QMapIterator<VpnConnection::SPtr, VpnItem::SPtr> it(m_items);
        while (it.hasNext())
        {
            it.next();
            sorted.insert(make_pair(it.key()->id(), it.value()));
        }
        for (auto pair : sorted)
        {
            m_connectionsMenu->append(pair.second->menuItem());
        }

        auto settingsItemIt = m_vpnSettingsMenu->find(m_openVpnSettings->menuItem());
        if (m_items.empty())
        {
            if (settingsItemIt != m_vpnSettingsMenu->end())
            {
                m_vpnSettingsMenu->remove(settingsItemIt);
            }
        }
        else
        {
            if (settingsItemIt == m_vpnSettingsMenu->end())
            {
                m_vpnSettingsMenu->insert(m_openVpnSettings->menuItem(), m_vpnSettingsMenu->begin());
            }
        }
    }

    void openVpnSettings()
    {
        UrlDispatcher::send("settings:///system/vpn", [](string url, bool success)
        {
            if (!success)
            {
                cerr << "URL Dispatcher failed on " << url << endl;
            }
        });
    }
};

VpnSection::VpnSection(nmofono::vpn::VpnManager::SPtr vpnManager)
    : d(new Private)
{
    d->m_manager = vpnManager;

    d->m_actionGroupMerger = make_shared<ActionGroupMerger>();
    d->m_menuMerger = make_shared<MenuMerger>();

    d->m_vpnSettingsMenu = make_shared<Menu>();
    d->m_openVpnSettings = make_shared<TextItem>(_("VPN settings..."), "vpn", "settings");

    d->m_connectionsItem = MenuItem::newSection(d->m_menuMerger);
    d->m_connectionsMenu = make_shared<Menu>();
    d->m_connectionsMenu->append(d->m_connectionsItem);

    d->m_menuMerger->append(d->m_connectionsMenu);
    d->m_menuMerger->append(d->m_vpnSettingsMenu);

    // have the VPN list in their own section.
    d->m_topItem = MenuItem::newSection(d->m_menuMerger);
    d->m_topMenu = make_shared<Menu>();
    d->m_topMenu->append(d->m_topItem);

    QObject::connect(d->m_openVpnSettings.get(), &TextItem::activated, d.get(), &Private::openVpnSettings);
    d->m_actionGroupMerger->add(d->m_openVpnSettings->actionGroup());

    QObject::connect(d->m_manager.get(), &VpnManager::connectionsChanged, d.get(), &Private::vpnConnectionsChanged);
    d->vpnConnectionsChanged();
}

ActionGroup::Ptr
VpnSection::actionGroup()
{
    return d->m_actionGroupMerger->actionGroup();
}

MenuModel::Ptr
VpnSection::menuModel()
{
    return d->m_topMenu;
}

#include "vpn-section.moc"
