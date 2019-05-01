/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Pete Woods <pete.woods@canonical.com>
 */

#pragma once

#include <connectivityqt/vpn-connection.h>

namespace connectivityqt
{

class Q_DECL_EXPORT OpenvpnConnection : public VpnConnection
{
    Q_OBJECT

public:
    UNITY_DEFINES_PTRS(OpenvpnConnection);

    Q_ENUMS(ConnectionType)
    enum ConnectionType
    {
        TLS,
        PASSWORD,
        PASSWORD_TLS,
        STATIC_KEY
    };

    Q_ENUMS(DevType)
    enum DevType
    {
        TUN,
        TAP
    };

    Q_ENUMS(Cipher)
    enum Cipher
    {
        DEFAULT_CIPHER,
        DES_CBC,
        RC2_CBC,
        DES_EDE_CBC,
        DES_EDE3_CBC,
        DESX_CBC,
        RC2_40_CBC,
        CAST5_CBC,
        AES_128_CBC,
        AES_192_CBC,
        AES_256_CBC,
        CAMELLIA_128_CBC,
        CAMELLIA_192_CBC,
        CAMELLIA_256_CBC,
        SEED_CBC,
        AES_128_CBC_HMAC_SHA1,
        AES_256_CBC_HMAC_SHA1,
        NONE
    };

    Q_ENUMS(Auth)
    enum Auth
    {
        DEFAULT_AUTH,
        NONE,
        RSA_MD4,
        MD5,
        SHA1,
        SHA224,
        SHA256,
        SHA384,
        SHA512,
        RIPEMD160
    };

    Q_ENUMS(TlsType)
    enum TlsType
    {
        SERVER,
        CLIENT
    };

    Q_ENUMS(KeyDir)
    enum KeyDir
    {
        KEY_NONE,
        ZERO,
        ONE
    };

    Q_ENUMS(ProxyType)
    enum ProxyType
    {
        NOT_REQUIRED,
        HTTP,
        SOCKS
    };

    OpenvpnConnection(const QDBusObjectPath& path, const QDBusConnection& connection);

    virtual ~OpenvpnConnection();

    Type type() const override;

    // Basic properties

    Q_PROPERTY(QString ca READ ca WRITE setCa NOTIFY caChanged)
    QString ca() const;

    Q_PROPERTY(QString cert READ cert WRITE setCert NOTIFY certChanged)
    QString cert() const;

    Q_PROPERTY(QString certPass READ certPass WRITE setCertPass NOTIFY certPassChanged)
    QString certPass() const;

    Q_PROPERTY(ConnectionType connectionType READ connectionType WRITE setConnectionType NOTIFY connectionTypeChanged)
    ConnectionType connectionType() const;

    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)
    QString key() const;

    Q_PROPERTY(QString localIp READ localIp WRITE setLocalIp NOTIFY localIpChanged)
    QString localIp() const;

    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    QString password() const;

    Q_PROPERTY(QString remote READ remote WRITE setRemote NOTIFY remoteChanged)
    QString remote() const;

    Q_PROPERTY(QString remoteIp READ remoteIp WRITE setRemoteIp NOTIFY remoteIpChanged)
    QString remoteIp() const;

    Q_PROPERTY(QString staticKey READ staticKey WRITE setStaticKey NOTIFY staticKeyChanged)
    QString staticKey() const;

    Q_PROPERTY(KeyDir staticKeyDirection READ staticKeyDirection WRITE setStaticKeyDirection NOTIFY staticKeyDirectionChanged)
    KeyDir staticKeyDirection() const;

    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    QString username() const;

    // Advanced general properties

    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    int port() const;

    Q_PROPERTY(bool portSet READ portSet WRITE setPortSet NOTIFY portSetChanged)
    bool portSet() const;

    Q_PROPERTY(int renegSeconds READ renegSeconds WRITE setRenegSeconds NOTIFY renegSecondsChanged)
    int renegSeconds() const;

    Q_PROPERTY(bool renegSecondsSet READ renegSecondsSet WRITE setRenegSecondsSet NOTIFY renegSecondsSetChanged)
    bool renegSecondsSet() const;

