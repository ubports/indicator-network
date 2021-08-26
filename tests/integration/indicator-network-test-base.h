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

#pragma once

#include <connectivityqt/connectivity.h>
#include <connectivityqt/modems-list-model.h>


#include <dbus-types.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <libqtdbusmock/DBusMock.h>

#include <unity/gmenuharness/MatchUtils.h>
#include <unity/gmenuharness/MenuMatcher.h>

#include <NetworkManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QTest>

inline QString qVariantToString(const QVariant& variant) {
    QString output;
    QDebug dbg(&output);
    dbg << variant;
    return output;
}

inline void PrintTo(const QVariant& variant, std::ostream* os) {
    QString output;
    QDebug dbg(&output);
    dbg << variant;

    *os << "QVariant(" << output.toStdString() << ")";
}

inline void PrintTo(const QString& s, std::ostream* os) {
    *os << "\"" << s.toStdString() << "\"";
}

inline void PrintTo(const QStringList& list, std::ostream* os) {
    QString output;
    QDebug dbg(&output);
    dbg << list;

    *os << "QStringList(" << output.toStdString() << ")";
}

inline void PrintTo(const QList<QDBusObjectPath>& list, std::ostream* os) {
    QString output;
    for (const auto& path: list)
    {
        output.append("\"" + path.path() + "\",");
    }

    *os << "QList<QDBusObjectPath>(" << output.toStdString() << ")";
}

#define WAIT_FOR_SIGNALS(signalSpy, signalsExpected)\
{\
    while (signalSpy.size() < signalsExpected)\
    {\
        ASSERT_TRUE(signalSpy.wait()) << "Waiting for " << signalsExpected << " signals, got " << signalSpy.size();\
    }\
    ASSERT_EQ(signalsExpected, signalSpy.size()) << "Waiting for " << signalsExpected << " signals, got " << signalSpy.size();\
}

#define WAIT_FOR_ROW_COUNT(signalSpy, model, expectedRowCount)\
{\
    while (model->rowCount() < expectedRowCount)\
    {\
        ASSERT_TRUE(signalSpy.wait()) << "Waiting for model to have " << expectedRowCount << " rows, got " << model->rowCount();\
    }\
    ASSERT_EQ(expectedRowCount, model->rowCount()) << "Waiting for model to have " << expectedRowCount << " rows, got " << model->rowCount();\
}

#define CHECK_METHOD_CALL(signalSpy, signalIndex, methodName, ...)\
{\
    QVariantList const& call(signalSpy.at(signalIndex));\
    EXPECT_EQ(methodName, call.at(0));\
    auto arguments = vector<pair<int, QVariant>>{__VA_ARGS__};\
    if (!arguments.empty())\
    {\
        QVariantList const& args(call.at(1).toList());\
        ASSERT_LE(arguments.back().first + 1, args.size());\
        for (auto const& argument : arguments)\
        {\
            EXPECT_EQ(argument.second, args.at(argument.first));\
        }\
    }\
}

class IndicatorNetworkTestBase: public testing::Test
{
public:
    enum class Secure
    {
        insecure,
        wpa,
        wpa_enterprise
    };

    enum class ApMode
    {
        infra,
        adhoc
    };

    enum class ConnectionStatus
    {
        connected,
        disconnected
    };

    IndicatorNetworkTestBase();

    ~IndicatorNetworkTestBase();

protected:
    void SetUp() override;

    static unity::gmenuharness::MenuMatcher::Parameters phoneParameters();

    static unity::gmenuharness::MenuMatcher::Parameters unlockSimParameters(std::string const& busName, int exportId);

    void startIndicator();

    QString createEthernetDevice(int state, const QString& id = "0");

    QString createWiFiDevice(int state, const QString& id = "0");

    static QString randomMac();

    void enableWiFi();

    void disableWiFi();

    QString createAccessPoint(const QString& id, const QString& ssid, const QString& device, uchar strength = 100,
                              Secure secure = Secure::wpa, ApMode apMode = ApMode::infra, const QString& mac = randomMac());

