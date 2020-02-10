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
using namespace connectivityqt;
namespace mh = unity::gmenuharness;

namespace
{

class TestIndicator: public IndicatorNetworkTestBase
{
};

TEST_F(TestIndicator, BasicMenuContents)
{
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .action("indicator.phone.network-status")
            .state_icons({"gsm-3g-full", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::all)
            .submenu()
            .item(flightModeSwitch())            
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("", "fake.tel", "gsm-3g-full"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
            )
        ).match());
}

TEST_F(TestIndicator, OneDisconnectedAccessPointAtStartup)
{
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);
    auto ap = createAccessPoint("0", "the ssid", device);
    auto connection = createAccessPointConnection("0", "the ssid", device);

    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "nm-no-connection"})
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()) // <-- modems are under here
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("the ssid",
                      Secure::wpa,
                      ApMode::infra,
                      ConnectionStatus::disconnected)
                )
            )
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
            )
        ).match());
}

TEST_F(TestIndicator, OneConnectedAccessPointAtStartup)
{
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap = createAccessPoint("0", "the ssid", device);
    auto connection = createAccessPointConnection("0", "the ssid", device);
    createActiveConnection("0", device, connection, ap);

    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "nm-signal-100-secure"})
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()) // <-- modems are under here
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("the ssid",
                      Secure::wpa,
                      ApMode::infra,
                      ConnectionStatus::connected)
                )
            )
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
            )
        ).match());
}

TEST_F(TestIndicator, AddOneDisconnectedAccessPointAfterStartup)
{
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);

    ASSERT_NO_THROW(startIndicator());
    auto ap = createAccessPoint("0", "the ssid", device);
    auto connection = createAccessPointConnection("0", "the ssid", device);

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("the ssid",
                      Secure::wpa,
                      ApMode::infra,
                      ConnectionStatus::disconnected)
                )
            )
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
            )
        ).match());
}

TEST_F(TestIndicator, AddOneConnectedAccessPointAfterStartup)
{
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);

    ASSERT_NO_THROW(startIndicator());

    auto ap = createAccessPoint("0", "the ssid", device);
    auto connection = createAccessPointConnection("0", "the ssid", device);
    createActiveConnection("0", device, connection, ap);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("the ssid",
                      Secure::wpa,
                      ApMode::infra,
                      ConnectionStatus::connected)
                )
            )
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
            )
        ).match());
}

TEST_F(TestIndicator, SecondModem)
{
    createModem("ril_1"); // ril_0 already exists
    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .submenu()
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-full"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch())
            .item(wifiSettings())
            .item(mh::MenuItemMatcher()
                .section()
            )
        ).match());
}

TEST_F(TestIndicator, FlightModeTalksToURfkill)
{
    ASSERT_NO_THROW(startIndicator());

    auto& urfkillInterface = dbusMock.urfkillInterface();
    QSignalSpy urfkillSpy(&urfkillInterface, SIGNAL(FlightModeChanged(bool)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
            .item(flightModeSwitch(false)
                .activate() // <--- Activate the action now
            )
        ).match());

    // Wait to be notified that flight mode was enabled
    WAIT_FOR_SIGNALS(urfkillSpy, 1);
    EXPECT_EQ(urfkillSpy.first(), QVariantList() << QVariant(true));
}

TEST_F(TestIndicator, IndicatorListensToURfkill)
{
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap = createAccessPoint("0", "the ssid", device);
    auto connection = createAccessPointConnection("0", "the ssid", device);
    createActiveConnection("0", device, connection, ap);

    ASSERT_NO_THROW(startIndicator());

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .has_exactly(1) // <-- has one access point
            )
        ).match());

    ASSERT_TRUE(dbusMock.urfkillInterface().FlightMode(true));

    removeWifiConnection(device, connection);
    removeAccessPoint(device, ap);

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true))
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty() // <-- no access points
            )
        ).match());
}

TEST_F(TestIndicator, SimStates_NoSIM)
{
    // set flight mode off, wifi off, and cell data off
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set no sim
    setSimManagerProperty(firstModem(), "Present", false);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is just a 0-bar wifi icon
    // check sim status shows “No SIM” with crossed sim card icon
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
        ).match());
}

TEST_F(TestIndicator, SimStates_NoSIM2)
{
    // set flight mode off, wifi off, and cell data off
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set no sim 2
    auto modem1 = createModem("ril_1");
    setSimManagerProperty(modem1, "Present", false);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a 4-bar signal icon and a 0-bar wifi icon
    // check sim 2 status shows “No SIM” with crossed sim card icon
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
        ).match());
}

TEST_F(TestIndicator, SimStates_LockedSIM)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim locked
    setSimManagerProperty(firstModem(), "PinRequired", "pin");

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a locked sim card and a 0-bar wifi icon.
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"simcard-locked", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("", "SIM Locked", "simcard-locked", true)
                      .string_attribute("x-canonical-modem-locked-action", "indicator.modem.1::locked")
                )
                .item(cellularSettings())
            )
        ).match());

    // set sim unlocked
    setSimManagerProperty(firstModem(), "PinRequired", "none");

    // check indicator is a 4-bar signal icon and a 0-bar wifi icon
    // check sim status shows correct carrier name with 4-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-full"))
                .item(cellularSettings())
            )
        ).match());
}

TEST_F(TestIndicator, SimStates_LockedSIM2)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim 2 locked
    auto modem1 = createModem("ril_1");
    setSimManagerProperty(modem1, "PinRequired", "pin");

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a 4-bar signal icon, a locked sim card and a 0-bar wifi icon.
    // check sim 2 status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "simcard-locked", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "SIM Locked", "simcard-locked", true))
                .item(cellularSettings())
            )
        ).match());

    // set sim 2 unlocked
    setSimManagerProperty(modem1, "PinRequired", "none");

    // check indicator is 4-bar signal icon, a 4-bar signal icon and a 0-bar wifi icon
    // check sim statuses show correct carrier names with 4-bar signal icons.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-full", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-full"))
                .item(cellularSettings())
            )
        ).match());
}

TEST_F(TestIndicator, SimStates_UnlockedSIM)
{
    // set flight mode off, wifi off, cell data off, sim in, and sim unlocked
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set no signal
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(0)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a crossed signal icon and a 0-bar wifi icon.
    // check sim status shows “No Signal” with crossed signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-no-service", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No Signal", "gsm-3g-no-service"))
                .item(cellularSettings())
            )
        ).match());

    // set sim searching
    setNetworkRegistrationProperty(firstModem(), "Status", "searching");

    // check indicator is a disabled signal icon and a 0-bar wifi icon.
    // check sim status shows “Searching” with disabled signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-disabled", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "Searching", "gsm-3g-disabled"))
                .item(cellularSettings())
            )
        ).match());

    // set sim registered
    setNetworkRegistrationProperty(firstModem(), "Status", "registered");

    // set signal strength to 1
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(1)));

    // check indicator is a 0-bar signal icon and a 0-bar wifi icon.
    // check sim status shows correct carrier name with 0-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-none", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-none"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 6
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(6)));

    // check indicator is a 1-bar signal icon and a 0-bar wifi icon.
    // check sim status shows correct carrier name with 1-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-low", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-low"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 16
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(16)));

    // check indicator is a 2-bar signal icon and a 0-bar wifi icon.
    // check sim status shows correct carrier name with 2-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-medium", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-medium"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 26
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(26)));

    // check indicator is a 3-bar signal icon and a 0-bar wifi icon.
    // check sim status shows correct carrier name with 3-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-high", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-high"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 39
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(39)));

    // check indicator is a 4-bar signal icon and a 0-bar wifi icon.
    // check sim status shows correct carrier name with 4-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-full"))
                .item(cellularSettings())
            )
        ).match());
}

TEST_F(TestIndicator, SimStates_UnlockedSIM2)
{
    // set flight mode off, wifi off, cell data off, sim in, and sim unlocked
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set no signal on sim 2
    auto modem1 = createModem("ril_1");
    setNetworkRegistrationProperty(modem1, "Strength", QVariant::fromValue(uchar(0)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a 4-bar signal icon, a crossed signal icon and a 0-bar wifi icon.
    // check sim 2 status shows “No Signal” with crossed signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-no-service", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "No Signal", "gsm-3g-no-service"))
                .item(cellularSettings())
            )
        ).match());

    // set sim searching
    setNetworkRegistrationProperty(modem1, "Status", "searching");

    // check indicator is a 4-bar signal icon, a disabled signal icon and a 0-bar wifi icon.
    // check sim 2 status shows “Searching” with disabled signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-disabled", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "Searching", "gsm-3g-disabled"))
                .item(cellularSettings())
            )
        ).match());

    // set sim registered
    setNetworkRegistrationProperty(modem1, "Status", "registered");

    // set signal strength to 1
    setNetworkRegistrationProperty(modem1, "Strength", QVariant::fromValue(uchar(1)));

    // check indicator is a 4-bar signal icon, a 0-bar signal icon and a 0-bar wifi icon.
    // check sim 2 status shows correct carrier name with 0-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-none", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-none"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 6
    setNetworkRegistrationProperty(modem1, "Strength", QVariant::fromValue(uchar(6)));

    // check indicator is a 4-bar signal icon, a 1-bar signal icon and a 0-bar wifi icon.
    // check sim 2 status shows correct carrier name with 1-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-low", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-low"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 16
    setNetworkRegistrationProperty(modem1, "Strength", QVariant::fromValue(uchar(16)));

    // check indicator is a 4-bar signal icon, a 2-bar signal icon and a 0-bar wifi icon.
    // check sim 2 status shows correct carrier name with 2-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-medium", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-medium"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 26
    setNetworkRegistrationProperty(modem1, "Strength", QVariant::fromValue(uchar(26)));

    // check indicator is a 4-bar signal icon, a 3-bar signal icon and a 0-bar wifi icon.
    // check sim 2 status shows correct carrier name with 3-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-high", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-high"))
                .item(cellularSettings())
            )
        ).match());

    // set signal strength to 39
    setNetworkRegistrationProperty(modem1, "Strength", QVariant::fromValue(uchar(39)));

    // check indicator is a 4-bar signal icon, a 4-bar signal icon and a 0-bar wifi icon.
    // check sim 2 status shows correct carrier name with 4-bar signal icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full", "gsm-3g-full", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-full"))
                .item(cellularSettings())
            )
        ).match());
}