    Q_PROPERTY(bool compLzo READ compLzo WRITE setCompLzo NOTIFY compLzoChanged)
    bool compLzo() const;

    Q_PROPERTY(bool protoTcp READ protoTcp WRITE setProtoTcp NOTIFY protoTcpChanged)
    bool protoTcp() const;

    Q_PROPERTY(QString dev READ dev WRITE setDev NOTIFY devChanged)
    QString dev() const;

    Q_PROPERTY(DevType devType READ devType WRITE setDevType NOTIFY devTypeChanged)
    DevType devType() const;

    Q_PROPERTY(bool devTypeSet READ devTypeSet WRITE setDevTypeSet NOTIFY devTypeSetChanged)
    bool devTypeSet() const;

    Q_PROPERTY(int tunnelMtu READ tunnelMtu WRITE setTunnelMtu NOTIFY tunnelMtuChanged)
    int tunnelMtu() const;

    Q_PROPERTY(bool tunnelMtuSet READ tunnelMtuSet WRITE setTunnelMtuSet NOTIFY tunnelMtuSetChanged)
    bool tunnelMtuSet() const;

    Q_PROPERTY(int fragmentSize READ fragmentSize WRITE setFragmentSize NOTIFY fragmentSizeChanged)
    int fragmentSize() const;

    Q_PROPERTY(bool fragmentSizeSet READ fragmentSizeSet WRITE setFragmentSizeSet NOTIFY fragmentSizeSetChanged)
    bool fragmentSizeSet() const;

    Q_PROPERTY(bool mssFix READ mssFix WRITE setMssFix NOTIFY mssFixChanged)
    bool mssFix() const;

    Q_PROPERTY(bool remoteRandom READ remoteRandom WRITE setRemoteRandom NOTIFY remoteRandomChanged)
    bool remoteRandom() const;

    // Advanced security properties

    Q_PROPERTY(Cipher cipher READ cipher WRITE setCipher NOTIFY cipherChanged)
    Cipher cipher() const;

    Q_PROPERTY(int keysize READ keysize WRITE setKeysize NOTIFY keysizeChanged)
    int keysize() const;

    Q_PROPERTY(bool keysizeSet READ keysizeSet WRITE setKeysizeSet NOTIFY keysizeSetChanged)
    bool keysizeSet() const;

    Q_PROPERTY(Auth auth READ auth WRITE setAuth NOTIFY authChanged)
    Auth auth() const;

    // Advanced TLS auth properties

    Q_PROPERTY(QString tlsRemote READ tlsRemote WRITE setTlsRemote NOTIFY tlsRemoteChanged)
    QString tlsRemote() const;

    Q_PROPERTY(TlsType remoteCertTls READ remoteCertTls WRITE setRemoteCertTls NOTIFY remoteCertTlsChanged)
    TlsType remoteCertTls() const;

    Q_PROPERTY(bool remoteCertTlsSet READ remoteCertTlsSet WRITE setRemoteCertTlsSet NOTIFY remoteCertTlsSetChanged)
    bool remoteCertTlsSet() const;

    Q_PROPERTY(QString ta READ ta WRITE setTa NOTIFY taChanged)
    QString ta() const;

    Q_PROPERTY(KeyDir taDir READ taDir WRITE setTaDir NOTIFY taDirChanged)
    KeyDir taDir() const;

    Q_PROPERTY(bool taSet READ taSet WRITE setTaSet NOTIFY taSetChanged)
    bool taSet() const;

    // Advanced proxy settings

    Q_PROPERTY(ProxyType proxyType READ proxyType WRITE setProxyType NOTIFY proxyTypeChanged)
    ProxyType proxyType() const;

    Q_PROPERTY(QString proxyServer READ proxyServer WRITE setProxyServer NOTIFY proxyServerChanged)
    QString proxyServer() const;

    Q_PROPERTY(int proxyPort READ proxyPort WRITE setProxyPort NOTIFY proxyPortChanged)
    int proxyPort() const;

    Q_PROPERTY(bool proxyRetry READ proxyRetry WRITE setProxyRetry NOTIFY proxyRetryChanged)
    bool proxyRetry() const;

