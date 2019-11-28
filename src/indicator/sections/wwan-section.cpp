/*
 * Copyright (C) 2014 Canonical, Ltd.
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
 */

#include <sections/wwan-section.h>
#include <menuitems/wwan-link-item.h>
#include <menuitems/switch-item.h>
#include <menuitems/text-item.h>

#include "menumodel-cpp/action-group-merger.h"
#include "menumodel-cpp/menu-merger.h"

#include "url-dispatcher-cpp/url-dispatcher.h"

#include <util/qhash-sharedptr.h>
#include <util/localisation.h>

#include <QDebug>

using namespace std;
using namespace nmofono;

class WwanSection::Private: public QObject
{
    Q_OBJECT

public:

    ActionGroupMerger::Ptr m_actionGroupMerger;

    MenuMerger::Ptr m_menuMerger;

    Menu::Ptr m_upperMenu;
    MenuMerger::Ptr m_linkMenuMerger;
    Menu::Ptr m_bottomMenu;

    Menu::Ptr m_topMenu;
    MenuItem::Ptr m_topItem;

    Manager::Ptr m_manager;

    SwitchItem::Ptr m_mobileDataSwitch;
    SwitchItem::Ptr m_hotspotSwitch;
    TextItem::Ptr m_openCellularSettings;

    QMap<wwan::Modem::Ptr, WwanLinkItem::Ptr> m_items;

    Private() = delete;
    Private(Manager::Ptr modemManager, SwitchItem::Ptr mobileDataSwitch ,SwitchItem::Ptr hotspotSwitch);

public Q_SLOTS:
    void modemsChanged();

    void openCellularSettings()
    {
        UrlDispatcher::send("settings:///system/cellular", [](string url, bool success)
        {
            if (!success)
            {
                cerr << "URL Dispatcher failed on " << url << endl;
            }
        });
    }
};

WwanSection::Private::Private(Manager::Ptr modemManager, SwitchItem::Ptr mobileDataSwitch,SwitchItem::Ptr hotspotSwitch)
    : QObject(nullptr), m_manager{modemManager}, m_mobileDataSwitch{mobileDataSwitch}, m_hotspotSwitch{hotspotSwitch}
{
    m_actionGroupMerger = make_shared<ActionGroupMerger>();
    m_menuMerger = make_shared<MenuMerger>();

    m_upperMenu  = make_shared<Menu>();
    m_linkMenuMerger = make_shared<MenuMerger>();
    m_bottomMenu = make_shared<Menu>();

    m_menuMerger->append(m_upperMenu);
    m_menuMerger->append(m_linkMenuMerger);
    m_menuMerger->append(m_bottomMenu);

    m_actionGroupMerger->add(m_mobileDataSwitch->actionGroup());

    // have the modem list in their own section.
    m_topItem = MenuItem::newSection(m_menuMerger);
    m_topMenu = make_shared<Menu>();
    m_topMenu->append(m_topItem);

    m_openCellularSettings = make_shared<TextItem>(_("Cellular settings..."), "cellular", "settings");
    connect(m_openCellularSettings.get(), &TextItem::activated, this, &Private::openCellularSettings);
    m_actionGroupMerger->add(m_openCellularSettings->actionGroup());

    connect(m_manager.get(), &Manager::linksUpdated, this, &Private::modemsChanged);
    connect(m_manager.get(), &Manager::hotspotStoredChanged, this, &Private::modemsChanged);
    modemsChanged();
}

void
WwanSection::Private::modemsChanged()
{
    auto modems = m_manager->modemLinks();
    auto current(m_items.keys().toSet());

    auto removed(current);
    removed.subtract(modems);

    auto added(modems);
    added.subtract(current);

    for (auto modem : removed)
    {
        m_linkMenuMerger->remove(m_items[modem]->menuModel());
        m_actionGroupMerger->remove(m_items[modem]->actionGroup());
        m_items.remove(modem);
    }

    for (auto modem : added)
    {
        auto item = make_shared<WwanLinkItem>(modem, m_manager);
        m_items[modem] = item;
        m_actionGroupMerger->add(item->actionGroup());
    }

    // for now just throw everything away and rebuild
    /// @todo add MenuMerger::insert() and ::find()
    m_linkMenuMerger->clear();

    multimap<int, WwanLinkItem::Ptr, wwan::Modem::Compare> sorted;
    QMapIterator<wwan::Modem::Ptr, WwanLinkItem::Ptr> it(m_items);
    while (it.hasNext())
    {
        it.next();
        sorted.insert(make_pair(it.key()->index(), it.value()));
    }
    for (auto pair : sorted)
    {
        m_linkMenuMerger->append(pair.second->menuModel());
    }

    if (modems.size() == 0)
    {
        m_bottomMenu->clear();
        m_upperMenu->clear();
    }
    else
    {
        // Check if the switch is already present
        if (m_upperMenu->find(m_mobileDataSwitch->menuItem()) == m_upperMenu->end())
        {
            // If not, add it
            m_upperMenu->insert(m_mobileDataSwitch->menuItem(), m_upperMenu->begin());
        }

        // Add the hotspot button if we have configuration stored
        if (m_manager->hotspotStored())
        {
            // Check if the switch is already present
            if (m_bottomMenu->find(m_hotspotSwitch->menuItem()) == m_bottomMenu->end())
            {
                // If not, add it
                m_bottomMenu->insert(m_hotspotSwitch->menuItem(), m_bottomMenu->begin());
            }
        }
        else
        {
            // Check if the switch is already present
            if (m_bottomMenu->find(m_hotspotSwitch->menuItem()) == m_bottomMenu->end())
            {
                // If so, remove it
                m_bottomMenu->removeAll(m_hotspotSwitch->menuItem());
            }
        }

        if (m_bottomMenu->find(m_openCellularSettings->menuItem()) == m_bottomMenu->end())
        {
            m_bottomMenu->append(m_openCellularSettings->menuItem());
        }
    }

    bool showSimIdentifier = (m_items.size() > 1);
    for(auto item: m_items.values())
    {
        item->showSimIdentifier(showSimIdentifier);
    }
}

WwanSection::WwanSection(nmofono::Manager::Ptr manager, SwitchItem::Ptr mobileDataSwitch, SwitchItem::Ptr hotspotSwitch)
    : d{new Private(manager, mobileDataSwitch, hotspotSwitch)}
{
}

WwanSection::~WwanSection()
{

}

ActionGroup::Ptr
WwanSection::actionGroup()
{
    return d->m_actionGroupMerger->actionGroup();
}

MenuModel::Ptr
WwanSection::menuModel()
{
    return d->m_topMenu;
}

#include "wwan-section.moc"
