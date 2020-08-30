/*
 * Copyright (C) 2014, 2015 Canonical, Ltd.
 *
 * Authors:
 *    Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *    Jonas G. Drange <jonas.drange@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include <QDBusConnection>
#include <memory>

#include <nmofono/connection/active-connection-manager.h>

 /**
 * HotspotManager API
 * ==============================
 *
 * Methods
 *
 * Signals
 *   enabledChanged(bool enabled)
 *     Signal that gets emitted when the hotspot is disabled or enabled.
 *
 *   storedChanged(bool stored)
 *     Signal that gets emitted when a hotspot was stored.
 *
 *   ssidChanged(QByteArray ssid)
 *     Signal that gets emitted when the ssid of the hotspot was changed.
 *
 *   passwordChanged(QString password)
 *     Signal that gets emitted when the password of the hotspot was changed.
 *
 *   modeChanged(QString mode)
 *     Signal that gets emitted when the mode was changed.
 *
 *   authChanged(QString auth)
 *
 *   Note that none of these signal will be emitted if a change to the hotspot
 *   was made by anyone else than the HotspotManager. Right now, the canonical
 *   way to edit a hotspot, is through the hotspot manager.
 *
 *
 *   reportError(int reason)
 *     The reasons correspond to https://developer.gnome.org/
 *         NetworkManager/0.9/spec.html#type-NM_DEVICE_STATE_REASON
 *
 * Properties
 *   bool enabled [readwrite]
 *     Whether or not the hotspot is enabled.
 *
 *   bool stored [readonly]
 *     Whether or not a hotspot is known to the hotspotmanager.
 *
 *   QByteArray ssid [readwrite]
 *     The current SSID of the hotspot.
 *
 *   QString auth [readwrite]
 *     The current authentication of the hotspot. The default for this property
 *     is "wpa-psk" and is currently the only supported scheme. WEP is unsupported
 *     by design.
 *
 *     Possible authentication schemes: wpa-psk, none
 *
 *
 *   QString password [readwrite]
 *     The current Pre-Shared-Key for the hotspot. If the key is 64-characters
 *     long, it must contain only hexadecimal characters and is interpreted as a
 *     hexadecimal WPA key. Otherwise, the key must be between 8 and 63 ASCII
 *     characters and is interpreted as a WPA passphrase.
 *
 *   QString mode [readwrite, optional]
 *     The current hotspot mode. The default of this value is "ap", but can be
 *     set to "p2p" or "adhoc". "p2p" and "adhoc" is currently not fully supported.
 *
 *     TODO: Complete support for adhoc and p2p modes.
 */
namespace nmofono
{

class HotspotManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY( bool enabled
        READ enabled
        WRITE setEnabled
        NOTIFY enabledChanged)

    Q_PROPERTY( QByteArray ssid
        READ ssid
        WRITE setSsid
        NOTIFY ssidChanged)

    Q_PROPERTY( QString password
        READ password
        WRITE setPassword
        NOTIFY passwordChanged)

    Q_PROPERTY( QString mode
        READ mode
        WRITE setMode
        NOTIFY modeChanged)

    Q_PROPERTY( QString auth
        READ auth
        WRITE setAuth
        NOTIFY authChanged)

    Q_PROPERTY( bool stored
        READ stored
        NOTIFY storedChanged)

    Q_PROPERTY( bool disconnectWifi
        READ disconnectWifi
        NOTIFY disconnectWifiChanged)

public:
    typedef std::shared_ptr<HotspotManager> SPtr;

    explicit HotspotManager(connection::ActiveConnectionManager::SPtr activeConnectionManager,
                            const QDBusConnection& connection,
                            QObject *parent = nullptr);

    ~HotspotManager() = default;

    bool enabled() const;

    bool stored() const;

    QByteArray ssid() const;

    QString password() const;

    QString mode() const;

    QString auth() const;

    QString interface() const;

    bool disconnectWifi() const;

Q_SIGNALS:
    void enabledChanged(bool enabled);

    void storedChanged(bool stored);

    void ssidChanged(const QByteArray& ssid);

    void passwordChanged(const QString& password);

    void modeChanged(const QString& mode);

    void authChanged(const QString& auth);

    void disconnectWifiChanged(bool disconnect);

    /*
     * The mapping of code to string is taken from
     *  http://bazaar.launchpad.net/~vcs-imports/
     *  network-manager/trunk/view/head:/cli/src/common.c
     *
     * NetworkManager documentation: https://developer.gnome.org/
     *  NetworkManager/0.9/spec.html#type-NM_DEVICE_STATE_REASON
     */
    void reportError(int reason);

public Q_SLOTS:
    void setEnabled(bool);

    void setSsid(const QByteArray&);

    void setPassword(const QString&);

    void setMode(const QString&);

    void setAuth(const QString&);

private:
    class Priv;
    std::shared_ptr<Priv> d;

    void setMtkTetheringEnabled(bool);
};

}