    void removeAccessPoint(const QString& device, const QString& ap);

    QString createAccessPointConnection(const QString& id, const QString& ssid, const QString& device);

    void removeWifiConnection(const QString& device, const QString& connection);

    QString createActiveConnection(const QString& id, const QString& device, const QString& connection, const QString& specificObject);

    void removeActiveConnection(const QString& device, const QString& active_connection);

    void setGlobalConnectedState(int state);

    void setNmProperty(const QString& path, const QString& iface, const QString& name, const QVariant& value);

    QString createModem(const QString& id);

    void setModemProperty(const QString& path, const QString& propertyName, const QVariant& value);

    void setSimManagerProperty(const QString& path, const QString& propertyName, const QVariant& value);

    void setConnectionManagerProperty(const QString& path, const QString& propertyName, const QVariant& value);

    QVariantMap getConnectionManagerProperties(const QString& path);

    void setNetworkRegistrationProperty(const QString& path, const QString& propertyName, const QVariant& value);

    OrgFreedesktopDBusMockInterface& notificationsMockInterface();

    OrgFreedesktopDBusMockInterface& modemMockInterface(const QString& path);

    OrgFreedesktopDBusMockInterface& networkManagerMockInterface();

    QString createVpnConnection(const QString& id,
                                const QString& serviceType = "org.freedesktop.NetworkManager.openvpn",
                                const QStringMap& =
                                {
                                    {"connection-type", "tls"},
                                    {"remote", "remote"},
                                    {"ca", "/path/to/ca.crt"},
                                    {"cert", "/path/to/cert.crt"},
                                    {"cert-pass-flags", "1"},
                                    {"key", "/path/to/key.key"}
                                },
                                const QStringMap& secrets = {});

    QString importVpnConnection();

    void deleteSettings(const QString& path);

    connectivityqt::Connectivity::UPtr newConnectivity();

    QVariantList getMethodCall(const QSignalSpy& spy, const QString& method);

    static bool qDBusArgumentToMap(QVariant const& variant, QVariantMap& map);

    static QString firstModem();

    static unity::gmenuharness::MenuItemMatcher flightModeSwitch(bool toggled = false);
    static unity::gmenuharness::MenuItemMatcher mobileDataSwitch(bool toggled = false);

    static unity::gmenuharness::MenuItemMatcher accessPoint(const std::string& ssid, Secure secure,
                ApMode apMode, ConnectionStatus connectionStatus, uchar strength = 100);

    static unity::gmenuharness::MenuItemMatcher wifiEnableSwitch(bool toggled = true);

    static unity::gmenuharness::MenuItemMatcher wifiSettings();

    static unity::gmenuharness::MenuItemMatcher modemInfo(const std::string& simIdentifier,
                const std::string& label,
                const std::string& statusIcon,
                bool locked = false,
                const std::string& connectivityIcon = "");

    static unity::gmenuharness::MenuItemMatcher cellularSettings();

    static unity::gmenuharness::MenuItemMatcher vpnSettings();

    static unity::gmenuharness::MenuItemMatcher vpnConnection(const std::string& name, ConnectionStatus connected = ConnectionStatus::disconnected);

    static connectivityqt::Sim* getModemSim(const QAbstractItemModel& model, int idx)
    {
        auto sim = model.data(model.index(idx,0),
                              connectivityqt::ModemsListModel::RoleSim)
                .value<connectivityqt::Sim*>();
        EXPECT_TRUE(sim);
        return sim;
    }

    static std::unique_ptr<QSortFilterProxyModel> getSortedModems(connectivityqt::Connectivity& connectivity);

    QtDBusTest::DBusTestRunner dbusTestRunner;

    QtDBusMock::DBusMock dbusMock;

    QtDBusTest::DBusServicePtr indicator;

    QString modem;

    QTemporaryDir temporaryDir;
};