    Q_PROPERTY(QString proxyUsername READ proxyUsername WRITE setProxyUsername NOTIFY proxyUsernameChanged)
    QString proxyUsername() const;

    Q_PROPERTY(QString proxyPassword READ proxyPassword WRITE setProxyPassword NOTIFY proxyPasswordChanged)
    QString proxyPassword() const;

public Q_SLOTS:
    // Basic properties

    void setCa(const QString &value);

    void setCert(const QString &value);

    void setCertPass(const QString &value);

    void setConnectionType(ConnectionType connectionType);

    void setKey(const QString &value);

    void setLocalIp(const QString &value);

    void setPassword(const QString &value);

    void setRemote(const QString &value);

    void setRemoteIp(const QString &value);

    void setStaticKey(const QString &value);

    void setStaticKeyDirection(KeyDir value);

    void setUsername(const QString &value);

    // Advanced general properties

    void setPort(int value);

    void setPortSet(bool value);

    void setRenegSeconds(int value);

    void setRenegSecondsSet(bool value);

    void setCompLzo(bool value);

    void setProtoTcp(bool value);

    void setDev(const QString &value);

    void setDevType(DevType value);

    void setDevTypeSet(bool value);

    void setTunnelMtu(int value);

    void setTunnelMtuSet(bool value);

    void setFragmentSize(int value);

    void setFragmentSizeSet(bool value);

    void setMssFix(bool value);

    void setRemoteRandom(bool value);

    // Advanced security properties

    void setCipher(Cipher value);

    void setKeysize(int value);

    void setKeysizeSet(bool value);

    void setAuth(Auth value);

    // Advanced TLS auth properties

    void setTlsRemote(const QString &value);

    void setRemoteCertTls(TlsType value);

    void setRemoteCertTlsSet(bool value);

    void setTa(const QString &value);

    void setTaDir(KeyDir value);

    void setTaSet(bool value);

    // Advanced proxy settings

    void setProxyType(ProxyType value);

    void setProxyServer(const QString &value);

    void setProxyPort(int value);

    void setProxyRetry(bool value);

    void setProxyUsername(const QString &value);

    void setProxyPassword(const QString &value);

Q_SIGNALS:
    // Basic properties

    void caChanged(const QString &value);

    void certChanged(const QString &value);

    void certPassChanged(const QString &value);

    void connectionTypeChanged(ConnectionType connectionType);

    void keyChanged(const QString &value);

    void localIpChanged(const QString &value);

    void passwordChanged(const QString &value);

    void remoteChanged(const QString &value);

    void remoteIpChanged(const QString &value);

    void staticKeyChanged(const QString &value);

    void staticKeyDirectionChanged(KeyDir value);

    void usernameChanged(const QString &value);

    // Advanced general properties

    void portChanged(int value);

    void portSetChanged(bool value);

    void renegSecondsChanged(int value);

    void renegSecondsSetChanged(bool value);

    void compLzoChanged(bool value);

    void protoTcpChanged(bool value);

    void devChanged(const QString &value);

    void devTypeChanged(DevType value);

    void devTypeSetChanged(bool value);

    void tunnelMtuChanged(int value);

    void tunnelMtuSetChanged(bool value);

    void fragmentSizeChanged(int value);

    void fragmentSizeSetChanged(bool value);

    void mssFixChanged(bool value);

    void remoteRandomChanged(bool value);

    // Advanced security properties

    void cipherChanged(Cipher value);

    void keysizeChanged(int value);

    void keysizeSetChanged(bool value);

    void authChanged(Auth value);

    // Advanced TLS auth properties

    void tlsRemoteChanged(const QString &value);

    void remoteCertTlsChanged(TlsType value);

    void remoteCertTlsSetChanged(bool value);

    void taChanged(const QString &value);

    void taDirChanged(KeyDir value);

    void taSetChanged(bool value);

    // Advanced proxy tings

    void proxyTypeChanged(ProxyType value);

    void proxyServerChanged(const QString &value);

    void proxyPortChanged(int value);

    void proxyRetryChanged(bool value);

    void proxyUsernameChanged(const QString &value);

    void proxyPasswordChanged(const QString &value);

protected:
    class Priv;
    std::shared_ptr<Priv> d;
};

}