TEST_F(TestIndicator, FlightMode_NoSIM)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // add and connect to 2-bar unsecure AP
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap = createAccessPoint("0", "the ssid", device, 40, Secure::insecure);
    auto connection = createAccessPointConnection("0", "the ssid", device);
    auto activeConnection = createActiveConnection("0", device, connection, ap);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // set no sim
    setSimManagerProperty(firstModem(), "Present", false);

    // check indicator is just a 2-bar wifi icon
    // check sim status shows “No SIM” with crossed sim card icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-50"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("the ssid",
                      Secure::insecure,
                      ApMode::infra,
                      ConnectionStatus::connected,
                      40)
                )
            )
        ).match());

    // set flight mode on
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)
                  .activate()
            )
        ).match());


    // Disconnect
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    removeActiveConnection(device, activeConnection);
    removeWifiConnection(device, connection);
    removeAccessPoint(device, ap);

    // check that the wifi switch turns off
    // check indicator is a plane icon and a 0-bar wifi icon
    // check sim status shows “No SIM” with crossed sim card icon (unchanged).
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"airplane-mode", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                  .is_empty()
            )
        ).match());

    // set flight mode off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true)
                  .activate()
            )
        ).match());

    // And we're back
    ap = createAccessPoint("1", "the ssid", device, 40, Secure::insecure);
    connection = createAccessPointConnection("1", "the ssid", device);
    activeConnection = createActiveConnection("1", device, connection, ap);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check that the wifi switch turns back on
    // check indicator is just a 2-bar wifi icon
    // check sim status shows “No SIM” with crossed sim card icon.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-50"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
        ).match());
}

TEST_F(TestIndicator, FlightMode_LockedSIM)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // add and connect to 1-bar secure AP
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap = createAccessPoint("0", "the ssid", device, 20, Secure::wpa);
    auto connection = createAccessPointConnection("0", "the ssid", device);
    auto activeConnection = createActiveConnection("0", device, connection, ap);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // set sim locked
    setSimManagerProperty(firstModem(), "PinRequired", "pin");

    // check indicator is a locked sim card and a 1-bar locked wifi icon
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"simcard-locked", "nm-signal-25-secure"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("the ssid",
                      Secure::wpa,
                      ApMode::infra,
                      ConnectionStatus::connected,
                      20)
                )
            )
        ).match());

    // set flight mode on
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)
                  .activate()
            )
        ).match());

    // Disconnect
    removeActiveConnection(device, activeConnection);
    removeWifiConnection(device, connection);
    removeAccessPoint(device, ap);
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // check that the wifi switch turns off
    // check indicator is a plane icon, a locked sim card and a 0-bar wifi icon
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath (unchanged).
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"airplane-mode", "simcard-locked", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());

    // set flight mode off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true)
                  .activate()
            )
        ).match());

    // And we're back
    ap = createAccessPoint("1", "the ssid", device, 20, Secure::wpa);
    connection = createAccessPointConnection("1", "the ssid", device);
    activeConnection = createActiveConnection("1", device, connection, ap);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check that the wifi switch turns back on
    // check indicator is a locked sim card and a 1-bar locked wifi icon
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"simcard-locked", "nm-signal-25-secure"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
        ).match());
}

TEST_F(TestIndicator, FlightMode_WifiOff)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // add some APs (secure / unsecure / adhoc / varied strength)
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap1 = createAccessPoint("1", "NSD", device, 0, Secure::wpa, ApMode::infra);
    auto ap2 = createAccessPoint("2", "JDR", device, 20, Secure::wpa, ApMode::adhoc);
    auto ap3 = createAccessPoint("3", "DGN", device, 40, Secure::wpa, ApMode::infra);
    auto ap4 = createAccessPoint("4", "JDY", device, 60, Secure::wpa, ApMode::adhoc);
    auto ap5 = createAccessPoint("5", "SCE", device, 20, Secure::insecure, ApMode::infra);
    auto ap6 = createAccessPoint("6", "ADS", device, 40, Secure::insecure, ApMode::adhoc);
    auto ap7 = createAccessPoint("7", "CFT", device, 60, Secure::insecure, ApMode::infra);
    auto ap8 = createAccessPoint("8", "GDF", device, 80, Secure::insecure, ApMode::adhoc);

    // connect to 2-bar secure AP
    auto connection = createAccessPointConnection("3", "DGN", device);
    auto active_connection = createActiveConnection("3", device, connection, ap3);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // set sim unlocked on carrier with 3-bar signal
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(26)));

    // check that the wifi switch is on
    // check indicator is a 3-bar signal icon and 2-bar locked wifi icon
    // check sim status shows correct carrier name with 3-bar signal icon.
    // check that AP list contains the connected AP at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-high", "nm-signal-50-secure"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("", "fake.tel", "gsm-3g-high"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::connected, 40))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 0))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 20))
            )
        ).match());

    // set wifi off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(false))
              .item(mh::MenuItemMatcher())
              .item(wifiEnableSwitch(true)
                  .activate()
              )
        ).match());

    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    removeActiveConnection(device, active_connection);
    removeWifiConnection(device, connection);
    removeAccessPoint(device, ap1);
    removeAccessPoint(device, ap2);
    removeAccessPoint(device, ap3);
    removeAccessPoint(device, ap4);
    removeAccessPoint(device, ap5);
    removeAccessPoint(device, ap6);
    removeAccessPoint(device, ap7);
    removeAccessPoint(device, ap8);

    // check that the flight mode switch is still off
    // check that the wifi switch is off
    // check indicator is a 3-bar signal icon and 0-bar wifi icon
    // check sim status shows correct carrier name with 3-bar signal icon.
    // check that AP list is empty
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-high", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("", "fake.tel", "gsm-3g-high"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());

    // set flight mode on
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)
                  .activate()
            )
        ).match());

    setModemProperty(firstModem(), "Online", false);

    // check indicator is a plane icon and a 0-bar wifi icon
    // check sim status shows “Offline” with 0-bar signal icon.
    // check that AP list is empty
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"airplane-mode", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("", "Offline", "gsm-3g-disabled"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());

    // set flight mode off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true)
                  .activate()
            )
        ).match());

    setModemProperty(firstModem(), "Online", true);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    setConnectionManagerProperty(firstModem(), "Bearer", "gprs");
    setConnectionManagerProperty(firstModem(), "Powered", true);

    // check that the wifi switch is still off
    // check indicator is a 3-bar signal icon and 0-bar wifi icon
    // check sim status shows correct carrier name with 3-bar signal icon.
    // check that AP list is empty
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-high", "network-cellular-pre-edge"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(true))
                .item(modemInfo("", "fake.tel", "gsm-3g-high", false, "network-cellular-pre-edge"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());
}

