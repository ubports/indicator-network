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
 * Authors:
 *     Pete Woods <pete.woods@canonical.com>
 */

#include <icons.h>

using namespace nmofono;

QString Icons::strengthIcon(int8_t strength)
{
    /* Using same values as used by Android, not linear (LP: #1329945)*/
    if (strength >= 39)
        return "gsm-3g-full";
    else if (strength >= 26)
        return "gsm-3g-high";
    else if (strength >= 16)
        return "gsm-3g-medium";
    else if (strength >= 6)
        return "gsm-3g-low";
    else
        return "gsm-3g-none";
}

QString Icons::bearerIcon(wwan::Modem::Bearer bearer)
{
    switch (bearer)
    {
    case wwan::Modem::Bearer::notAvailable:
        return "";
    case wwan::Modem::Bearer::gprs:
        return "network-cellular-pre-edge";
    case wwan::Modem::Bearer::edge:
        return "network-cellular-edge";
    case wwan::Modem::Bearer::umts:
        return "network-cellular-3g";
    case wwan::Modem::Bearer::hspa:
        return "network-cellular-hspa";
    case wwan::Modem::Bearer::hspa_plus:
        return "network-cellular-hspa-plus";
    case wwan::Modem::Bearer::lte:
        return "network-cellular-4g";
    }
    // shouldn't be reached
    return QString();
}

QString Icons::wifiIcon(nmofono::wifi::WifiLink::Signal signal)
{
    switch (signal)
    {
    case wifi::WifiLink::Signal::disconnected:
        return "wifi-no-connection";
    case wifi::WifiLink::Signal::signal_0:
        return "nm-signal-00";
    case wifi::WifiLink::Signal::signal_0_secure:
        return "nm-signal-00-secure";
    case wifi::WifiLink::Signal::signal_25:
        return "nm-signal-25";
    case wifi::WifiLink::Signal::signal_25_secure:
        return "nm-signal-25-secure";
    case wifi::WifiLink::Signal::signal_50:
        return "nm-signal-50";
    case wifi::WifiLink::Signal::signal_50_secure:
        return "nm-signal-50-secure";
    case wifi::WifiLink::Signal::signal_75:
        return "nm-signal-75";
    case wifi::WifiLink::Signal::signal_75_secure:
        return "nm-signal-75-secure";
    case wifi::WifiLink::Signal::signal_100:
        return "nm-signal-100";
    case wifi::WifiLink::Signal::signal_100_secure:
        return "nm-signal-100-secure";
    }
    // shouldn't be reached
    return QString();
}
