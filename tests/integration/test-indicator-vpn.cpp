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

#include <indicator-network-test-base.h>

#include <QDebug>
#include <QTestEventLoop>
#include <QSignalSpy>

using namespace std;
using namespace testing;
namespace mh = unity::gmenuharness;

namespace
{

class TestIndicatorVpn: public IndicatorNetworkTestBase
{
};

TEST_F(TestIndicatorVpn, ReadStateAtStartup)
{
    // Add VPN configurations
    auto appleConnection = createVpnConnection("apple");
    auto bananaConnection = createVpnConnection("banana");
    auto coconutConnection = createVpnConnection("coconut");

    // Add a physical device to use for the connection
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    // Activate the banana connection
    auto activeConnection = createActiveConnection("0", device, bananaConnection, "/");

    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
                .item(vpnConnection("apple"))
                .item(vpnConnection("banana", ConnectionStatus::connected))
                .item(vpnConnection("coconut"))
                .item(vpnSettings())
            )
        ).match());
}

TEST_F(TestIndicatorVpn, FollowStateChanges)
{
    // Add VPN configurations
    auto appleConnection = createVpnConnection("apple");
    auto bananaConnection = createVpnConnection("banana");

    // Add a physical device to use for the connection
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
                .item(vpnConnection("apple"))
                .item(vpnConnection("banana"))
                .item(vpnSettings())
            )
        ).match());

    // Activate the apple connection
    auto activeConnection = createActiveConnection("0", device, appleConnection, "/");

    // Create another inactive connection
    auto coconutConnection = createVpnConnection("coconut");

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
                .item(vpnConnection("apple", ConnectionStatus::connected))
                .item(vpnConnection("banana"))
                .item(vpnConnection("coconut"))
                .item(vpnSettings())
            )
        ).match());
}

TEST_F(TestIndicatorVpn, ActivatesConnection)
{
    // Add VPN configurations
    auto appleConnection = createVpnConnection("apple");

    // Add a physical device to use for the connection
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    ASSERT_NO_THROW(startIndicator());

    QSignalSpy networkManagerMockInterfaceSpy(&networkManagerMockInterface(),
                                   SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
                .item(vpnConnection("apple")
                    .activate()
                )
                .item(vpnSettings())
            )
        ).match());

    WAIT_FOR_SIGNALS(networkManagerMockInterfaceSpy, 1);
    EXPECT_EQ(qVariantToString(
        QVariantList()
            << QVariant("ActivateConnection")
            << QVariant(
                QVariantList()
                    << QVariant::fromValue(QDBusObjectPath(appleConnection))
                    << QVariant::fromValue(QDBusObjectPath("/"))
                    << QVariant::fromValue(QDBusObjectPath("/"))
               )
        ),
        qVariantToString(networkManagerMockInterfaceSpy.first())
    );
}

TEST_F(TestIndicatorVpn, DeactivatesConnection)
{
    // Add VPN configurations
    auto appleConnection = createVpnConnection("apple");

    // Add a physical device to use for the connection
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    // Activate the apple connection
    auto activeConnection = createActiveConnection("0", device, appleConnection, "/");

    ASSERT_NO_THROW(startIndicator());

    QSignalSpy networkManagerMockInterfaceSpy(&networkManagerMockInterface(),
                                   SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
                .item(vpnConnection("apple", ConnectionStatus::connected)
                    .activate()
                )
                .item(vpnSettings())
            )
        ).match());

    WAIT_FOR_SIGNALS(networkManagerMockInterfaceSpy, 2);
    EXPECT_EQ(qVariantToString(
        QVariantList()
            << QVariant("DeactivateConnection")
            << QVariant(
                QVariantList()
                    << QVariant::fromValue(QDBusObjectPath(activeConnection))
               )
        ),
        qVariantToString(networkManagerMockInterfaceSpy.first())
    );
}

TEST_F(TestIndicatorVpn, importConnection)
{
    // Add VPN configurations
    auto sampleConnection = importVpnConnection();

    // Add a physical device to use for the connection
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
                .item(vpnConnection("sample"))
                .item(vpnSettings())
            )
        ).match());
}

} // namespace