TEST_F(TestIndicator, FlightMode_WifiOn)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // add some APs (secure / unsecure / adhoc / varied strength)
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap1 = createAccessPoint("1", "NSD", device, 0, Secure::wpa, ApMode::infra);
    auto ap2 = createAccessPoint("2", "JDR", device, 20, Secure::wpa, ApMode::adhoc);
    auto ap3 = createAccessPoint("3", "DGN", device, 40, Secure::wpa, ApMode::infra);
    auto ap4 = createAccessPoint("4", "JDY", device, 60, Secure::wpa, ApMode::adhoc);
    auto ap5 = createAccessPoint("5", "SCE", device, 20, Secure::insecure, ApMode::infra);
    auto ap6 = createAccessPoint("6", "ADS", device, 40, Secure::insecure, ApMode::adhoc);
    auto ap7 = createAccessPoint("7", "CFT", device, 60, Secure::insecure, ApMode::infra);
    auto ap8 = createAccessPoint("8", "GDF", device, 80, Secure::insecure, ApMode::adhoc);

    // connect to 4-bar insecure AP
    auto connection = createAccessPointConnection("8", "GDF", device);
    auto active_connection = createActiveConnection("8", device, connection, ap8);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // set sim unlocked on carrier with 1-bar signal
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(6)));

    // check that the wifi switch is on
    // check indicator is a 1-bar signal icon and 4-bar wifi icon
    // check sim status shows correct carrier name with 1-bar signal icon.
    // check that AP list contains the connected AP at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-low", "nm-signal-100"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-low"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 80))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 0))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 20))
            )
        ).match());

    // set flight mode on
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)
                  .activate()
            )
        ).match());

    setModemProperty(firstModem(), "Online", false);
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    removeActiveConnection(device, active_connection);
    removeWifiConnection(device, connection);
    removeAccessPoint(device, ap1);
    removeAccessPoint(device, ap2);
    removeAccessPoint(device, ap3);
    removeAccessPoint(device, ap4);
    removeAccessPoint(device, ap5);
    removeAccessPoint(device, ap6);
    removeAccessPoint(device, ap7);
    removeAccessPoint(device, ap8);

    // check that the wifi switch turns off
    // check indicator is a plane icon and a 0-bar wifi icon
    // check sim status shows “Offline” with 0-bar signal icon.
    // check that AP list is empty
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"airplane-mode", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "Offline", "gsm-3g-disabled"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());


    // set wifi on
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(true))
              .item(mh::MenuItemMatcher())
              .item(wifiEnableSwitch(false)
                  .activate()
              )
        ).match());

    // NOTE: every newly created access point increments AP index (see: AccessPointItem::Private::ConstructL())
    //       so here we need to start at index 1+8 as we've had 8 APs previously.
    ap1 = createAccessPoint("9", "NSD", device, 0, Secure::wpa, ApMode::infra);
    ap2 = createAccessPoint("10", "JDR", device, 20, Secure::wpa, ApMode::adhoc);
    ap3 = createAccessPoint("11", "DGN", device, 40, Secure::wpa, ApMode::infra);
    ap4 = createAccessPoint("12", "JDY", device, 60, Secure::wpa, ApMode::adhoc);
    ap5 = createAccessPoint("13", "SCE", device, 20, Secure::insecure, ApMode::infra);
    ap6 = createAccessPoint("14", "ADS", device, 40, Secure::insecure, ApMode::adhoc);
    ap7 = createAccessPoint("15", "CFT", device, 60, Secure::insecure, ApMode::infra);
    ap8 = createAccessPoint("16", "GDF", device, 80, Secure::insecure, ApMode::adhoc);
    connection = createAccessPointConnection("16", "GDF", device);
    active_connection = createActiveConnection("16", device, connection, ap8);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check that the flight mode switch is still on
    // check that the wifi switch is on
    // check indicator is a plane icon and a 4-bar wifi icon
    // check sim status shows “Offline” with 0-bar signal icon.
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"airplane-mode", "nm-signal-100"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "Offline", "gsm-3g-disabled"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 80))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 0))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 20))
            )
        ).match());

    // set flight mode off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(true)
                  .activate()
            )
        ).match());

    setModemProperty(firstModem(), "Online", true);

    // check that the wifi switch remains on
    // check indicator is a 1-bar signal icon and 4-bar wifi icon
    // check sim status shows correct carrier name with 1-bar signal icon.
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
              .state_icons({"gsm-3g-low", "nm-signal-100"})
              .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(false))
              .item(mh::MenuItemMatcher()
                  .section()
                  .item(mobileDataSwitch())
                  .item(modemInfo("", "fake.tel", "gsm-3g-low"))
                  .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 80))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 0))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 20))
            )
        ).match());
}

TEST_F(TestIndicator, GroupedWiFiAccessPoints)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // create the wifi device
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // add a single AP
    auto ap1 = createAccessPoint("1", "groupA", device, 40, Secure::wpa, ApMode::infra);

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("groupA", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 40))
            )
        ).match());

    // add a second AP with the same SSID
    auto ap2 = createAccessPoint("2", "groupA", device, 60, Secure::wpa, ApMode::infra);

    // check that we see a single AP with the higher strength
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("groupA", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
            )
        ).match());

    // up the strength of the first AP
    setNmProperty(ap1, NM_DBUS_INTERFACE_ACCESS_POINT, "Strength", QVariant::fromValue(uchar(80)));

    // check that we see a single AP with the higher strength
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("groupA", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 80))
            )
        ).match());

    // add another AP with a different SSID
    auto ap3 = createAccessPoint("3", "groupB", device, 75, Secure::wpa, ApMode::infra);

    // check that we see a single AP with the higher strength
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("groupA", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 80))
                .item(accessPoint("groupB", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 75))
            )
        ).match());

    // remove the first access point
    removeAccessPoint(device, ap1);

    // verify the list has the old combined access point and the strength matches the second ap initial strength
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher())
            .item(wifiEnableSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(accessPoint("groupA", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("groupB", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 75))
            )
        ).match());
}

TEST_F(TestIndicator, WifiStates_SSIDs)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // add some APs (secure / unsecure / adhoc / varied strength)
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    // prepend a non-utf8 character to the end of AP 1's SSID
    auto ap1 = createAccessPoint("1", "NSD", device, 20, Secure::wpa, ApMode::infra);
    setNmProperty(ap1, NM_DBUS_INTERFACE_ACCESS_POINT, "Ssid", QByteArray(1, -1) + QByteArray("NSD"));

    // append a non-utf8 character to the end of AP 2's SSID
    auto ap2 = createAccessPoint("2", "DGN", device, 20, Secure::wpa, ApMode::infra);
    setNmProperty(ap2, NM_DBUS_INTERFACE_ACCESS_POINT, "Ssid", QByteArray("DGN") + QByteArray(1, -1));

    // insert a non-utf8 character into AP 3's SSID
    auto ap3 = createAccessPoint("3", "JDY", device, 20, Secure::wpa, ApMode::infra);
    setNmProperty(ap3, NM_DBUS_INTERFACE_ACCESS_POINT, "Ssid", QByteArray("JD") + QByteArray(1, -1) + QByteArray("Y"));

    // use only non-utf8 characters for AP 4's SSID
    auto ap4 = createAccessPoint("4", "---", device, 20, Secure::wpa, ApMode::infra);
    setNmProperty(ap4, NM_DBUS_INTERFACE_ACCESS_POINT, "Ssid", QByteArray(4, -1));

    // leave AP 5's SSID blank
    auto ap5 = createAccessPoint("5", "", device, 20, Secure::wpa, ApMode::infra);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is just a mobile data connection (we are not connected to WiFi)
    // check that AP list contains the 4 APs in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-full"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "fake.tel", "gsm-3g-full"))
                .item(cellularSettings())
            )
          .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("DGN�", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("JD�Y", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("�NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("����", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
            )
        ).match());
}

TEST_F(TestIndicator, WifiStates_Connect1AP)
{
    // create a wifi device
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // set wifi off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(false))
              .item(mh::MenuItemMatcher())
              .item(wifiEnableSwitch(true)
                  .activate()
              )
        ).match());

    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set no sim
    setSimManagerProperty(firstModem(), "Present", false);

    // check indicator is just a 0-bar wifi icon
    // check that AP list is empty
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());

    // set wifi switch on and add some APs (secure/unsecure/adhoc/varied strength)
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(false))
              .item(mh::MenuItemMatcher())
              .item(wifiEnableSwitch(false)
                  .activate()
              )
        ).match());

    auto ap1 = createAccessPoint("1", "NSD", device, 20, Secure::wpa, ApMode::infra);
    auto ap2 = createAccessPoint("2", "JDR", device, 40, Secure::wpa, ApMode::adhoc);
    auto ap3 = createAccessPoint("3", "DGN", device, 60, Secure::wpa, ApMode::infra);
    auto ap4 = createAccessPoint("4", "JDY", device, 80, Secure::wpa, ApMode::adhoc);
    auto ap5 = createAccessPoint("5", "SCE", device, 0, Secure::insecure, ApMode::infra);
    auto ap6 = createAccessPoint("6", "ADS", device, 20, Secure::insecure, ApMode::adhoc);
    auto ap7 = createAccessPoint("7", "CFT", device, 40, Secure::insecure, ApMode::infra);
    auto ap8 = createAccessPoint("8", "GDF", device, 60, Secure::insecure, ApMode::adhoc);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check indicator is empty (we aren't connected to WiFi)
    // check that AP list contains available APs in alphabetical order (with correct signal and security icons).
    // check AP items have the correct associated action names.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 0))
            )
        ).match());

    // connect to 1-bar unsecure AP
    setGlobalConnectedState(NM_STATE_CONNECTING);
    auto connection = createAccessPointConnection("6", "ADS", device);
    auto active_connection = createActiveConnection("6", device, connection, ap6);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check indicator is just a 1-bar wifi icon
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-25"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 0))
            )
        ).match());

    // set AP signal strength 0
    setNmProperty(ap6, NM_DBUS_INTERFACE_ACCESS_POINT, "Strength", QVariant::fromValue(uchar(0)));

    // check indicator is a 0-bar wifi icon.
    // check that AP signal icon also updates accordingly.
    auto ap_item = mh::MenuItemMatcher::checkbox();
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-00"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 0))
                .item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item)
            )
        ).match());

    // set AP signal strength 40
    setNmProperty(ap6, NM_DBUS_INTERFACE_ACCESS_POINT, "Strength", QVariant::fromValue(uchar(40)));

    // check indicator is a 2-bar wifi icon.
    // check that AP signal icon also updates accordingly.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-50"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 40))
                .item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item)
            )
        ).match());

    // set AP signal strength 60
    setNmProperty(ap6, NM_DBUS_INTERFACE_ACCESS_POINT, "Strength", QVariant::fromValue(uchar(60)));

    // check indicator is a 3-bar wifi icon.
    // check that AP signal icon also updates accordingly.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-75"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 60))
                .item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item)
            )
        ).match());

    // set AP signal strength 80
    setNmProperty(ap6, NM_DBUS_INTERFACE_ACCESS_POINT, "Strength", QVariant::fromValue(uchar(80)));

    // check indicator is a 4-bar wifi icon.
    // check that AP signal icon also updates accordingly.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-100"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::connected, 80))
                .item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item).item(ap_item)
            )
        ).match());

    // set wifi off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(false))
              .item(mh::MenuItemMatcher())
              .item(wifiEnableSwitch(true)
                  .activate()
              )
        ).match());

    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    removeActiveConnection(device, active_connection);
    removeWifiConnection(device, connection);
    removeAccessPoint(device, ap1);
    removeAccessPoint(device, ap2);
    removeAccessPoint(device, ap3);
    removeAccessPoint(device, ap4);
    removeAccessPoint(device, ap5);
    removeAccessPoint(device, ap6);
    removeAccessPoint(device, ap7);
    removeAccessPoint(device, ap8);

    // check indicator is just a 0-bar wifi icon
    // check that AP list is empty
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());
}

