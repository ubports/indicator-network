
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
#include <dbus-types.h>
#include <NetworkManagerSettingsInterface.h>

#include <QDebug>
#include <QTestEventLoop>

using namespace std;
using namespace testing;
using namespace connectivityqt;

namespace
{

class TestConnectivityApi: public IndicatorNetworkTestBase
{
protected:
    static void SetUpTestCase()
    {
        Connectivity::registerMetaTypes();
    }
};

TEST_F(TestConnectivityApi, OnlineStatus)
{
    // Set up disconnected
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // Connect to the service
    auto connectivity(newConnectivity());
    QSignalSpy spy(connectivity.get(), &Connectivity::statusUpdated);

    // Check we are connected
    EXPECT_EQ(Connectivity::Status::Offline, connectivity->status());

    // Now we are connecting
    spy.clear();
    setGlobalConnectedState(NM_STATE_CONNECTING);
    WAIT_FOR_SIGNALS(spy, 1);
    EXPECT_EQ(Connectivity::Status::Connecting, connectivity->status());

    // Now we are connecting
    spy.clear();
    setGlobalConnectedState(NM_STATE_CONNECTED_GLOBAL);
    WAIT_FOR_SIGNALS(spy, 1);
    EXPECT_EQ(Connectivity::Status::Online, connectivity->status());
}

TEST_F(TestConnectivityApi, FollowsFlightMode)
{
    // Set up disconnected with flight mode on
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    ASSERT_TRUE(dbusMock.urfkillInterface().FlightMode(true));

    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // Connect to the service
    auto connectivity(newConnectivity());

    // Check flight mode is enabled
    EXPECT_TRUE(connectivity->flightMode());

    // Now disable flight mode
    ASSERT_TRUE(dbusMock.urfkillInterface().FlightMode(false));

    // Check that flight mode gets updated
    {
        QSignalSpy spy(connectivity.get(), SIGNAL(flightModeUpdated(bool)));
        ASSERT_TRUE(spy.wait());
        ASSERT_EQ(1, spy.size());
    }
    // Check that flight mode is disabled
    EXPECT_FALSE(connectivity->flightMode());
}

TEST_F(TestConnectivityApi, FlightModeTalksToURfkill)
{
    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // Connect to the UrfKill mock
    auto& urfkillInterface = dbusMock.urfkillInterface();
    QSignalSpy urfkillSpy(&urfkillInterface, SIGNAL(FlightModeChanged(bool)));

    // Connect the the service
    auto connectivity(newConnectivity());

    // Follow the flightMode property
    QSignalSpy flightModeSpy(connectivity.get(), SIGNAL(flightModeUpdated(bool)));

    // Follow the switch enabled properties
    QSignalSpy flightModeSwitchSpy(connectivity.get(), SIGNAL(flightModeSwitchEnabledUpdated(bool)));
    QSignalSpy wifiSwitchSpy(connectivity.get(), SIGNAL(wifiSwitchEnabledUpdated(bool)));
    QSignalSpy hotspotSwitchSpy(connectivity.get(), SIGNAL(hotspotSwitchEnabledUpdated(bool)));

    // Check that nothing is happening yet
    EXPECT_TRUE(connectivity->flightModeSwitchEnabled());
    EXPECT_TRUE(connectivity->wifiSwitchEnabled());
    EXPECT_TRUE(connectivity->hotspotSwitchEnabled());

    // Enable flight mode
    connectivity->setFlightMode(true);

    // We should first get the switch disabled change
    WAIT_FOR_SIGNALS(flightModeSwitchSpy, 1);
    EXPECT_EQ(flightModeSwitchSpy.first(), QVariantList() << QVariant(false));
    if (wifiSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(1, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.first(), QVariantList() << QVariant(false));
    if (hotspotSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(hotspotSwitchSpy.wait());
    }
    ASSERT_EQ(1, hotspotSwitchSpy.size());
    EXPECT_EQ(hotspotSwitchSpy.first(), QVariantList() << QVariant(false));

    // Wait to be notified that flight mode was enabled
    if (urfkillSpy.size() != 1)
    {
        ASSERT_TRUE(urfkillSpy.wait());
    }
    ASSERT_EQ(1, urfkillSpy.size());
    EXPECT_EQ(urfkillSpy.first(), QVariantList() << QVariant(true));

    // The switch enabled change should complete
    if (flightModeSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(flightModeSwitchSpy.wait());
    }
    ASSERT_EQ(2, flightModeSwitchSpy.size());
    EXPECT_EQ(flightModeSwitchSpy.last(), QVariantList() << QVariant(true));
    if (wifiSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(2, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.last(), QVariantList() << QVariant(true));

    // Wait for flight mode property change
    if (flightModeSpy.size() != 1)
    {
        ASSERT_TRUE(flightModeSpy.wait());
    }
    ASSERT_EQ(1, flightModeSpy.size());
    EXPECT_EQ(flightModeSpy.first(), QVariantList() << QVariant(true));

    // Check that nothing is happening again
    EXPECT_TRUE(connectivity->flightModeSwitchEnabled());
    EXPECT_TRUE(connectivity->wifiSwitchEnabled());

    // Hotspot not available when in flight mode
    EXPECT_FALSE(connectivity->hotspotSwitchEnabled());

    // The icing on the cake
    EXPECT_TRUE(connectivity->flightMode());

    // Start again
    urfkillSpy.clear();
    flightModeSwitchSpy.clear();
    wifiSwitchSpy.clear();
    hotspotSwitchSpy.clear();
    flightModeSpy.clear();

    // Disable flight mode
    connectivity->setFlightMode(false);

    // We should first get the unstoppable operation change
    ASSERT_TRUE(flightModeSwitchSpy.wait());
    ASSERT_EQ(1, flightModeSwitchSpy.size());
    EXPECT_EQ(flightModeSwitchSpy.first(), QVariantList() << QVariant(false));
    if (wifiSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(1, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.first(), QVariantList() << QVariant(false));

    // Wait to be notified that flight mode was disabled
    if (urfkillSpy.size() != 1)
    {
        ASSERT_TRUE(urfkillSpy.wait());
    }
    ASSERT_EQ(1, urfkillSpy.size());
    EXPECT_EQ(urfkillSpy.first(), QVariantList() << QVariant(false));

    // The toggles should become enabled again
    if (flightModeSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(flightModeSwitchSpy.wait());
    }
    ASSERT_EQ(2, flightModeSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.last(), QVariantList() << QVariant(true));
    if (wifiSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(2, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.last(), QVariantList() << QVariant(true));
    if (hotspotSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(hotspotSwitchSpy.wait());
    }
    ASSERT_EQ(1, hotspotSwitchSpy.size());
    EXPECT_EQ(hotspotSwitchSpy.first(), QVariantList() << QVariant(true));

    // Wait for flight mode property change
    if (flightModeSpy.size() != 1)
    {
        ASSERT_TRUE(flightModeSpy.wait());
    }
    ASSERT_EQ(1, flightModeSpy.size());
    EXPECT_EQ(flightModeSpy.first(), QVariantList() << QVariant(false));

    // Check that nothing is happening again
    EXPECT_TRUE(connectivity->flightModeSwitchEnabled());
    EXPECT_TRUE(connectivity->wifiSwitchEnabled());
    EXPECT_TRUE(connectivity->hotspotSwitchEnabled());

    // The icing on the cake
    EXPECT_FALSE(connectivity->flightMode());
}

TEST_F(TestConnectivityApi, WifiToggleTalksToUrfkill)
{
    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // Connect to the UrfKill mock
    OrgFreedesktopURfkillKillswitchInterface wifiKillswitchInterface(
            DBusTypes::URFKILL_BUS_NAME, DBusTypes::URFKILL_WIFI_OBJ_PATH,
            dbusTestRunner.systemConnection());
    QSignalSpy urfkillSpy(&wifiKillswitchInterface, SIGNAL(StateChanged()));

    // Connect the the service
    auto connectivity(newConnectivity());

    // Follow the wifiEnabled property
    QSignalSpy wifiEnabledSpy(connectivity.get(), SIGNAL(wifiEnabledUpdated(bool)));

    // Follow the switch enabled properties
    QSignalSpy flightModeSwitchSpy(connectivity.get(), SIGNAL(flightModeSwitchEnabledUpdated(bool)));
    QSignalSpy wifiSwitchSpy(connectivity.get(), SIGNAL(wifiSwitchEnabledUpdated(bool)));
    QSignalSpy hotspotSwitchSpy(connectivity.get(), SIGNAL(hotspotSwitchEnabledUpdated(bool)));

    // Check that nothing is happening yet
    EXPECT_TRUE(connectivity->flightModeSwitchEnabled());
    EXPECT_TRUE(connectivity->wifiSwitchEnabled());
    EXPECT_TRUE(connectivity->hotspotSwitchEnabled());
    EXPECT_TRUE(connectivity->wifiEnabled());
    EXPECT_EQ(0, wifiKillswitchInterface.state());

    // Disable WiFi
    connectivity->setwifiEnabled(false);

    // Check the switch enabled flags change
    ASSERT_TRUE(flightModeSwitchSpy.wait());
    ASSERT_EQ(1, flightModeSwitchSpy.size());
    EXPECT_EQ(flightModeSwitchSpy.first(), QVariantList() << QVariant(false));

    if (wifiSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(1, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.first(), QVariantList() << QVariant(false));

    if (hotspotSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(hotspotSwitchSpy.wait());
    }
    ASSERT_EQ(1, hotspotSwitchSpy.size());
    EXPECT_EQ(hotspotSwitchSpy.first(), QVariantList() << QVariant(false));

    // Wait to be notified that wifi was toggled
    if (urfkillSpy.size() != 1)
    {
        ASSERT_TRUE(urfkillSpy.wait());
    }
    ASSERT_EQ(1, urfkillSpy.size());

    // Switch should be re-enabled now
    if (flightModeSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(flightModeSwitchSpy.wait());
    }
    ASSERT_EQ(2, flightModeSwitchSpy.size());
    EXPECT_EQ(flightModeSwitchSpy.last(), QVariantList() << QVariant(true));

    if (hotspotSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(hotspotSwitchSpy.wait());
    }
    ASSERT_EQ(2, hotspotSwitchSpy.size());
    EXPECT_EQ(hotspotSwitchSpy.last(), QVariantList() << QVariant(true));

    if (wifiSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(2, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.last(), QVariantList() << QVariant(true));

    // Wait for wifi enabled property change
    if (wifiEnabledSpy.size() != 1)
    {
        ASSERT_TRUE(wifiEnabledSpy.wait());
    }
    ASSERT_EQ(1, wifiEnabledSpy.size());
    EXPECT_EQ(wifiEnabledSpy.first(), QVariantList() << QVariant(false));

    // Check switches are enabled
    EXPECT_TRUE(connectivity->flightModeSwitchEnabled());
    EXPECT_TRUE(connectivity->wifiSwitchEnabled());
    EXPECT_TRUE(connectivity->hotspotSwitchEnabled());

    // The icing on the cake
    EXPECT_FALSE(connectivity->wifiEnabled());
    EXPECT_EQ(1, wifiKillswitchInterface.state());

    // Start again
    urfkillSpy.clear();
    flightModeSwitchSpy.clear();
    wifiSwitchSpy.clear();
    hotspotSwitchSpy.clear();
    wifiEnabledSpy.clear();

    // Enable wifi
    connectivity->setwifiEnabled(true);

    // Toggles should be disabled
    ASSERT_TRUE(flightModeSwitchSpy.wait());
    ASSERT_EQ(1, flightModeSwitchSpy.size());
    EXPECT_EQ(flightModeSwitchSpy.first(), QVariantList() << QVariant(false));

    if (wifiSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(1, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.first(), QVariantList() << QVariant(false));

    if (hotspotSwitchSpy.size() != 1)
    {
        ASSERT_TRUE(hotspotSwitchSpy.wait());
    }
    ASSERT_EQ(1, hotspotSwitchSpy.size());
    EXPECT_EQ(hotspotSwitchSpy.first(), QVariantList() << QVariant(false));

    // Wait to be notified that wifi was toggled
    if (urfkillSpy.size() != 1)
    {
        ASSERT_TRUE(urfkillSpy.wait());
    }
    ASSERT_EQ(1, urfkillSpy.size());

    // Toggles should be re-enabled
    if (flightModeSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(flightModeSwitchSpy.wait());
    }
    ASSERT_EQ(2, flightModeSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.last(), QVariantList() << QVariant(true));

    if (wifiSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(wifiSwitchSpy.wait());
    }
    ASSERT_EQ(2, wifiSwitchSpy.size());
    EXPECT_EQ(wifiSwitchSpy.last(), QVariantList() << QVariant(true));

    // Hotspot should become available again
    if (hotspotSwitchSpy.size() != 2)
    {
        ASSERT_TRUE(hotspotSwitchSpy.wait());
    }
    ASSERT_EQ(2, hotspotSwitchSpy.size());
    EXPECT_EQ(hotspotSwitchSpy.last(), QVariantList() << QVariant(true));

    // Wait for wifi enabled property change
    if (wifiEnabledSpy.size() != 1)
    {
        ASSERT_TRUE(wifiEnabledSpy.wait());
    }
    ASSERT_EQ(1, wifiEnabledSpy.size());
    EXPECT_EQ(wifiEnabledSpy.first(), QVariantList() << QVariant(true));

    // All toggles should be enabled
    EXPECT_TRUE(connectivity->flightModeSwitchEnabled());
    EXPECT_TRUE(connectivity->wifiSwitchEnabled());
    EXPECT_TRUE(connectivity->hotspotSwitchEnabled());

    // The icing on the cake
    EXPECT_TRUE(connectivity->wifiEnabled());
    EXPECT_EQ(0, wifiKillswitchInterface.state());
}

TEST_F(TestConnectivityApi, Status)
{
    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // Connect the the service
    auto connectivity(newConnectivity());

    // Check we are online
    EXPECT_TRUE(connectivity->online());
    EXPECT_EQ(Connectivity::Status::Online, connectivity->status());

    // Listen to property changes
    QSignalSpy onlineSpy(connectivity.get(), SIGNAL(onlineUpdated(bool)));
    QSignalSpy statusSpy(connectivity.get(), SIGNAL(statusUpdated(connectivityqt::Connectivity::Status)));

    // Set up disconnected with flight mode on
    setGlobalConnectedState(NM_STATE_DISCONNECTED);

    // Wait for changes
    ASSERT_TRUE(onlineSpy.wait());
    if (statusSpy.isEmpty())
    {
        ASSERT_TRUE(statusSpy.wait());
    }

    // Check we are online
    EXPECT_FALSE(connectivity->online());
    EXPECT_EQ(Connectivity::Status::Offline, connectivity->status());

    // Check we have just one signal
    ASSERT_EQ(1, onlineSpy.size());
    ASSERT_EQ(1, statusSpy.size());

    EXPECT_EQ(QVariantList() << QVariant(false), onlineSpy.first());
    EXPECT_EQ(Connectivity::Status::Offline, qvariant_cast<Connectivity::Status>(statusSpy.first().first()));
}

TEST_F(TestConnectivityApi, Limitations)
{
    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // Connect the the service
    auto connectivity(newConnectivity());

    // These methods will always return these values - i.e. are not implemented
    EXPECT_EQ(QVector<Connectivity::Limitations>(), connectivity->limitations());
    EXPECT_FALSE(connectivity->limitedBandwith());
}

TEST_F(TestConnectivityApi, HotspotConfig)
{
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);

    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    auto& powerdMock = dbusMock.mockInterface(DBusTypes::POWERD_DBUS_NAME,
                           DBusTypes::POWERD_DBUS_PATH,
                           DBusTypes::POWERD_DBUS_INTERFACE,
                           QDBusConnection::SystemBus);
    QSignalSpy powerdMockCallSpy(
                           &powerdMock,
                           SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    auto& nmMock = dbusMock.mockInterface(NM_DBUS_SERVICE,
                           NM_DBUS_PATH,
                           NM_DBUS_INTERFACE,
                           QDBusConnection::SystemBus);
    QSignalSpy nmMockCallSpy(
                           &nmMock,
                           SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    auto& nmSettingsMock = dbusMock.mockInterface(NM_DBUS_SERVICE,
                           NM_DBUS_PATH_SETTINGS,
                           NM_DBUS_INTERFACE_SETTINGS,
                           QDBusConnection::SystemBus);
    QSignalSpy nmSettingsMockCallSpy(
                           &nmSettingsMock,
                           SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    OrgFreedesktopNetworkManagerSettingsInterface settings(
            NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS, dbusTestRunner.systemConnection());
    QSignalSpy settingsNewConnectionSpy(
                           &settings,
                           SIGNAL(NewConnection(const QDBusObjectPath &)));

    // Connect the the service
    auto connectivity(newConnectivity());

    QSignalSpy storedSpy(connectivity.get(), SIGNAL(hotspotStoredUpdated(bool)));
    QSignalSpy enabledSpy(connectivity.get(), SIGNAL(hotspotEnabledUpdated(bool)));
    QSignalSpy passwordSpy(connectivity.get(), SIGNAL(hotspotPasswordUpdated(const QString&)));

    EXPECT_EQ("Ubuntu", connectivity->hotspotSsid().toStdString());
    EXPECT_EQ("wpa-psk", connectivity->hotspotAuth().toStdString());
    EXPECT_FALSE(connectivity->hotspotStored());
    EXPECT_FALSE(connectivity->hotspotEnabled());

    connectivity->setHotspotPassword("the password");

    if (passwordSpy.empty())
    {
        ASSERT_TRUE(passwordSpy.wait());
    }
    EXPECT_FALSE(passwordSpy.empty());
    EXPECT_EQ("the password", connectivity->hotspotPassword().toStdString());

    connectivity->setHotspotEnabled(true);

    if (enabledSpy.empty())
    {
        ASSERT_TRUE(enabledSpy.wait());
    }
    EXPECT_FALSE(enabledSpy.empty());

    if (storedSpy.empty())
    {
        ASSERT_TRUE(storedSpy.wait());
    }
    EXPECT_FALSE(storedSpy.empty());

    if (powerdMockCallSpy.empty())
    {
        ASSERT_TRUE(powerdMockCallSpy.wait());
    }
    EXPECT_FALSE(powerdMockCallSpy.empty());

    if (nmSettingsMockCallSpy.empty())
    {
        ASSERT_TRUE(nmSettingsMockCallSpy.wait());
    }
    EXPECT_FALSE(nmSettingsMockCallSpy.empty());

    if (settingsNewConnectionSpy.empty())
    {
        ASSERT_TRUE(settingsNewConnectionSpy.wait());
    }
    EXPECT_FALSE(settingsNewConnectionSpy.empty());

    EXPECT_TRUE(connectivity->hotspotEnabled());
    EXPECT_TRUE(connectivity->hotspotStored());

    // Connect to method calls on the newly added connection
    auto connectionPath = qvariant_cast<QDBusObjectPath>(settingsNewConnectionSpy.first().first());
    auto& connectionSettingsMock = dbusMock.mockInterface(NM_DBUS_SERVICE, connectionPath.path(),
                           NM_DBUS_INTERFACE_SETTINGS_CONNECTION,
                           QDBusConnection::SystemBus);
    QSignalSpy connectionSettingsMockCallSpy(
                           &connectionSettingsMock,
                           SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    // Expect a wakelock request
    EXPECT_EQ(
        QVariantList({"connectivity-service", int(1)}),
        getMethodCall(powerdMockCallSpy, "requestSysState"));

    {
        auto call = getMethodCall(nmSettingsMockCallSpy, "AddConnection");
        // Decode the QDBusArgument
        QVariantDictMap connection;
        qvariant_cast<QDBusArgument>(call.first()) >> connection;
        EXPECT_EQ(QVariantMap({
            {"mode", "ap"},
            {"security", "802-11-wireless-security"},
            {"ssid", "Ubuntu"}
        }), connection["802-11-wireless"]);
        EXPECT_EQ(QVariantMap({
            {"group", "ccmp"},
            {"key-mgmt", "wpa-psk"},
            {"pairwise" ,  QStringList{"ccmp"}},
            {"proto" ,  QStringList{"rsn"}},
            {"psk", "the password"}
        }), connection["802-11-wireless-security"]);
        EXPECT_FALSE(connection["connection"]["autoconnect"].toBool());
    }

    // Next we'll disable the hotspot
    storedSpy.clear();
    enabledSpy.clear();
    passwordSpy.clear();
    powerdMockCallSpy.clear();
    nmMockCallSpy.clear();
    nmSettingsMockCallSpy.clear();

    connectivity->setHotspotEnabled(false);

    if (powerdMockCallSpy.empty())
    {
        ASSERT_TRUE(powerdMockCallSpy.wait());
    }
    EXPECT_FALSE(powerdMockCallSpy.empty());

    if (nmMockCallSpy.empty())
    {
        ASSERT_TRUE(nmMockCallSpy.wait());
    }
    EXPECT_FALSE(nmMockCallSpy.empty());

    if (enabledSpy.empty())
    {
        ASSERT_TRUE(enabledSpy.wait());
    }
    EXPECT_FALSE(enabledSpy.empty());

    EXPECT_FALSE(connectivity->hotspotEnabled());
    EXPECT_TRUE(connectivity->hotspotStored());

    // Expect a wakelock cancel
    EXPECT_EQ(
        QVariantList{"dummy_cookie"},
        getMethodCall(powerdMockCallSpy, "clearSysState"));

    // We should expect a disconnect call
    {
        auto call = getMethodCall(nmMockCallSpy, "DeactivateConnection");
        EXPECT_EQ("/org/freedesktop/NetworkManager/ActiveConnection/0", qvariant_cast<QDBusObjectPath>(call.first()).path());
    }
}

TEST_F(TestConnectivityApi, InsecureHotspotConfig)
{
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);

    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    auto& nmSettingsMock = dbusMock.mockInterface(NM_DBUS_SERVICE,
                           NM_DBUS_PATH_SETTINGS,
                           NM_DBUS_INTERFACE_SETTINGS,
                           QDBusConnection::SystemBus);
    QSignalSpy nmSettingsMockCallSpy(
                           &nmSettingsMock,
                           SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    OrgFreedesktopNetworkManagerSettingsInterface settings(
            NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS, dbusTestRunner.systemConnection());
    QSignalSpy settingsNewConnectionSpy(
                           &settings,
                           SIGNAL(NewConnection(const QDBusObjectPath &)));

    // Connect the the service
    auto connectivity(newConnectivity());

    QSignalSpy storedSpy(connectivity.get(), SIGNAL(hotspotStoredUpdated(bool)));
    QSignalSpy enabledSpy(connectivity.get(), SIGNAL(hotspotEnabledUpdated(bool)));
    QSignalSpy authSpy(connectivity.get(), SIGNAL(hotspotAuthUpdated(const QString&)));

    EXPECT_EQ("Ubuntu", connectivity->hotspotSsid().toStdString());
    EXPECT_EQ("wpa-psk", connectivity->hotspotAuth().toStdString());
    EXPECT_FALSE(connectivity->hotspotStored());
    EXPECT_FALSE(connectivity->hotspotEnabled());

    connectivity->setHotspotAuth("none");

    if (authSpy.empty())
    {
        ASSERT_TRUE(authSpy.wait());
    }
    EXPECT_FALSE(authSpy.empty());
    EXPECT_EQ("none", connectivity->hotspotAuth().toStdString());

    connectivity->setHotspotEnabled(true);

    if (enabledSpy.empty())
    {
        ASSERT_TRUE(enabledSpy.wait());
    }
    EXPECT_FALSE(enabledSpy.empty());

    if (storedSpy.empty())
    {
        ASSERT_TRUE(storedSpy.wait());
    }
    EXPECT_FALSE(storedSpy.empty());

    if (nmSettingsMockCallSpy.empty())
    {
        ASSERT_TRUE(nmSettingsMockCallSpy.wait());
    }
    EXPECT_FALSE(nmSettingsMockCallSpy.empty());

    if (settingsNewConnectionSpy.empty())
    {
        ASSERT_TRUE(settingsNewConnectionSpy.wait());
    }
    EXPECT_FALSE(settingsNewConnectionSpy.empty());

    EXPECT_TRUE(connectivity->hotspotEnabled());
    EXPECT_TRUE(connectivity->hotspotStored());

    // Connect to method calls on the newly added connection
    auto connectionPath = qvariant_cast<QDBusObjectPath>(settingsNewConnectionSpy.first().first());
    auto& connectionSettingsMock = dbusMock.mockInterface(NM_DBUS_SERVICE, connectionPath.path(),
                           NM_DBUS_INTERFACE_SETTINGS_CONNECTION,
                           QDBusConnection::SystemBus);
    QSignalSpy connectionSettingsMockCallSpy(
                           &connectionSettingsMock,
                           SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    {
        auto call = getMethodCall(nmSettingsMockCallSpy, "AddConnection");
        // Decode the QDBusArgument
        QVariantDictMap connection;
        qvariant_cast<QDBusArgument>(call.first()) >> connection;
        EXPECT_EQ(QVariantMap({
            {"mode", "ap"},
            {"ssid", "Ubuntu"}
        }), connection["802-11-wireless"]);
        EXPECT_EQ(QVariantMap(), connection["802-11-wireless-security"]);
        EXPECT_FALSE(connection["connection"]["autoconnect"].toBool());
    }
}

TEST_F(TestConnectivityApi, HotspotModemAvailable)
{
    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);

    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    auto& nmSettingsMock = dbusMock.mockInterface(NM_DBUS_SERVICE,
                           NM_DBUS_PATH_SETTINGS,
                           NM_DBUS_INTERFACE_SETTINGS,
                           QDBusConnection::SystemBus);
    QSignalSpy nmSettingsMockCallSpy(
                           &nmSettingsMock,
                           SIGNAL(MethodCalled(const QString &, const QVariantList &)));

    OrgFreedesktopNetworkManagerSettingsInterface settings(
            NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS, dbusTestRunner.systemConnection());
    QSignalSpy settingsNewConnectionSpy(
                           &settings,
                           SIGNAL(NewConnection(const QDBusObjectPath &)));

    // Connect the the service
    auto connectivity(newConnectivity());

    EXPECT_TRUE(connectivity->modemAvailable());
}

TEST_F(TestConnectivityApi, MobileDataDisablePowersOffAllSims)
{
    auto modem2 = createModem("ril_1");
    setConnectionManagerProperty(modem2, "Powered", true);

    setSimManagerProperty(modem2, "CardIdentifier", "");

    setGlobalConnectedState(NM_STATE_DISCONNECTED);
    auto device = createWiFiDevice(NM_DEVICE_STATE_DISCONNECTED);

    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // Connect the the service
    auto connectivity(newConnectivity());

    setSimManagerProperty(modem2, "CardIdentifier", "893581234000000000001");

    QSignalSpy mobileDataEnabledSpy(connectivity.get(), SIGNAL(mobileDataEnabledUpdated(bool)));
    QSignalSpy simForMobileDataSpy(connectivity.get(), SIGNAL(simForMobileDataUpdated(Sim*)));

    // One of the modems was started and we had no settings.
    // So we start by assuming mobile data was enabled.
    if (!connectivity->mobileDataEnabled())
    {
        WAIT_FOR_SIGNALS(mobileDataEnabledSpy, 1)
    }
    mobileDataEnabledSpy.clear();
    EXPECT_TRUE(connectivity->mobileDataEnabled());

    if (!connectivity->simForMobileData())
    {
        WAIT_FOR_SIGNALS(simForMobileDataSpy, 1)
    }
    simForMobileDataSpy.clear();
    EXPECT_TRUE(connectivity->simForMobileData() != nullptr);


    QSignalSpy simsModelRowsInsertedSpy(connectivity->sims(), SIGNAL(rowsInserted(const QModelIndex &, int, int)));
    QSignalSpy modemsModelRowsInsertedSpy(connectivity->modems(), SIGNAL(rowsInserted(const QModelIndex &, int, int)));
    WAIT_FOR_ROW_COUNT(simsModelRowsInsertedSpy, connectivity->sims(), 2)
    WAIT_FOR_ROW_COUNT(modemsModelRowsInsertedSpy, connectivity->modems(), 2)

    auto modems = getSortedModems(*connectivity);
    auto simForMobileData = connectivity->simForMobileData();
    auto secondSim = getModemSim(*modems, 1);

    ASSERT_TRUE(simForMobileData);
    ASSERT_TRUE(secondSim);
    // These should be the exact same object.
    EXPECT_TRUE(simForMobileData == secondSim);

    // Only the second modem should be powered
    EXPECT_FALSE(getConnectionManagerProperties(modem)["Powered"].toBool());
    EXPECT_TRUE(getConnectionManagerProperties(modem2)["Powered"].toBool());

    auto& connectionManager(dbusMock.ofonoConnectionManagerInterface(modem2));
    QSignalSpy connectionManagerPropertyChangedSpy(
                               &connectionManager,
                               SIGNAL(PropertyChanged(const QString &, const QDBusVariant &)));

    // Disable all mobile data.
    connectivity->setMobileDataEnabled(false);

    WAIT_FOR_SIGNALS(connectionManagerPropertyChangedSpy, 1)

    // Both modems should by un-powered
    EXPECT_FALSE(getConnectionManagerProperties(modem)["Powered"].toBool());
    EXPECT_FALSE(getConnectionManagerProperties(modem2)["Powered"].toBool());

    if (connectivity->mobileDataEnabled())
    {
        WAIT_FOR_SIGNALS(mobileDataEnabledSpy, 1)
    }
    EXPECT_FALSE(connectivity->mobileDataEnabled());
}

TEST_F(TestConnectivityApi, SettingsRestoredOnStartup)
{

    QString path = temporaryDir.path() + "/config.ini";
    auto settings = make_unique<QSettings>(path, QSettings::IniFormat);

    settings->beginGroup("Sims/893581234000000000000/");
    settings->setValue("Imsi", "310150000000000");
    settings->setValue("PrimaryPhoneNumber", "123456789");
    settings->setValue("Mcc", "310");
    settings->setValue("Mnc", "150");
    settings->setValue("PreferredLanguages", QVariant(QList<QString>({"en"})));
    settings->setValue("DataRoamingEnabled", true);
    settings->endGroup();

    settings->beginGroup("Sims/893581234000000000001/");
    settings->setValue("Imsi", "310150000000001");
    settings->setValue("PrimaryPhoneNumber", "123456789");
    settings->setValue("Mcc", "310");
    settings->setValue("Mnc", "150");
    settings->setValue("PreferredLanguages", QVariant(QList<QString>({"en"})));
    settings->setValue("DataRoamingEnabled", false);
    settings->endGroup();

    settings->setValue("KnownSims", QVariant(QList<QString>({"893581234000000000000", "893581234000000000001"})));
    settings->setValue("SimForMobileData", "893581234000000000001");
    settings->setValue("MobileDataEnabled", true);
    settings->sync();

    setConnectionManagerProperty(modem, "Powered", true);
    setConnectionManagerProperty(modem, "RoamingAllowed", false);

    auto modem2 = createModem("ril_1");
    setConnectionManagerProperty(modem2, "Powered", false);
    setConnectionManagerProperty(modem2, "RoamingAllowed", true);

    auto& connectionManager(dbusMock.ofonoConnectionManagerInterface(modem));
    QSignalSpy connectionManagerPropertyChangedSpy(
                &connectionManager,
                SIGNAL(PropertyChanged(const QString &, const QDBusVariant &)));

    auto& connectionManager2(dbusMock.ofonoConnectionManagerInterface(modem2));
    QSignalSpy connectionManager2PropertyChangedSpy(
                &connectionManager2,
                SIGNAL(PropertyChanged(const QString &, const QDBusVariant &)));

    // Start the indicator
    ASSERT_NO_THROW(startIndicator());

    // make sure the ofono mock has updated it's values
    while (getConnectionManagerProperties(modem)["Powered"].toBool() == true)
    {
        ASSERT_TRUE(connectionManagerPropertyChangedSpy.wait());
    }
    while (getConnectionManagerProperties(modem)["RoamingAllowed"].toBool() == false)
    {
        ASSERT_TRUE(connectionManagerPropertyChangedSpy.wait());
    }
    while (getConnectionManagerProperties(modem2)["Powered"].toBool() == false)
    {
        ASSERT_TRUE(connectionManager2PropertyChangedSpy.wait());
    }
    while (getConnectionManagerProperties(modem2)["RoamingAllowed"].toBool() == true)
    {
        ASSERT_TRUE(connectionManager2PropertyChangedSpy.wait());
    }


    // Check that settings are restored on startup

    EXPECT_FALSE(getConnectionManagerProperties(modem)["Powered"].toBool());
    EXPECT_TRUE(getConnectionManagerProperties(modem)["RoamingAllowed"].toBool());

    EXPECT_TRUE(getConnectionManagerProperties(modem2)["Powered"].toBool());
    EXPECT_FALSE(getConnectionManagerProperties(modem2)["RoamingAllowed"].toBool());
}

}
