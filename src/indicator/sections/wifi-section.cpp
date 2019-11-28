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

#include <sections/wifi-section.h>
#include <menuitems/wifi-link-item.h>
#include <menuitems/switch-item.h>
#include <menuitems/text-item.h>

#include "url-dispatcher-cpp/url-dispatcher.h"

#include "menumodel-cpp/action-group.h"
#include "menumodel-cpp/action-group-merger.h"
#include "menumodel-cpp/menu.h"
#include "menumodel-cpp/menu-merger.h"

#include <util/localisation.h>

using namespace nmofono;

class WifiSection::Private : public QObject
{
    Q_OBJECT
public:
    Manager::Ptr m_manager;

    ActionGroupMerger::Ptr m_actionGroupMerger;
    Menu::Ptr m_menu;
    Menu::Ptr m_settingsMenu;

    SwitchItem::Ptr m_switch;

    WifiLinkItem::Ptr m_wifiLink;
    TextItem::Ptr m_openWifiSettings;

    Private(Manager::Ptr manager, SwitchItem::Ptr wifiSwitch)
        : m_manager{manager}, m_switch{wifiSwitch}
    {
        m_actionGroupMerger = std::make_shared<ActionGroupMerger>();
        m_menu = std::make_shared<Menu>();
        m_settingsMenu = std::make_shared<Menu>();

        /// @todo don't now really care about actully being able to detach the whole
        ///       wifi chipset. on touch devices we always have wifi.
        if (m_manager->hasWifi()) {
            m_menu->append(m_switch->menuItem());
            m_settingsMenu->append(m_switch->menuItem());
        }

        m_openWifiSettings = std::make_shared<TextItem>(_("Wi-Fi settings..."), "wifi", "settings");
        connect(m_openWifiSettings.get(), &TextItem::activated, this, &Private::openWiFiSettings);

        m_actionGroupMerger->add(m_openWifiSettings->actionGroup());
        m_menu->append(m_openWifiSettings->menuItem());

        // We have this last because the menu item insertion location
        // depends on the presence of the WiFi settings item.
        updateLinks();
        connect(m_manager.get(), &Manager::linksUpdated, this, &Private::updateLinks);
    }

public Q_SLOTS:
    void openWiFiSettings()
    {
        UrlDispatcher::send("settings:///system/wifi", [](std::string url, bool success){
            if (!success)
                std::cerr << "URL Dispatcher failed on " << url << std::endl;
        });
    }

    void updateLinks()
    {
        // remove all and recreate. we have top 1 now anyway
        if (m_wifiLink) {
            m_actionGroupMerger->remove(m_wifiLink->actionGroup());
            m_menu->removeAll(m_wifiLink->menuItem());
            m_settingsMenu->removeAll(m_wifiLink->menuItem());
            m_wifiLink.reset();
        }

        for (auto wifi_link : m_manager->wifiLinks()) {
            m_wifiLink = std::make_shared<WifiLinkItem>(wifi_link);

            m_actionGroupMerger->add(m_wifiLink->actionGroup());
            auto comp = [this](MenuItem::Ptr, MenuItem::Ptr other)
            {
                return other == m_openWifiSettings->menuItem();
            };
            m_menu->insert(m_wifiLink->menuItem(), comp);
            m_settingsMenu->insert(m_wifiLink->menuItem(), comp);

            // just take the first one
            break;
        }
    }
};

WifiSection::WifiSection(Manager::Ptr manager, SwitchItem::Ptr wifiSwitch)
    : d{new Private(manager, wifiSwitch)}
{
}

WifiSection::~WifiSection()
{}

ActionGroup::Ptr
WifiSection::actionGroup()
{
    return d->m_actionGroupMerger->actionGroup();
}

MenuModel::Ptr
WifiSection::menuModel()
{
    return d->m_menu;
}

MenuModel::Ptr
WifiSection::settingsModel()
{
    return d->m_settingsMenu;
}

#include "wifi-section.moc"