TEST_F(TestIndicator, WifiStates_Connect2APs)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // set no sim
    setSimManagerProperty(firstModem(), "Present", false);

    // add some APs (secure / unsecure / adhoc / varied strength)
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap1 = createAccessPoint("1", "NSD", device, 20, Secure::wpa, ApMode::infra);
    auto ap2 = createAccessPoint("2", "JDR", device, 40, Secure::wpa, ApMode::adhoc);
    auto ap3 = createAccessPoint("3", "DGN", device, 60, Secure::wpa, ApMode::infra);
    auto ap4 = createAccessPoint("4", "JDY", device, 80, Secure::wpa, ApMode::adhoc);
    auto ap5 = createAccessPoint("5", "SCE", device, 0, Secure::insecure, ApMode::infra);
    auto ap6 = createAccessPoint("6", "ADS", device, 20, Secure::insecure, ApMode::adhoc);
    auto ap7 = createAccessPoint("7", "CFT", device, 40, Secure::insecure, ApMode::infra);
    auto ap8 = createAccessPoint("8", "GDF", device, 60, Secure::insecure, ApMode::adhoc);

    // connect to 4-bar secure AP
    auto connection = createAccessPointConnection("4", "JDY", device);
    auto active_connection = createActiveConnection("4", device, connection, ap4);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is just a 4-bar locked wifi icon
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-100-secure"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
          .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::connected, 80))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 0))
            )
        ).match());

    // connect to 2-bar unsecure AP
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    removeActiveConnection(device, active_connection);
    removeWifiConnection(device, connection);
    setGlobalConnectedState(NM_STATE_CONNECTING);
    connection = createAccessPointConnection("7", "CFT", device);
    active_connection = createActiveConnection("7", device, connection, ap7);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check indicator is just a 2-bar wifi icon
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-50"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::connected, 40))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 0))
            )
        ).match());

    // set wifi off
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(false))
              .item(mh::MenuItemMatcher())
              .item(wifiEnableSwitch(true)
                  .activate()
              )
        ).match());

    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    removeActiveConnection(device, active_connection);
    removeWifiConnection(device, connection);
    removeAccessPoint(device, ap1);
    removeAccessPoint(device, ap2);
    removeAccessPoint(device, ap3);
    removeAccessPoint(device, ap4);
    removeAccessPoint(device, ap5);
    removeAccessPoint(device, ap6);
    removeAccessPoint(device, ap7);
    removeAccessPoint(device, ap8);

    // check indicator is just a 0-bar wifi icon
    // check that AP list is empty
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(false))
            .item(mh::MenuItemMatcher()
                .is_empty()
            )
        ).match());

    // set wifi on
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
              .item(flightModeSwitch(false))
              .item(mh::MenuItemMatcher())
              .item(wifiEnableSwitch(false)
                  .activate()
              )
        ).match());

    // NOTE: every newly created access point increments AP index (see: AccessPointItem::Private::ConstructL())
    //       so here we need to start at index 1+8 as we've had 8 APs previously.
    ap1 = createAccessPoint("9", "NSD", device, 20, Secure::wpa, ApMode::infra);
    ap2 = createAccessPoint("10", "JDR", device, 40, Secure::wpa, ApMode::adhoc);
    ap3 = createAccessPoint("11", "DGN", device, 60, Secure::wpa, ApMode::infra);
    ap4 = createAccessPoint("12", "JDY", device, 80, Secure::wpa, ApMode::adhoc);
    ap5 = createAccessPoint("13", "SCE", device, 0, Secure::insecure, ApMode::infra);
    ap6 = createAccessPoint("14", "ADS", device, 20, Secure::insecure, ApMode::adhoc);
    ap7 = createAccessPoint("15", "CFT", device, 40, Secure::insecure, ApMode::infra);
    ap8 = createAccessPoint("16", "GDF", device, 60, Secure::insecure, ApMode::adhoc);

    connection = createAccessPointConnection("12", "JDY", device);
    active_connection = createActiveConnection("12", device, connection, ap4);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check that the 4-bar secure AP is reconnected (as it has the highest signal).
    // check indicator is just a 4-bar locked wifi icon
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-100-secure"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false)).item(mh::MenuItemMatcher()).item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::connected, 80))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 20))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 0))
            )
        ).match());
}

TEST_F(TestIndicator, WifiStates_AddAndActivate)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set no sim
    setSimManagerProperty(firstModem(), "Present", false);

    // add some APs (secure / unsecure / adhoc / varied strength)
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap1 = createAccessPoint("1", "NSD", device, 40, Secure::wpa, ApMode::infra);
    auto ap2 = createAccessPoint("2", "JDR", device, 40, Secure::wpa, ApMode::adhoc);
    auto ap3 = createAccessPoint("3", "DGN", device, 60, Secure::wpa, ApMode::infra);
    auto ap4 = createAccessPoint("4", "JDY", device, 80, Secure::wpa, ApMode::adhoc);
    auto ap5 = createAccessPoint("5", "SCE", device, 20, Secure::insecure, ApMode::infra);
    auto ap6 = createAccessPoint("6", "ADS", device, 20, Secure::insecure, ApMode::adhoc);
    auto ap7 = createAccessPoint("7", "CFT", device, 40, Secure::insecure, ApMode::infra);
    auto ap8 = createAccessPoint("8", "GDF", device, 60, Secure::insecure, ApMode::adhoc);

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is just a 0-bar wifi icon
    // check that AP list contains the APs in alphabetical order.
    // activate the "SCE" AP (AddAndActivateConnection)
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
          .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 20)
                      .activate())
            )
        ).match());

    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check indicator is just a 1-bar wifi icon
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    // activate the "NSD" AP (AddAndActivateConnection)
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-25"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
          .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::connected, 20))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 40)
                      .activate())
            )
        ).match());

    setGlobalConnectedState(NM_STATE_CONNECTING);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check indicator is just a 2-bar locked wifi icon
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    // re-activate the "SCE" AP (ActivateConnection)
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-50-secure"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
          .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::connected, 40))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 20)
                      .activate())
            )
        ).match());

    setGlobalConnectedState(NM_STATE_CONNECTING);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // check indicator is just a 1-bar wifi icon
    // check that AP list contains the connected AP highlighted at top then other APs underneath in alphabetical order.
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-signal-25"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
          .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("SCE", Secure::insecure, ApMode::infra, ConnectionStatus::connected, 20))
                .item(accessPoint("ADS", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 20))
                .item(accessPoint("CFT", Secure::insecure, ApMode::infra, ConnectionStatus::disconnected, 40))
                .item(accessPoint("DGN", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 60))
                .item(accessPoint("GDF", Secure::insecure, ApMode::adhoc, ConnectionStatus::disconnected, 60))
                .item(accessPoint("JDR", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 40))
                .item(accessPoint("JDY", Secure::wpa, ApMode::adhoc, ConnectionStatus::disconnected, 80))
                .item(accessPoint("NSD", Secure::wpa, ApMode::infra, ConnectionStatus::disconnected, 40))
            )
        ).match());
}

