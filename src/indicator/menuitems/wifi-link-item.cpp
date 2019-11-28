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

#include "wifi-link-item.h"

#include "menuitems/text-item.h"
#include "menuitems/access-point-item.h"

#include "menumodel-cpp/action-group.h"
#include "menumodel-cpp/action-group-merger.h"
#include "menumodel-cpp/menu.h"
#include "menumodel-cpp/menu-merger.h"

#include <util/qhash-sharedptr.h>
#include <util/localisation.h>

using namespace nmofono;

#include <algorithm>
#include <locale>

class WifiLinkItem::Private : public QObject
{
    Q_OBJECT

public:
    ActionGroupMerger::Ptr m_actionGroupMerger;

    wifi::WifiLink::Ptr m_link;

    /// @todo do something with me...
    Action::Ptr m_actionBusy;

    wifi::AccessPoint::Ptr m_activeAccessPoint;
    QMap<wifi::AccessPoint::Ptr, AccessPointItem::Ptr> m_accessPoints;

    Menu::Ptr m_topMenu;

    Menu::Ptr m_connectedBeforeApsMenu;
    Menu::Ptr m_neverConnectedApsMenu;
    MenuMerger::Ptr m_apsMerger;

    Menu::Ptr m_bottomMenu;

    MenuMerger::Ptr m_rootMerger;
    MenuItem::Ptr m_item;

    std::function<bool(MenuItem::Ptr a, MenuItem::Ptr b)> m_accessPointCompare;

    std::string m_icon;
    std::string m_a11ydesc;

    TextItem::Ptr m_otherNetwork;

public:


    Private() = delete;
    ~Private() {}
    Private(wifi::WifiLink::Ptr link)
        : m_link {link}
    {
        m_actionGroupMerger = std::make_shared<ActionGroupMerger>();

        m_topMenu = std::make_shared<Menu>();

        m_connectedBeforeApsMenu = std::make_shared<Menu>();
        m_neverConnectedApsMenu = std::make_shared<Menu>();
        m_apsMerger = std::make_shared<MenuMerger>();

        m_bottomMenu = std::make_shared<Menu>();

        m_rootMerger = std::make_shared<MenuMerger>();

        m_accessPointCompare = [](MenuItem::Ptr a, MenuItem::Ptr b){
            // order alphabetically by SSID

            QString a_label = a->label();
            QString b_label = b->label();

            QString a_upper = a_label.toUpper();
            QString b_upper = b_label.toUpper();

            return a_upper < b_upper;
        };

        updateAccessPoints(m_link->accessPoints());
        connect(m_link.get(), &wifi::WifiLink::accessPointsUpdated, this, &Private::updateAccessPoints);

        updateActiveAccessPoint(m_link->activeAccessPoint());
        connect(m_link.get(), &wifi::WifiLink::activeAccessPointUpdated, this, &Private::updateActiveAccessPoint);

        m_otherNetwork = std::make_shared<TextItem>(_("Other network..."), "wifi", "othernetwork");
        //m_actionGroupMerger->add(*m_otherNetwork);

        m_rootMerger->append(m_topMenu);

        m_apsMerger->append(m_connectedBeforeApsMenu);
        m_apsMerger->append(m_neverConnectedApsMenu);
        m_rootMerger->append(m_apsMerger);

        //m_bottomMenu->append(*m_otherNetwork);
        m_rootMerger->append(m_bottomMenu);

        m_item = MenuItem::newSection(m_rootMerger);
    }

public Q_SLOTS:
    void updateAccessPoints(const QSet<wifi::AccessPoint::Ptr>& accessPoints)
    {
        /// @todo previously connected
        /// @todo apply visibility policy.

        auto old(m_accessPoints.keys().toSet());

        auto added(accessPoints);
        added.subtract(old);

        auto removed(old);
        removed.subtract(accessPoints);


        for (auto ap: removed) {
            bool isActive = (ap == m_activeAccessPoint);
            if (isActive)
                m_connectedBeforeApsMenu->removeAll(m_accessPoints[ap]->menuItem());
            else
                m_neverConnectedApsMenu->removeAll(m_accessPoints[ap]->menuItem());
            /// @todo disconnect activated...
            m_actionGroupMerger->remove(m_accessPoints[ap]->actionGroup());
            m_accessPoints.remove(ap);
        }

        for (auto ap : added) {

            /// @todo handle hidden APs all the way
            if (ap->ssid().isEmpty())
                continue;

            bool isActive = (ap == m_activeAccessPoint);
            auto item = std::make_shared<AccessPointItem>(ap, isActive);
            connect(item.get(), &AccessPointItem::activated, [this, ap](){
                m_link->connect_to(ap);
            });
            m_accessPoints[ap] = item;
            m_actionGroupMerger->add(item->actionGroup());
            if (isActive) {
                updateActiveAccessPoint(m_activeAccessPoint);
            } else {
                m_neverConnectedApsMenu->insert(item->menuItem(), m_accessPointCompare);
            }
        }
    }

    void updateActiveAccessPoint(wifi::AccessPoint::Ptr ap)
    {
        m_activeAccessPoint = ap;

        auto current = m_connectedBeforeApsMenu->begin();
        if (current != m_connectedBeforeApsMenu->end()) {
            // move to other menu
            m_neverConnectedApsMenu->insert(*current, m_accessPointCompare);
            m_connectedBeforeApsMenu->clear();
        }

        QMapIterator<wifi::AccessPoint::Ptr, AccessPointItem::Ptr> i(m_accessPoints);
        while (i.hasNext()) {
            i.next();
            auto menuItem = i.value();
            if (ap && ap == i.key()) {
                m_connectedBeforeApsMenu->insert(menuItem->menuItem(), m_connectedBeforeApsMenu->begin());
                menuItem->setActive(true);
                m_neverConnectedApsMenu->removeAll(menuItem->menuItem());
                continue;
            }
            menuItem->setActive(false);
        }
    }

};


WifiLinkItem::WifiLinkItem(wifi::WifiLink::Ptr link)
    : d{new Private(link)}
{
}

WifiLinkItem::~WifiLinkItem()
{}

ActionGroup::Ptr
WifiLinkItem::actionGroup()
{
    return d->m_actionGroupMerger->actionGroup();
}

MenuItem::Ptr
WifiLinkItem::menuItem()
{
    return d->m_item;
}

#include "wifi-link-item.moc"