TEST_F(TestIndicator, EnterpriseWifiConnect)
{
    // set wifi on, flight mode off
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set no sim
    setSimManagerProperty(firstModem(), "Present", false);

    // add some APs (secure / unsecure / adhoc / varied strength)
    auto device = createWiFiDevice(NM_DEVICE_STATE_ACTIVATED);
    auto ap1 = createAccessPoint("1", "ABC", device, 80, Secure::wpa_enterprise, ApMode::infra, "11:22:33:44:55:66");

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // interface to the URL dispatcher
    auto& urlDispatcher = dbusMock.mockInterface(
                        "com.canonical.URLDispatcher",
                        "/com/canonical/URLDispatcher",
                        "com.canonical.URLDispatcher",
                        QDBusConnection::SessionBus);

   // Wait for method calls on the URL dispatcher
   QSignalSpy urlDispatcherSpy(&urlDispatcher, SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch(false))
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "No SIM", "no-simcard"))
                .item(cellularSettings())
            )
          .item(wifiEnableSwitch(true))
          .item(mh::MenuItemMatcher()
               .item(accessPoint("ABC", Secure::wpa_enterprise, ApMode::infra, ConnectionStatus::disconnected, 80).activate())
           )
        ).match());

    if (urlDispatcherSpy.isEmpty())
    {
        ASSERT_TRUE(urlDispatcherSpy.wait());
    }

    ASSERT_FALSE(urlDispatcherSpy.isEmpty());
    EXPECT_EQ(urlDispatcherSpy.first(),
        QVariantList()
            << QVariant("DispatchURL")
            << QVariant(
                QVariantList()
                    << QVariant("settings:///system/wifi?ssid=ABC&bssid=11:22:33:44:55:66")
                    << QVariant("")
               )
    );
}

TEST_F(TestIndicator, CellDataEnabled)
{
    // We are connected
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // Create a WiFi device and power it off
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);
    disableWiFi();

    // sim in with carrier and 4-bar signal and HSPA
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(26)));
    setConnectionManagerProperty(firstModem(), "Bearer", "hspa");
    setModemProperty(firstModem(), "Online", true);
    setConnectionManagerProperty(firstModem(), "Powered", true);

    // second sim with umts (3G)
    auto secondModem = createModem("ril_1");
    setNetworkRegistrationProperty(secondModem, "Strength", QVariant::fromValue(uchar(6)));
    setConnectionManagerProperty(secondModem, "Bearer", "umts");
    setModemProperty(secondModem, "Online", true);
    setConnectionManagerProperty(secondModem, "Powered", false);

    // Set first SIM as the active data connection
    auto connectivity = newConnectivity();
    auto modems = getSortedModems(*connectivity);
    auto sim1 = getModemSim(*modems, 0);
    connectivity->setSimForMobileData(sim1);

    ASSERT_NO_THROW(startIndicator());

    // Should be connected to HSPA
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-high", "gsm-3g-low", "network-cellular-hspa"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(true))
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-high", false, "network-cellular-hspa"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-low"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
        ).match());

    // First SIM card now only has EDGE
    setConnectionManagerProperty(firstModem(), "Bearer", "edge");

    // Now we should have an EDGE icon
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
            .item(mh::MenuItemMatcher()
                .state_icons({"gsm-3g-high", "gsm-3g-low", "network-cellular-edge"})
                .mode(mh::MenuItemMatcher::Mode::starts_with)
                .item(flightModeSwitch())
                .item(mh::MenuItemMatcher()
                    .section()
                    .item(mobileDataSwitch(true))
                    .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-high", false, "network-cellular-edge"))
                    .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-low"))
                    .item(cellularSettings())
                )
                .item(wifiEnableSwitch(false))
            ).match());

    // Set second SIM as the active data connection
    auto connectivity = newConnectivity();
    auto modems = getSortedModems(*connectivity);
    auto sim2 = getModemSim(*modems, 1);
    connectivity->setSimForMobileData(sim2);

    // Now we should have a 3G icon
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
            .item(mh::MenuItemMatcher()
                .state_icons({"gsm-3g-high", "gsm-3g-low", "network-cellular-3g"})
                .mode(mh::MenuItemMatcher::Mode::starts_with)
                .item(flightModeSwitch())
                .item(mh::MenuItemMatcher()
                    .section()
                    .item(mobileDataSwitch(true))
                    .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-high"))
                    .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-low", false, "network-cellular-3g"))
                    .item(cellularSettings())
                )
                .item(wifiEnableSwitch(false))
            ).match());
}

TEST_F(TestIndicator, CellDataDisabled)
{
    // We are disconnected
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // Create a WiFi device and power it off
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);
    disableWiFi();

    // sim in with carrier and 1-bar signal and HSPA, data disabled
    setNetworkRegistrationProperty(firstModem(), "Strength", QVariant::fromValue(uchar(6)));
    setConnectionManagerProperty(firstModem(), "Bearer", "hspa");
    setModemProperty(firstModem(), "Online", true);
    setConnectionManagerProperty(firstModem(), "Powered", false);

    // second sim with 4-bar signal umts (3G), data disabled
    auto secondModem = createModem("ril_1");
    setNetworkRegistrationProperty(secondModem, "Strength", QVariant::fromValue(uchar(26)));
    setConnectionManagerProperty(secondModem, "Bearer", "umts");
    setModemProperty(secondModem, "Online", true);
    setConnectionManagerProperty(secondModem, "Powered", false);

    ASSERT_NO_THROW(startIndicator());

    // Should be totally disconnected
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-low", "gsm-3g-high", "nm-no-connection"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(false))
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-low"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-high"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
        ).match());

    // Set second SIM as the active data connection
    setGlobalConnectedState(NM_STATE_CONNECTING);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    setConnectionManagerProperty(firstModem(), "Powered", false);
    setConnectionManagerProperty(secondModem, "Powered", true);

    // Should be connected to 3G
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .state_icons({"gsm-3g-low", "gsm-3g-high", "network-cellular-3g"})
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch(true))
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-low"))
                .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-high", false, "network-cellular-3g"))
                .item(cellularSettings())
            )
            .item(wifiEnableSwitch(false))
        ).match());

    // Enable WiFi and connect to it
    enableWiFi();
    auto ap1 = createAccessPoint("1", "ABC", device, 20, Secure::wpa, ApMode::infra);
    auto connection = createAccessPointConnection("1", "ABC", device);
    setNmProperty(device, NM_DBUS_INTERFACE_DEVICE, "State", QVariant::fromValue(uint(NM_DEVICE_STATE_ACTIVATED)));
    auto active_connection = createActiveConnection("1", device, connection, ap1);
    setGlobalConnectedState(NM_STATE_CONNECTING);
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);

    // Should be connected to WiFi
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
       .item(mh::MenuItemMatcher()
           .state_icons({"gsm-3g-low", "gsm-3g-high", "nm-signal-25-secure"})
           .mode(mh::MenuItemMatcher::Mode::starts_with)
           .item(flightModeSwitch())
           .item(mh::MenuItemMatcher()
               .section()
               .item(mobileDataSwitch(true))
               .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-low"))
               .item(modemInfo("SIM 2", "fake.tel", "gsm-3g-high", false, "network-cellular-3g"))
               .item(cellularSettings())
            )
            .item(wifiEnableSwitch(true))
            .item(mh::MenuItemMatcher()
                .item(accessPoint("ABC",
                    Secure::wpa,
                    ApMode::infra,
                    ConnectionStatus::connected,
                    20)
                )
            )
        ).match());
}

TEST_F(TestIndicator, UnlockSIM_MenuContents)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim locked
    setSimManagerProperty(firstModem(), "PinRequired", "pin");

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                               SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a locked sim card and a 0-bar wifi icon.
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    // activate “Unlock SIM” action
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(cellularSettings())
            )
        ).match());

    // check that the "GetServerInformation" method was called
    // check method arguments are correct
    string busName;
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    {
        QVariantList const& call(notificationsSpy.at(0));
        EXPECT_EQ("GetServerInformation", call.at(0));
        QVariantList const& args(call.at(1).toList());
        ASSERT_EQ(0, args.size());
    }
    {
        QVariantList const& call(notificationsSpy.at(1));
        EXPECT_EQ("Notify", call.at(0));
        QVariantList const& args(call.at(1).toList());
        ASSERT_EQ(8, args.size());
        EXPECT_EQ("indicator-network", args.at(0));
        EXPECT_EQ(0, args.at(1));
        EXPECT_EQ("", args.at(2));
        EXPECT_EQ("Enter SIM PIN", args.at(3));
        EXPECT_EQ("3 attempts remaining", args.at(4));
        EXPECT_EQ(QStringList(), args.at(5));
        EXPECT_EQ(-1, args.at(7));

        QVariantMap hints;
        ASSERT_TRUE(qDBusArgumentToMap(args.at(6), hints));
        ASSERT_EQ(3, hints.size());
        ASSERT_TRUE(hints.contains("x-canonical-private-menu-model"));
        ASSERT_TRUE(hints.contains("x-canonical-snap-decisions"));
        ASSERT_TRUE(hints.contains("x-canonical-snap-decisions-timeout"));
        EXPECT_EQ(true, hints["x-canonical-snap-decisions"]);
        EXPECT_EQ(numeric_limits<int32_t>::max(), hints["x-canonical-snap-decisions-timeout"]);

        QVariantMap menuInfo;
        ASSERT_TRUE(qDBusArgumentToMap(hints["x-canonical-private-menu-model"], menuInfo));
        ASSERT_EQ(3, menuInfo.size());
        ASSERT_TRUE(menuInfo.contains("actions"));
        ASSERT_TRUE(menuInfo.contains("busName"));
        ASSERT_TRUE(menuInfo.contains("menuPath"));
        busName = menuInfo["busName"].toString().toStdString();
        EXPECT_EQ("/com/canonical/indicator/network/unlocksim0", menuInfo["menuPath"]);

        QVariantMap actions;
        ASSERT_TRUE(qDBusArgumentToMap(menuInfo["actions"], actions));
        ASSERT_EQ(1, actions.size());
        ASSERT_TRUE(actions.contains("notifications"));
        EXPECT_EQ("/com/canonical/indicator/network/unlocksim0", actions["notifications"]);
    }
    notificationsSpy.clear();

    ASSERT_FALSE(busName.empty());
    // check contents of x-canonical-private-menu-model
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .action("notifications.simunlock")
            .string_attribute("x-canonical-type", "com.canonical.snapdecision.pinlock")
            .string_attribute("x-canonical-pin-min-max", "notifications.pinMinMax")
            .string_attribute("x-canonical-pin-popup", "notifications.popup")
            .string_attribute("x-canonical-pin-error", "notifications.error")
        ).match());
}

TEST_F(TestIndicator, UnlockSIM_Cancel)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim locked
    setSimManagerProperty(firstModem(), "PinRequired", "pin");

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                               SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a locked sim card and a 0-bar wifi icon.
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    // activate “Unlock SIM” action
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(cellularSettings())
            )
        ).match());

    // check that the "GetServerInformation" method was called
    // check method arguments are correct
    std::string busName;
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "GetServerInformation", /* no_args */);
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 0}, {3, "Enter SIM PIN"}, {4, "3 attempts remaining"});
    {
        QVariantList const& call(notificationsSpy.at(1));
        QVariantList const& args(call.at(1).toList());
        QVariantMap hints;
        QVariantMap menuInfo;
        ASSERT_TRUE(qDBusArgumentToMap(args.at(6), hints));
        ASSERT_TRUE(qDBusArgumentToMap(hints["x-canonical-private-menu-model"], menuInfo));
        busName = menuInfo["busName"].toString().toStdString();
    }
    notificationsSpy.clear();

    // cancel the notification
    QSignalSpy notificationClosedSpy(&dbusMock.notificationDaemonInterface(),
                                     SIGNAL(NotificationClosed(uint, uint)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .action("notifications.simunlock")
            .string_attribute("x-canonical-type", "com.canonical.snapdecision.pinlock")
            .string_attribute("x-canonical-pin-min-max", "notifications.pinMinMax")
            .string_attribute("x-canonical-pin-popup", "notifications.popup")
            .string_attribute("x-canonical-pin-error", "notifications.error")
            .activate(shared_ptr<GVariant>(g_variant_new_boolean(false), &mh::gvariant_deleter))
        ).match());

    // check that the "NotificationClosed" signal was emitted
    WAIT_FOR_SIGNALS(notificationClosedSpy, 1);
    EXPECT_EQ(notificationClosedSpy.first(), QVariantList() << QVariant(1) << QVariant(1));
    notificationClosedSpy.clear();

    // check that the "CloseNotification" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "CloseNotification", {0, "1"});
    notificationsSpy.clear();

    // re-activate “Unlock SIM” action
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(cellularSettings())
            )
        ).match());

    // check that the "Notify" method was called
    // check method arguments are correct (we re-use the same notification and reopen it)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1});
    notificationsSpy.clear();

    // cancel the notification again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .action("notifications.simunlock")
            .string_attribute("x-canonical-type", "com.canonical.snapdecision.pinlock")
            .string_attribute("x-canonical-pin-min-max", "notifications.pinMinMax")
            .string_attribute("x-canonical-pin-popup", "notifications.popup")
            .string_attribute("x-canonical-pin-error", "notifications.error")
            .activate(shared_ptr<GVariant>(g_variant_new_boolean(false), &mh::gvariant_deleter))
        ).match());

    // check that the "NotificationClosed" signal was emitted (new notification index should be 1)
    WAIT_FOR_SIGNALS(notificationClosedSpy, 1);
    EXPECT_EQ(notificationClosedSpy.first(), QVariantList() << QVariant(1) << QVariant(1));
    notificationClosedSpy.clear();

    // check that the "CloseNotification" method was called
    // check method arguments are correct (still using the same notification: 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "CloseNotification", {0, "1"});
    notificationsSpy.clear();
}

TEST_F(TestIndicator, UnlockSIM_CancelFirstUnlockSecond)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim locked
    setSimManagerProperty(firstModem(), "PinRequired", "pin");

    // Create a second locked modem
    auto secondModem = createModem("ril_1");
    setSimManagerProperty(secondModem, "PinRequired", "pin");

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                               SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    QSignalSpy secondModemMockInterfaceSpy(&modemMockInterface(secondModem),
                               SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a locked sim card and a 0-bar wifi icon.
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    // unlock first SIM
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(modemInfo("SIM 2", "SIM Locked", "simcard-locked", true))
                .item(cellularSettings())
            )
        ).match());

    // check that the "GetServerInformation" method was called
    // check method arguments are correct
    std::string busName;
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "GetServerInformation", /* no_args */);
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 0}, {3, "Enter SIM 1 PIN"}, {4, "3 attempts remaining"});
    {
        QVariantList const& call(notificationsSpy.at(1));
        QVariantList const& args(call.at(1).toList());
        QVariantMap hints;
        QVariantMap menuInfo;
        ASSERT_TRUE(qDBusArgumentToMap(args.at(6), hints));
        ASSERT_TRUE(qDBusArgumentToMap(hints["x-canonical-private-menu-model"], menuInfo));
        busName = menuInfo["busName"].toString().toStdString();
    }
    notificationsSpy.clear();

    // cancel the notification
    QSignalSpy notificationClosedSpy(&dbusMock.notificationDaemonInterface(),
                                     SIGNAL(NotificationClosed(uint, uint)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .action("notifications.simunlock")
            .string_attribute("x-canonical-type", "com.canonical.snapdecision.pinlock")
            .string_attribute("x-canonical-pin-min-max", "notifications.pinMinMax")
            .string_attribute("x-canonical-pin-popup", "notifications.popup")
            .string_attribute("x-canonical-pin-error", "notifications.error")
            .activate(shared_ptr<GVariant>(g_variant_new_boolean(false), &mh::gvariant_deleter))
        ).match());

    // check that the "NotificationClosed" signal was emitted
    WAIT_FOR_SIGNALS(notificationClosedSpy, 1);
    EXPECT_EQ(notificationClosedSpy.first(), QVariantList() << QVariant(1) << QVariant(1));
    notificationClosedSpy.clear();

    // check that the "CloseNotification" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "CloseNotification", {0, "1"});
    notificationsSpy.clear();

    // Activate  the second SIM unlock
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "SIM Locked", "simcard-locked", true))
                .item(modemInfo("SIM 2", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(cellularSettings())
            )
        ).match());

    // check that the "Notify" method was called
    // check method arguments are correct (we re-use the same notification and reopen it)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter SIM 2 PIN"});
    notificationsSpy.clear();

    secondModemMockInterfaceSpy.clear();

    // enter the PIN
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .action("notifications.simunlock")
            .string_attribute("x-canonical-type", "com.canonical.snapdecision.pinlock")
            .string_attribute("x-canonical-pin-min-max", "notifications.pinMinMax")
            .string_attribute("x-canonical-pin-popup", "notifications.popup")
            .string_attribute("x-canonical-pin-error", "notifications.error")
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("1234"), &mh::gvariant_deleter))
        ).match());

    // Check the PIN was sent to Ofono
    WAIT_FOR_SIGNALS(secondModemMockInterfaceSpy, 1);
    CHECK_METHOD_CALL(secondModemMockInterfaceSpy, 0, "EnterPin", {0, "pin"}, {1, "1234"});

    // check that the "NotificationClosed" signal was emitted (new notification index should be 1)
    WAIT_FOR_SIGNALS(notificationClosedSpy, 1);
    EXPECT_EQ(notificationClosedSpy.first(), QVariantList() << QVariant(1) << QVariant(1));
    notificationClosedSpy.clear();

    // check that the "CloseNotification" method was called
    // check method arguments are correct (still using the same notification: 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "CloseNotification", {0, "1"});
    notificationsSpy.clear();
}

TEST_F(TestIndicator, UnlockSIM_CorrectPin)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim locked
    setSimManagerProperty(firstModem(), "PinRequired", "pin");

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                               SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a locked sim card and a 0-bar wifi icon.
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    // activate “Unlock SIM” action
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(cellularSettings())
            )
        ).match());

    // check that the "GetServerInformation" method was called
    // check that the "Notify" method was called twice
    // check method arguments are correct
    std::string busName;
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "GetServerInformation", /* no_args */);
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 0}, {3, "Enter SIM PIN"}, {4, "3 attempts remaining"});
    {
        QVariantList const& call(notificationsSpy.at(1));
        QVariantList const& args(call.at(1).toList());
        QVariantMap hints;
        QVariantMap menuInfo;
        ASSERT_TRUE(qDBusArgumentToMap(args.at(6), hints));
        ASSERT_TRUE(qDBusArgumentToMap(hints["x-canonical-private-menu-model"], menuInfo));
        busName = menuInfo["busName"].toString().toStdString();
    }
    notificationsSpy.clear();

    // enter correct pin
    QSignalSpy notificationClosedSpy(&dbusMock.notificationDaemonInterface(),
                                     SIGNAL(NotificationClosed(uint, uint)));

    QSignalSpy modemSpy(&modemMockInterface(firstModem()),
                        SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .action("notifications.simunlock")
            .string_attribute("x-canonical-type", "com.canonical.snapdecision.pinlock")
            .string_attribute("x-canonical-pin-min-max", "notifications.pinMinMax")
            .string_attribute("x-canonical-pin-popup", "notifications.popup")
            .string_attribute("x-canonical-pin-error", "notifications.error")
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("1234"), &mh::gvariant_deleter))
        ).match());

    // check that the "EnterPin" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "EnterPin", {0, "pin"}, {1, "1234"});
    modemSpy.clear();

    // check that the "NotificationClosed" signal was emitted
    WAIT_FOR_SIGNALS(notificationClosedSpy, 1);
    EXPECT_EQ(notificationClosedSpy.first(), QVariantList() << QVariant(1) << QVariant(1));
    notificationClosedSpy.clear();

    // check that the "CloseNotification" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "CloseNotification", {0, "1"});
    notificationsSpy.clear();
}

TEST_F(TestIndicator, UnlockSIM_IncorrectPin)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim locked
    setSimManagerProperty(firstModem(), "PinRequired", "pin");

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                                SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a locked sim card and a 0-bar wifi icon.
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    // activate “Unlock SIM” action
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(cellularSettings())
            )
        ).match());

    // check that the "GetServerInformation" method was called
    // check that the "Notify" method was called twice
    // check method arguments are correct
    std::string busName;
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "GetServerInformation", /* no_args */);
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 0}, {3, "Enter SIM PIN"}, {4, "3 attempts remaining"});
    {
        QVariantList const& call(notificationsSpy.at(1));
        QVariantList const& args(call.at(1).toList());
        QVariantMap hints;
        QVariantMap menuInfo;
        ASSERT_TRUE(qDBusArgumentToMap(args.at(6), hints));
        ASSERT_TRUE(qDBusArgumentToMap(hints["x-canonical-private-menu-model"], menuInfo));
        busName = menuInfo["busName"].toString().toStdString();
    }
    notificationsSpy.clear();

    // enter incorrect pin
    // check that the notification is displaying no error message
    QSignalSpy modemSpy(&modemMockInterface(firstModem()),
                        SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "EnterPin" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "EnterPin", {0, "pin"}, {1, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter SIM PIN"}, {4, "2 attempts remaining"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter SIM PIN"}, {4, "2 attempts remaining"});
    notificationsSpy.clear();

    // check that the notification is displaying the appropriate error message
    // close the error message
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PIN")
            .pass_through_activate("x-canonical-pin-error")
        ).match());

    // check that the error message is no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
        ).match());

    // enter incorrect pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "EnterPin" method was called
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "EnterPin", {0, "pin"}, {1, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter SIM PIN"}, {4, "1 attempt remaining"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter SIM PIN"}, {4, "1 attempt remaining"});
    notificationsSpy.clear();

    // check that the error message and last attempt popup are displayed
    // close the error and popup
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PIN")
            .pass_through_string_attribute("x-canonical-pin-popup",
                "Sorry, incorrect SIM PIN. This will be your last attempt. "
                "If SIM PIN is entered incorrectly you will require your PUK code to unlock.")
            .pass_through_activate("x-canonical-pin-error")
            .pass_through_activate("x-canonical-pin-popup")
        ).match());

    // check that the error and popup are no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
            .pass_through_string_attribute("x-canonical-pin-popup", "")
        ).match());

    // enter incorrect pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "EnterPin" method was called
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "EnterPin", {0, "pin"}, {1, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter SIM PIN"}, {4, "0 attempts remaining"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter SIM PIN"}, {4, "0 attempts remaining"});
    notificationsSpy.clear();

    // set sim blocked
    setSimManagerProperty(firstModem(), "PinRequired", "puk");

    // clear the "SetProperty" method call
    WAIT_FOR_SIGNALS(modemSpy, 1);
    modemSpy.clear();

    // check that the "Notify" method was called
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter PUK code"}, {4, "10 attempts remaining"});
    notificationsSpy.clear();

    // check that the error message and last attempt popup are displayed
    // close the error and popup
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PIN")
            .pass_through_string_attribute("x-canonical-pin-popup",
                "Sorry, your SIM is now blocked. Please enter your PUK code to unblock SIM card. "
                "You may need to contact your network provider for PUK code.")
            .pass_through_activate("x-canonical-pin-error")
            .pass_through_activate("x-canonical-pin-popup")
        ).match());

    // check that the error and popup are no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
            .pass_through_string_attribute("x-canonical-pin-popup", "")
        ).match());

    // enter incorrect puk
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("87654321"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter new SIM PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "ResetPin" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "ResetPin", {0, "puk"}, {1, "87654321"}, {2, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM PIN"}, {4, "Create new PIN"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter PUK code"}, {4, "9 attempts remaining"});
    notificationsSpy.clear();

    // check that the notification is displaying the appropriate error message
    // close the error message
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PUK")
            .pass_through_activate("x-canonical-pin-error")
        ).match());

    // check that the error message is no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
        ).match());

    // enter correct puk
    QSignalSpy notificationClosedSpy(&dbusMock.notificationDaemonInterface(),
                                     SIGNAL(NotificationClosed(uint, uint)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("12345678"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter new SIM PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "ResetPin" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "ResetPin", {0, "puk"}, {1, "12345678"}, {2, "4321"});
    modemSpy.clear();

    // check that the "NotificationClosed" signal was emitted
    WAIT_FOR_SIGNALS(notificationClosedSpy, 1);
    EXPECT_EQ(notificationClosedSpy.first(), QVariantList() << QVariant(1) << QVariant(1));
    notificationClosedSpy.clear();

    // check that the "Notify" method was called twice when retries changes
    // check that the "CloseNotification" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM PIN"}, {4, "Create new PIN"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "CloseNotification", {0, "1"});
    notificationsSpy.clear();
}

TEST_F(TestIndicator, UnlockSIM2_IncorrectPin)
{
    // set flight mode off, wifi off, and cell data off, and sim in
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // set sim locked
    auto secondModem = createModem("ril_1");
    setSimManagerProperty(secondModem, "PinRequired", "pin");

    QSignalSpy notificationsSpy(&notificationsMockInterface(),
                                SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    // start the indicator
    ASSERT_NO_THROW(startIndicator());

    // check indicator is a locked sim card and a 0-bar wifi icon.
    // check sim status shows “SIM Locked”, with locked sim card icon and a “Unlock SIM” button beneath.
    // check that the “Unlock SIM” button has the correct action name.
    // activate “Unlock SIM” action
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .item(flightModeSwitch())
            .item(mh::MenuItemMatcher()
                .section()
                .item(mobileDataSwitch())
                .item(modemInfo("SIM 1", "fake.tel", "gsm-3g-full"))
                .item(modemInfo("SIM 2", "SIM Locked", "simcard-locked", true)
                      .pass_through_activate("x-canonical-modem-locked-action")
                )
                .item(cellularSettings())
            )
        ).match());

    // check that the "GetServerInformation" method was called
    // check that the "Notify" method was called twice
    // check method arguments are correct
    std::string busName;
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "GetServerInformation", /* no_args */);
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 0}, {3, "Enter SIM 2 PIN"}, {4, "3 attempts remaining"});
    {
        QVariantList const& call(notificationsSpy.at(1));
        QVariantList const& args(call.at(1).toList());
        QVariantMap hints;
        QVariantMap menuInfo;
        ASSERT_TRUE(qDBusArgumentToMap(args.at(6), hints));
        ASSERT_TRUE(qDBusArgumentToMap(hints["x-canonical-private-menu-model"], menuInfo));
        busName = menuInfo["busName"].toString().toStdString();
    }
    notificationsSpy.clear();

    // enter incorrect pin
    // check that the notification is displaying no error message
    QSignalSpy modemSpy(&modemMockInterface(secondModem),
                        SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "EnterPin" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "EnterPin", {0, "pin"}, {1, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter SIM 2 PIN"}, {4, "2 attempts remaining"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter SIM 2 PIN"}, {4, "2 attempts remaining"});
    notificationsSpy.clear();

    // check that the notification is displaying the appropriate error message
    // close the error message
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PIN")
            .pass_through_activate("x-canonical-pin-error")
        ).match());

    // check that the error message is no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
        ).match());

    // enter incorrect pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "EnterPin" method was called
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "EnterPin", {0, "pin"}, {1, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter SIM 2 PIN"}, {4, "1 attempt remaining"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter SIM 2 PIN"}, {4, "1 attempt remaining"});
    notificationsSpy.clear();

    // check that the error message and last attempt popup are displayed
    // close the error and popup
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PIN")
            .pass_through_string_attribute("x-canonical-pin-popup",
                "Sorry, incorrect SIM 2 PIN. This will be your last attempt. "
                "If SIM 2 PIN is entered incorrectly you will require your PUK code to unlock.")
            .pass_through_activate("x-canonical-pin-error")
            .pass_through_activate("x-canonical-pin-popup")
        ).match());

    // check that the error and popup are no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
            .pass_through_string_attribute("x-canonical-pin-popup", "")
        ).match());

    // enter incorrect pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "EnterPin" method was called
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "EnterPin", {0, "pin"}, {1, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter SIM 2 PIN"}, {4, "0 attempts remaining"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter SIM 2 PIN"}, {4, "0 attempts remaining"});
    notificationsSpy.clear();

    // set sim blocked
    setSimManagerProperty(secondModem, "PinRequired", "puk");

    // clear the "SetProperty" method call
    WAIT_FOR_SIGNALS(modemSpy, 1);
    modemSpy.clear();

    // check that the "Notify" method was called
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter PUK code for SIM 2"}, {4, "10 attempts remaining"});
    notificationsSpy.clear();

    // check that the error message and last attempt popup are displayed
    // close the error and popup
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PIN")
            .pass_through_string_attribute("x-canonical-pin-popup",
                "Sorry, your SIM 2 is now blocked. Please enter your PUK code to unblock SIM card. "
                "You may need to contact your network provider for PUK code.")
            .pass_through_activate("x-canonical-pin-error")
            .pass_through_activate("x-canonical-pin-popup")
        ).match());

    // check that the error and popup are no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
            .pass_through_string_attribute("x-canonical-pin-popup", "")
        ).match());

    // enter incorrect puk
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("87654321"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter new SIM 2 PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM 2 PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "ResetPin" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "ResetPin", {0, "puk"}, {1, "87654321"}, {2, "4321"});
    modemSpy.clear();

    // check that the "Notify" method was called when retries changes, then again for incorrect pin
    // check method arguments are correct (notification index should still be 1)
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM 2 PIN"}, {4, "Create new PIN"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "Notify", {1, 1}, {3, "Enter PUK code for SIM 2"}, {4, "9 attempts remaining"});
    notificationsSpy.clear();

    // check that the notification is displaying the appropriate error message
    // close the error message
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "Sorry, incorrect PUK")
            .pass_through_activate("x-canonical-pin-error")
        ).match());

    // check that the error message is no longer displayed
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .pass_through_string_attribute("x-canonical-pin-error", "")
        ).match());

    // enter correct puk
    QSignalSpy notificationClosedSpy(&dbusMock.notificationDaemonInterface(),
                                     SIGNAL(NotificationClosed(uint, uint)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("12345678"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Enter new SIM 2 PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "Notify" method was called
    WAIT_FOR_SIGNALS(notificationsSpy, 1);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM 2 PIN"}, {4, "Create new PIN"});
    notificationsSpy.clear();

    // enter new pin again
    EXPECT_MATCHRESULT(mh::MenuMatcher(unlockSimParameters(busName, 0))
        .item(mh::MenuItemMatcher()
            .set_action_state(shared_ptr<GVariant>(g_variant_new_string("4321"), &mh::gvariant_deleter))
        ).match());

    // check that the "ResetPin" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(modemSpy, 1);
    CHECK_METHOD_CALL(modemSpy, 0, "ResetPin", {0, "puk"}, {1, "12345678"}, {2, "4321"});
    modemSpy.clear();

    // check that the "NotificationClosed" signal was emitted
    WAIT_FOR_SIGNALS(notificationClosedSpy, 1);
    EXPECT_EQ(notificationClosedSpy.first(), QVariantList() << QVariant(1) << QVariant(1));
    notificationClosedSpy.clear();

    // check that the "Notify" method was called twice when retries changes
    // check that the "CloseNotification" method was called
    // check method arguments are correct
    WAIT_FOR_SIGNALS(notificationsSpy, 2);
    CHECK_METHOD_CALL(notificationsSpy, 0, "Notify", {1, 1}, {3, "Confirm new SIM 2 PIN"}, {4, "Create new PIN"});
    CHECK_METHOD_CALL(notificationsSpy, 1, "CloseNotification", {0, "1"});
    notificationsSpy.clear();
}

TEST_F(TestIndicator, CellularData_1)
{
    auto con = newConnectivity();

    ASSERT_NO_THROW(startIndicator());


    auto modems = con->modems();
    QSignalSpy rowsInsertedSpy(modems, SIGNAL(rowsInserted(const QModelIndex &, int, int)));
    if (modems->rowCount() == 0) {
        ASSERT_TRUE(rowsInsertedSpy.wait());
    }

    ASSERT_EQ(1, modems->rowCount());

    // Check that the indicator switch is enabled when we are not in flightmode
    // and there is a SIM for mobile data set.

    con->setMobileDataEnabled(true);
    con->setFlightMode(false);
    con->setSimForMobileData(getModemSim(*modems, 0));

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
              .item(flightModeSwitch())
              .item(mh::MenuItemMatcher()
                  .mode(mh::MenuItemMatcher::Mode::starts_with)
                  .section()
                  .item(mobileDataSwitch(true)
                    .enabled(true)
                  )
            )
        ).match());

    // Check that the indicator switch is disabled when we are in flightmode
    con->setFlightMode(true);
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
              .item(flightModeSwitch())
              .item(mh::MenuItemMatcher()
                  .mode(mh::MenuItemMatcher::Mode::starts_with)
                  .section()
                  .item(mobileDataSwitch(true)
                    .enabled(false)
                  )
            )
        ).match());

    // Check that the indicator switch is disabled when there is no SIM for mobile data set.
    con->setFlightMode(false);
    con->setSimForMobileData(nullptr);
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
              .item(flightModeSwitch())
              .item(mh::MenuItemMatcher()
                  .mode(mh::MenuItemMatcher::Mode::starts_with)
                  .section()
                  .item(mobileDataSwitch(true)
                    .enabled(false)
                  )
            )
        ).match());
}

TEST_F(TestIndicator, CellularData_2)
{
    auto con = newConnectivity();

    ASSERT_NO_THROW(startIndicator());

    auto modems = con->modems();
    QSignalSpy rowsInsertedSpy(modems, SIGNAL(rowsInserted(const QModelIndex &, int, int)));
    if (modems->rowCount() == 0) {
        ASSERT_TRUE(rowsInsertedSpy.wait());
    }

    ASSERT_EQ(1, modems->rowCount());

    // Check that Connectivity::mobileDataEnabled follows the indicator switch

    con->setMobileDataEnabled(true);
    con->setFlightMode(false);
    con->setSimForMobileData(getModemSim(*modems, 0));
    QTest::qWait(250);
    QSignalSpy spy(con.get(), SIGNAL(mobileDataEnabledUpdated(bool)));

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
              .item(flightModeSwitch())
              .item(mh::MenuItemMatcher()
                  .mode(mh::MenuItemMatcher::Mode::starts_with)
                  .section()
                  .item(mobileDataSwitch(true)
                    .activate()
                  )
            )
        ).match());

    WAIT_FOR_SIGNALS(spy, 1);
    EXPECT_EQ(spy[0], QVariantList() << QVariant(false));
    spy.clear();

    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
              .item(flightModeSwitch())
              .item(mh::MenuItemMatcher()
                  .mode(mh::MenuItemMatcher::Mode::starts_with)
                  .section()
                  .item(mobileDataSwitch(false)
                    .activate()
                  )
            )
        ).match());

    WAIT_FOR_SIGNALS(spy, 1);
    EXPECT_EQ(spy[0], QVariantList() << QVariant(true));


    // Check that indicator switch follows the Connectivity::mobileDataEnabled

    con->setMobileDataEnabled(true);
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
              .item(flightModeSwitch())
              .item(mh::MenuItemMatcher()
                  .mode(mh::MenuItemMatcher::Mode::starts_with)
                  .section()
                  .item(mobileDataSwitch(true))
            )
        ).match());

    con->setMobileDataEnabled(false);
    EXPECT_MATCHRESULT(mh::MenuMatcher(phoneParameters())
        .item(mh::MenuItemMatcher()
            .mode(mh::MenuItemMatcher::Mode::starts_with)
            .submenu()
              .item(flightModeSwitch())
              .item(mh::MenuItemMatcher()
                  .mode(mh::MenuItemMatcher::Mode::starts_with)
                  .section()
                  .item(mobileDataSwitch(false))
            )
        ).match());
}

} // namespace
