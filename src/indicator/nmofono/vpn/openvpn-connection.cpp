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

#include <nmofono/vpn/openvpn-connection.h>

#include <NetworkManagerSettingsConnectionInterface.h>

using namespace std;

#define DEFINE_UPDATE_SETTER(name, uppername, type)\
void set##uppername(type value)\
{\
    if (m_data.m_##name == value)\
    {\
        return;\
    }\
    m_data.m_##name = value;\
    Q_EMIT p.name##Changed(m_data.m_##name);\
}

#define DEFINE_UPDATE_PROPERTY_STRING(name, uppername, strname) \
void update##uppername(const QStringMap& data)\
{\
    set##uppername(data.value(strname));\
}\
\
DEFINE_UPDATE_SETTER(name, uppername, const QString &)

#define DEFINE_UPDATE_PROPERTY_BOOL(name, uppername, strname) \
void update##uppername(const QStringMap& data)\
{\
    set##uppername(data.value(strname) == "yes");\
}\
\
DEFINE_UPDATE_SETTER(name, uppername, bool)

#define DEFINE_UPDATE_PROPERTY_PAIR(name, uppername, strname, type, conversion)\
void update##uppername(const QStringMap& data)\
{\
    auto it = data.constFind(strname);\
    bool found = (it != data.constEnd());\
    set##uppername##Set(found);\
    if (found)\
    {\
        set##uppername(it->conversion);\
    }\
}\
\
DEFINE_UPDATE_SETTER(name, uppername, type)\
\
DEFINE_UPDATE_SETTER(name##Set, uppername##Set, bool)\

#define DEFINE_PROPERTY_GETTER(name,type) \
type OpenvpnConnection::name() const\
{\
    return d->m_data.m_##name;\
}\

#define DEFINE_PROPERTY_SETTER(varname, uppername, type) \
void OpenvpnConnection::set##uppername(type value)\
{\
    if (d->m_data.m_##varname == value)\
    {\
        return;\
    }\
    if (!d->m_dirty)\
    {\
        d->m_pendingData = d->m_data;\
    }\
    d->m_dirty = true;\
    d->m_pendingData.m_##varname = value;\
    Q_EMIT updateVpnData(d->m_pendingData.buildData());\
}

#define DEFINE_SECRET_PROPERTY_SETTER(varname, uppername, type) \
void OpenvpnConnection::set##uppername(type value)\
{\
    if (d->m_data.m_##varname == value)\
    {\
        return;\
    }\
    if (!d->m_dirty)\
    {\
        d->m_pendingData = d->m_data;\
    }\
    d->m_dirty = true;\
    d->m_pendingData.m_##varname = value;\
    Q_EMIT updateVpnSecrets(d->m_pendingData.buildSecrets());\
}

namespace nmofono
{
namespace vpn
{

class OpenvpnConnection::Priv
{

public:
    struct Data
    {
        Data()
        {
        }

        Data(const Data& other) = delete;

        Data& operator=(const Data& other)
        {
            // Basic properties

            m_ca = other.m_ca;
            m_cert = other.m_cert;
            m_certPass = other.m_certPass;
            m_connectionType = other.m_connectionType;
            m_key = other.m_key;
            m_localIp = other.m_localIp;
            m_password = other.m_password;
            m_remote = other.m_remote;
            m_remoteIp = other.m_remoteIp;
            m_staticKey = other.m_staticKey;
            m_staticKeyDirection = other.m_staticKeyDirection;
            m_username = other.m_username;

            // Advanced general properties

            m_port = other.m_port;
            m_portSet = other.m_portSet;
            m_renegSeconds = other.m_renegSeconds;
            m_renegSecondsSet = other.m_renegSecondsSet;
            m_compLzo = other.m_compLzo;
            m_protoTcp = other.m_protoTcp;
            m_dev = other.m_dev;
            m_devType = other.m_devType;
            m_devTypeSet = other.m_devTypeSet;
            m_tunnelMtu = other.m_tunnelMtu;
            m_tunnelMtuSet = other.m_tunnelMtuSet;
            m_fragmentSize = other.m_fragmentSize;
            m_fragmentSizeSet = other.m_fragmentSizeSet;
            m_mssFix = other.m_mssFix;
            m_remoteRandom = other.m_remoteRandom;

            // Advanced security properties

            m_cipher = other.m_cipher;
            m_keysize = other.m_keysize;
            m_keysizeSet = other.m_keysizeSet;
            m_auth = other.m_auth;

            // Advanced TLS auth properties

            m_tlsRemote = other.m_tlsRemote;
            m_remoteCertTls = other.m_remoteCertTls;
            m_remoteCertTlsSet = other.m_remoteCertTlsSet;
            m_ta = other.m_ta;
            m_taDir = other.m_taDir;
            m_taSet = other.m_taSet;

            // Advanced proxy settings

            m_proxyType = other.m_proxyType;
            m_proxyServer = other.m_proxyServer;
            m_proxyPort = other.m_proxyPort;
            m_proxyRetry = other.m_proxyRetry;
            m_proxyUsername = other.m_proxyUsername;
            m_proxyPassword = other.m_proxyPassword;

            return *this;
        }

        QStringMap buildSecrets()
        {
            QStringMap secrets;

            switch(m_connectionType)
            {
               case ConnectionType::TLS:
                   secrets["cert-pass"] = m_certPass;
                   break;
               case ConnectionType::PASSWORD:
                   secrets["password"] = m_password;
                   break;
               case ConnectionType::PASSWORD_TLS:
                   secrets["cert-pass"] = m_certPass;
                   secrets["password"] = m_password;
                   break;
               case ConnectionType::STATIC_KEY:
                   break;
            }

            if (!m_proxyPassword.isEmpty())
            {
                switch (m_proxyType)
                {
                    case ProxyType::NOT_REQUIRED:
                        break;
                    case ProxyType::HTTP:
                        secrets["http-proxy-password"] = m_proxyPassword;
                        break;
                    case ProxyType::SOCKS:
                        secrets["socks-proxy-password"] = m_proxyPassword;
                        break;
                }
            }

            return secrets;
        }

        QStringMap buildData()
        {
            QStringMap data;

            static const QMap<KeyDir, QString> keyDirMap
            {
                {ZERO, "0"},
                {ONE, "1"}
            };

            // Basic properties

            static const QMap<ConnectionType, QString> connectionTypeMap
            {
                {ConnectionType::TLS, "tls"},
                {ConnectionType::PASSWORD, "password"},
                {ConnectionType::PASSWORD_TLS, "password-tls"},
                {ConnectionType::STATIC_KEY, "static-key"}
            };
            data["connection-type"] = connectionTypeMap[m_connectionType];

            switch (m_connectionType)
            {
                case ConnectionType::TLS:
                    data["ca"] = m_ca;
                    data["cert"] = m_cert;
                    data["cert-pass-flags"] = "1";
                    data["key"] = m_key;
                    data["remote"] = m_remote;
                    break;
                case ConnectionType::PASSWORD:
                    data["ca"] = m_ca;
                    data["username"] = m_username;
                    data["password-flags"] = "1";
                    data["remote"] = m_remote;
                    break;
                case ConnectionType::PASSWORD_TLS:
                    data["ca"] = m_ca;
                    data["cert"] = m_cert;
                    data["cert-pass-flags"] = "1";
                    data["key"] = m_key;
                    data["username"] = m_username;
                    data["password-flags"] = "1";
                    data["remote"] = m_remote;
                    break;
                case ConnectionType::STATIC_KEY:
                    data["ca"] = m_ca;
                    data["local-ip"] = m_localIp;
                    data["remote"] = m_remote;
                    data["remote-ip"] = m_remoteIp;
                    data["static-key"] = m_staticKey;
                    if (m_staticKeyDirection != KeyDir::KEY_NONE)
                    {
                        data["static-key-direction"] = keyDirMap[m_staticKeyDirection];
                    }
                    break;
            }

            // Advanced general properties

            if (m_portSet)
            {
                data["port"] = QString::number(m_port);
            }
            if (m_renegSecondsSet)
            {
                data["reneg-seconds"] = QString::number(m_renegSeconds);
            }
            if (m_compLzo)
            {
                data["comp-lzo"] = "yes";
            }
            if (m_protoTcp)
            {
                data["proto-tcp"] = "yes";
            }
            static const QMap<DevType, QString> devTypeMap
            {
                {DevType::TUN, "tun"},
                {DevType::TAP, "tap"}
            };
            if (m_devTypeSet)
            {
                data["dev-type"] = devTypeMap[m_devType];
                if (!m_dev.isEmpty())
                {
                    data["dev"] = m_dev;
                }
            }
            if (m_tunnelMtuSet)
            {
                data["tunnel-mtu"] = QString::number(m_tunnelMtu);
            }
            if (m_fragmentSizeSet)
            {
                data["fragment-size"] = QString::number(m_fragmentSize);
            }
            if (m_mssFix)
            {
                data["mssfix"] = "yes";
            }
            if (m_remoteRandom)
            {
                data["remote-random"] = "yes";
            }

            // Advanced security properties

            static const QMap<Cipher, QString> cipherMap
            {
                {Cipher::DES_CBC, "DES-CBC"},
                {Cipher::RC2_CBC, "RC2-CBC"},
                {Cipher::DES_EDE_CBC, "DES-EDE-CBC"},
                {Cipher::DES_EDE3_CBC, "DES-EDE3-CBC"},
                {Cipher::DESX_CBC, "DESX-CBC"},
                {Cipher::RC2_40_CBC, "RC2-40-CBC"},
                {Cipher::CAST5_CBC, "CAST5-CBC"},
                {Cipher::AES_128_CBC, "AES-128-CBC"},
                {Cipher::AES_192_CBC, "AES-192-CBC"},
                {Cipher::AES_256_CBC, "AES-256-CBC"},
                {Cipher::CAMELLIA_128_CBC, "CAMELLIA-128-CBC"},
                {Cipher::CAMELLIA_192_CBC, "CAMELLIA-192-CBC"},
                {Cipher::CAMELLIA_256_CBC, "CAMELLIA-256-CBC",},
                {Cipher::SEED_CBC, "SEED-CBC"},
                {Cipher::AES_128_CBC_HMAC_SHA1, "AES-128-CBC-HMAC-SHA1"},
                {Cipher::AES_256_CBC_HMAC_SHA1, "AES-256-CBC-HMAC-SHA1"},
                {Cipher::NO_CIPHER, "none"}
            };
            if (m_cipher != Cipher::DEFAULT_CIPHER)
            {
                data["cipher"] = cipherMap[m_cipher];
            }
            if (m_keysizeSet)
            {
                data["keysize"] = QString::number(m_keysize);
            }
            static const QMap<Auth, QString> authMap
            {
                {NONE, "none"},
                {RSA_MD4, "RSA-MD4"},
                {MD5, "MD5"},
                {SHA1, "SHA1"},
                {SHA224, "SHA224"},
                {SHA256, "SHA256"},
                {SHA384, "SHA384"},
                {SHA512, "SHA512"},
                {RIPEMD160, "RIPEMD160"}
            };
            if (m_auth != Auth::DEFAULT_AUTH)
            {
                data["auth"] = authMap[m_auth];
            }

            // Advanced TLS auth properties

            if (m_connectionType != ConnectionType::STATIC_KEY)
            {
                if (!m_tlsRemote.isEmpty())
                {
                    data["tls-remote"] = m_tlsRemote;
                }
                static const QMap<TlsType, QString> remoteCertTlsMap
                {
                    {SERVER, "server"},
                    {CLIENT, "client"}
                };
                if (m_remoteCertTlsSet)
                {
                    data["remote-cert-tls"] = remoteCertTlsMap[m_remoteCertTls];
                }

                if (m_taSet)
                {
                    if (m_taDir != KeyDir::KEY_NONE)
                    {
                        data["ta-dir"] = keyDirMap[m_taDir];
                    }
                    data["ta"] = (m_ta.isEmpty() ? "/" : m_ta);
                }
            }

            // Advanced proxy settings

            static const QMap<ProxyType, QString> proxyTypeMap
            {
                {HTTP, "http"},
                {SOCKS, "socks"}
            };
            if (m_proxyType != ProxyType::NOT_REQUIRED)
            {
                data["proxy-type"] = proxyTypeMap[m_proxyType];
                data["proxy-server"] = m_proxyServer;
                data["proxy-port"] = QString::number(m_proxyPort);
                if (m_proxyRetry)
                {
                    data["proxy-retry"] = "yes";
                }
                if (!m_proxyUsername.isEmpty())
                {
                    switch (m_proxyType)
                    {
                        case ProxyType::NOT_REQUIRED:
                            break;
                        case ProxyType::HTTP:
                            data["http-proxy-username"] = m_proxyUsername;
                            break;
                        case ProxyType::SOCKS:
                            data["socks-proxy-username"] = m_proxyUsername;
                            break;
                    }
                }
            }

            return data;
        }

        // Basic properties

        QString m_ca;
        QString m_cert;
        QString m_certPass;
        ConnectionType m_connectionType = ConnectionType::TLS;
        QString m_key;
        QString m_localIp;
        QString m_password;
        QString m_remote;
        QString m_remoteIp;
        QString m_staticKey;
        KeyDir m_staticKeyDirection = KeyDir::KEY_NONE;
        QString m_username;

        // Advanced general properties

        int m_port = 1194;
        bool m_portSet = false;
        int m_renegSeconds = 0;
        bool m_renegSecondsSet = false;
        bool m_compLzo = false;
        bool m_protoTcp = false;
        QString m_dev;
        DevType m_devType = DevType::TUN;
        bool m_devTypeSet = false;
        int m_tunnelMtu = 1500;
        bool m_tunnelMtuSet = false;
        int m_fragmentSize = 1300;
        bool m_fragmentSizeSet = false;
        bool m_mssFix = false;
        bool m_remoteRandom = false;

        // Advanced security properties

        Cipher m_cipher = Cipher::DEFAULT_CIPHER;
        int m_keysize = 128;
        bool m_keysizeSet = false;
        Auth m_auth = Auth::DEFAULT_AUTH;

        // Advanced TLS auth properties

        QString m_tlsRemote;
        TlsType m_remoteCertTls = TlsType::SERVER;
        bool m_remoteCertTlsSet = false;
        QString m_ta;
        KeyDir m_taDir = KeyDir::KEY_NONE;
        bool m_taSet = false;

        // Advanced proxy settings

        ProxyType m_proxyType = ProxyType::NOT_REQUIRED;
        QString m_proxyServer;
        int m_proxyPort = 80;
        bool m_proxyRetry = false;
        QString m_proxyUsername;
        QString m_proxyPassword;
    };

    Priv(OpenvpnConnection& parent) :
        p(parent)
    {
    }

    // Basic properties

    DEFINE_UPDATE_PROPERTY_STRING(ca, Ca, "ca")
    DEFINE_UPDATE_PROPERTY_STRING(cert, Cert, "cert")
    DEFINE_UPDATE_PROPERTY_STRING(certPass, CertPass, "cert-pass")

    void updateConnectionType(const QStringMap& data)
    {
        static const QMap<QString, ConnectionType> typeMap
        {
            {"tls", ConnectionType::TLS},
            {"password", ConnectionType::PASSWORD},
            {"password-tls", ConnectionType::PASSWORD_TLS},
            {"static-key", ConnectionType::STATIC_KEY}
        };

        setConnectionType(typeMap.value(data["connection-type"], ConnectionType::TLS));
    }
    DEFINE_UPDATE_SETTER(connectionType, ConnectionType, OpenvpnConnection::ConnectionType)

    DEFINE_UPDATE_PROPERTY_STRING(key, Key, "key")
    DEFINE_UPDATE_PROPERTY_STRING(localIp, LocalIp, "local-ip")
    DEFINE_UPDATE_PROPERTY_STRING(password, Password, "password")
    DEFINE_UPDATE_PROPERTY_STRING(remote, Remote, "remote")
    DEFINE_UPDATE_PROPERTY_STRING(remoteIp, RemoteIp, "remote-ip")
    DEFINE_UPDATE_PROPERTY_STRING(staticKey, StaticKey, "static-key")

    void updateStaticKeyDirection(const QStringMap& data)
    {
        static const QMap<QString, KeyDir> typeMap
        {
            {"0", KeyDir::ZERO},
            {"1", KeyDir::ONE}
        };

        auto it = data.constFind("static-key-direction");
        if (it != data.constEnd())
        {
            setStaticKeyDirection(typeMap.value(*it, KeyDir::KEY_NONE));
        }
        else
        {
            setStaticKeyDirection(KeyDir::KEY_NONE);
        }
    }
    DEFINE_UPDATE_SETTER(staticKeyDirection, StaticKeyDirection, OpenvpnConnection::KeyDir)

    DEFINE_UPDATE_PROPERTY_STRING(username, Username, "username")

    // Advanced general properties

    DEFINE_UPDATE_PROPERTY_PAIR(port, Port, "port", int, toInt())
    DEFINE_UPDATE_PROPERTY_PAIR(renegSeconds, RenegSeconds, "reneg-seconds", int, toInt())
    DEFINE_UPDATE_PROPERTY_BOOL(compLzo, CompLzo, "comp-lzo")
    DEFINE_UPDATE_PROPERTY_BOOL(protoTcp, ProtoTcp, "proto-tcp")

    void updateDevType(const QStringMap& data)
    {
        static const QMap<QString, DevType> typeMap
        {
            {"tun", DevType::TUN},
            {"tap", DevType::TAP}
        };

        auto it = data.constFind("dev-type");
        bool found = (it != data.constEnd());
        setDevTypeSet(found);
        if (found)
        {
            setDevType(typeMap.value(*it, DevType::TUN));
            setDev(data.value("dev", QString()));
        }
    }
    DEFINE_UPDATE_SETTER(dev, Dev, const QString &)
    DEFINE_UPDATE_SETTER(devType, DevType, OpenvpnConnection::DevType)
    DEFINE_UPDATE_SETTER(devTypeSet, DevTypeSet, bool)

    DEFINE_UPDATE_PROPERTY_PAIR(tunnelMtu, TunnelMtu, "tunnel-mtu", int, toInt())
    DEFINE_UPDATE_PROPERTY_PAIR(fragmentSize, FragmentSize, "fragment-size", int, toInt())
    DEFINE_UPDATE_PROPERTY_BOOL(mssFix, MssFix, "mssfix")
    DEFINE_UPDATE_PROPERTY_BOOL(remoteRandom, RemoteRandom, "remote-random")

    // Advanced security properties

    void updateCipher(const QStringMap& data)
    {
        static const QMap<QString, Cipher> typeMap
        {
            {"DES-CBC", Cipher::DES_CBC},
            {"RC2-CBC", Cipher::RC2_CBC},
            {"DES-EDE-CBC", Cipher::DES_EDE_CBC},
            {"DES-EDE3-CBC", Cipher::DES_EDE3_CBC},
            {"DESX-CBC", Cipher::DESX_CBC},
            {"RC2-40-CBC", Cipher::RC2_40_CBC},
            {"CAST5-CBC", Cipher::CAST5_CBC},
            {"AES-128-CBC", Cipher::AES_128_CBC},
            {"AES-192-CBC", Cipher::AES_192_CBC},
            {"AES-256-CBC", Cipher::AES_256_CBC},
            {"CAMELLIA-128-CBC", Cipher::CAMELLIA_128_CBC},
            {"CAMELLIA-192-CBC", Cipher::CAMELLIA_192_CBC},
            {"CAMELLIA-256-CBC", Cipher::CAMELLIA_256_CBC},
            {"SEED-CBC", Cipher::SEED_CBC},
            {"AES-128-CBC-HMAC-SHA1", Cipher::AES_128_CBC_HMAC_SHA1},
            {"AES-256-CBC-HMAC-SHA1", Cipher::AES_256_CBC_HMAC_SHA1},
            {"none", Cipher::NO_CIPHER}
        };

        auto it = data.constFind("cipher");
        bool found = (it != data.constEnd());
        if (found)
        {
            setCipher(typeMap.value(*it, Cipher::DEFAULT_CIPHER));
        }
        else
        {
            setCipher(Cipher::DEFAULT_CIPHER);
        }
    }
    DEFINE_UPDATE_SETTER(cipher, Cipher, OpenvpnConnection::Cipher)

    DEFINE_UPDATE_PROPERTY_PAIR(keysize, Keysize, "keysize", int, toInt())

    void updateAuth(const QStringMap& data)
    {
        static const QMap<QString, Auth> typeMap
        {
            {"none", NONE},
            {"RSA-MD4", RSA_MD4},
            {"MD5", MD5},
            {"SHA1", SHA1},
            {"SHA224", SHA224},
            {"SHA256", SHA256},
            {"SHA384", SHA384},
            {"SHA512", SHA512},
            {"RIPEMD160", RIPEMD160}
        };

        auto it = data.constFind("auth");
        bool found = (it != data.constEnd());
        if (found)
        {
            setAuth(typeMap.value(*it, Auth::DEFAULT_AUTH));
        }
        else
        {
            setAuth(Auth::DEFAULT_AUTH);
        }
    }
    DEFINE_UPDATE_SETTER(auth, Auth, OpenvpnConnection::Auth)

    // Advanced TLS auth properties

    DEFINE_UPDATE_PROPERTY_STRING(tlsRemote, TlsRemote, "tls-remote")

    void updateRemoteCertTls(const QStringMap& data)
    {
        static const QMap<QString, TlsType> typeMap
        {
            {"server", TlsType::SERVER},
            {"client", TlsType::CLIENT}
        };

        auto it = data.constFind("remote-cert-tls");
        bool found = (it != data.constEnd());
        setRemoteCertTlsSet(found);
        if (found)
        {
            setRemoteCertTls(typeMap.value(*it, TlsType::SERVER));
        }
    }
    DEFINE_UPDATE_SETTER(remoteCertTls, RemoteCertTls, OpenvpnConnection::TlsType)
    DEFINE_UPDATE_SETTER(remoteCertTlsSet, RemoteCertTlsSet, bool)

    void updateTa(const QStringMap& data)
    {
        static const QMap<QString, KeyDir> typeMap
        {
            {"0", KeyDir::ZERO},
            {"1", KeyDir::ONE}
        };

        auto it = data.constFind("ta");
        bool found = (it != data.constEnd());
        setTaSet(found);
        if (found)
        {
            setTa(*it);
            setTaDir(typeMap.value(data.value("ta-dir"), KeyDir::KEY_NONE));
        }
    }
    DEFINE_UPDATE_SETTER(ta, Ta, const QString &)
    DEFINE_UPDATE_SETTER(taDir, TaDir, OpenvpnConnection::KeyDir)
    DEFINE_UPDATE_SETTER(taSet, TaSet, bool)

    // Advanced proxy settings

    void updateProxy(const QStringMap& data)
    {
        static const QMap<QString, ProxyType> typeMap
        {
            {"http", ProxyType::HTTP},
            {"socks", ProxyType::SOCKS}
        };

        auto it = data.constFind("proxy-type");
        bool found = (it != data.constEnd());
        if (found)
        {
            setProxyType(typeMap.value(*it, ProxyType::NOT_REQUIRED));
            updateProxyServer(data);
            auto portIt = data.constFind("proxy-port");
            if (portIt != data.constEnd())
            {
                setProxyPort(portIt->toInt());
            }
            else
            {
               setProxyPort(0);
            }
            updateProxyRetry(data);

            switch (m_data.m_proxyType)
            {
                case ProxyType::NOT_REQUIRED:
                    break;
                case ProxyType::HTTP:
                    setProxyUsername(data.value("http-proxy-username"));
                    break;
                case ProxyType::SOCKS:
                    setProxyUsername(data.value("socks-proxy-username"));
                    break;
            }
        }
        else
        {
            setProxyType(ProxyType::NOT_REQUIRED);
        }
    }
    DEFINE_UPDATE_SETTER(proxyType, ProxyType, OpenvpnConnection::ProxyType)
    DEFINE_UPDATE_PROPERTY_STRING(proxyServer, ProxyServer, "proxy-server")
    DEFINE_UPDATE_SETTER(proxyPort, ProxyPort, int)
    DEFINE_UPDATE_PROPERTY_BOOL(proxyRetry, ProxyRetry, "proxy-retry")
    DEFINE_UPDATE_SETTER(proxyUsername, ProxyUsername, const QString &)
    DEFINE_UPDATE_SETTER(proxyPassword, ProxyPassword, const QString &)

    void updateProxySecrets(const QStringMap& data)
    {
        switch (m_data.m_proxyType)
        {
            case ProxyType::NOT_REQUIRED:
                break;
            case ProxyType::HTTP:
                setProxyPassword(data.value("http-proxy-password"));
                break;
            case ProxyType::SOCKS:
                setProxyPassword(data.value("socks-proxy-password"));
                break;
        }
    }

public:

    OpenvpnConnection& p;

    Data m_data;

    Data m_pendingData;

    bool m_dirty = false;
};

OpenvpnConnection::OpenvpnConnection() :
        d(new Priv(*this))
{
}

OpenvpnConnection::~OpenvpnConnection()
{
}

void OpenvpnConnection::updateData(const QStringMap& data)
{
    // Basic properties

    d->updateCa(data);
    d->updateCert(data);
    d->updateConnectionType(data);
    d->updateKey(data);
    d->updateLocalIp(data);
    d->updateRemote(data);
    d->updateRemoteIp(data);
    d->updateStaticKey(data);
    d->updateStaticKeyDirection(data);
    d->updateUsername(data);

    // Advanced general properties

    d->updatePort(data);
    d->updateRenegSeconds(data);
    d->updateCompLzo(data);
    d->updateProtoTcp(data);
    d->updateDevType(data);
    d->updateTunnelMtu(data);
    d->updateFragmentSize(data);
    d->updateMssFix(data);
    d->updateRemoteRandom(data);

    // Advanced security properties

    d->updateCipher(data);
    d->updateKeysize(data);
    d->updateAuth(data);

    // Advanced TLS auth properties

    d->updateTlsRemote(data);
    d->updateRemoteCertTls(data);
    d->updateTa(data);

    // Advanced proxy settings

    d->updateProxy(data);
}

void OpenvpnConnection::updateSecrets(const QStringMap& secrets)
{
    d->updateCertPass(secrets);
    d->updatePassword(secrets);
    d->updateProxySecrets(secrets);
}

void OpenvpnConnection::markClean()
{
    d->m_dirty = false;
}

// Basic properties

DEFINE_PROPERTY_GETTER(ca, QString)
DEFINE_PROPERTY_GETTER(cert, QString)
DEFINE_PROPERTY_GETTER(connectionType, OpenvpnConnection::ConnectionType)
DEFINE_PROPERTY_GETTER(key, QString)
DEFINE_PROPERTY_GETTER(localIp, QString)
DEFINE_PROPERTY_GETTER(password, QString)
DEFINE_PROPERTY_GETTER(certPass, QString)
DEFINE_PROPERTY_GETTER(remote, QString)
DEFINE_PROPERTY_GETTER(remoteIp, QString)
DEFINE_PROPERTY_GETTER(staticKey, QString)
DEFINE_PROPERTY_GETTER(staticKeyDirection, OpenvpnConnection::KeyDir)
DEFINE_PROPERTY_GETTER(username, QString)

// Advanced general properties

DEFINE_PROPERTY_GETTER(port, int)
DEFINE_PROPERTY_GETTER(portSet, bool)
DEFINE_PROPERTY_GETTER(renegSeconds, int)
DEFINE_PROPERTY_GETTER(renegSecondsSet, bool)
DEFINE_PROPERTY_GETTER(compLzo, bool)
DEFINE_PROPERTY_GETTER(protoTcp, bool)
DEFINE_PROPERTY_GETTER(dev, QString)
DEFINE_PROPERTY_GETTER(devType, OpenvpnConnection::DevType)
DEFINE_PROPERTY_GETTER(devTypeSet, bool)
DEFINE_PROPERTY_GETTER(tunnelMtu, int)
DEFINE_PROPERTY_GETTER(tunnelMtuSet, bool)
DEFINE_PROPERTY_GETTER(fragmentSize, int)
DEFINE_PROPERTY_GETTER(fragmentSizeSet, bool)
DEFINE_PROPERTY_GETTER(mssFix, bool)
DEFINE_PROPERTY_GETTER(remoteRandom, bool)

// Advanced security properties

DEFINE_PROPERTY_GETTER(cipher, OpenvpnConnection::Cipher)
DEFINE_PROPERTY_GETTER(keysize, int)
DEFINE_PROPERTY_GETTER(keysizeSet, bool)
DEFINE_PROPERTY_GETTER(auth, OpenvpnConnection::Auth)

// Advanced TLS auth properties

DEFINE_PROPERTY_GETTER(tlsRemote, QString)
DEFINE_PROPERTY_GETTER(remoteCertTls, OpenvpnConnection::TlsType)
DEFINE_PROPERTY_GETTER(remoteCertTlsSet, bool)
DEFINE_PROPERTY_GETTER(ta, QString)
DEFINE_PROPERTY_GETTER(taDir, OpenvpnConnection::KeyDir)
DEFINE_PROPERTY_GETTER(taSet, bool)

// Advanced proxy settings

DEFINE_PROPERTY_GETTER(proxyType, OpenvpnConnection::ProxyType)
DEFINE_PROPERTY_GETTER(proxyServer, QString)
DEFINE_PROPERTY_GETTER(proxyPort, int)
DEFINE_PROPERTY_GETTER(proxyRetry, bool)
DEFINE_PROPERTY_GETTER(proxyUsername, QString)
DEFINE_PROPERTY_GETTER(proxyPassword, QString)




// Basic properties

DEFINE_PROPERTY_SETTER(ca, Ca, const QString &)
DEFINE_PROPERTY_SETTER(cert, Cert, const QString &)
DEFINE_SECRET_PROPERTY_SETTER(certPass, CertPass, const QString &)
DEFINE_PROPERTY_SETTER(connectionType, ConnectionType, ConnectionType)
DEFINE_PROPERTY_SETTER(key, Key, const QString &)
DEFINE_PROPERTY_SETTER(localIp, LocalIp, const QString &)
DEFINE_SECRET_PROPERTY_SETTER(password, Password, const QString &)
DEFINE_PROPERTY_SETTER(remote, Remote, const QString &)
DEFINE_PROPERTY_SETTER(remoteIp, RemoteIp, const QString &)
DEFINE_PROPERTY_SETTER(staticKey, StaticKey, const QString &)
DEFINE_PROPERTY_SETTER(staticKeyDirection, StaticKeyDirection, KeyDir)
DEFINE_PROPERTY_SETTER(username, Username, const QString &)

// Advanced general properties

DEFINE_PROPERTY_SETTER(port, Port, int)
DEFINE_PROPERTY_SETTER(portSet, PortSet, bool)
DEFINE_PROPERTY_SETTER(renegSeconds, RenegSeconds, int)
DEFINE_PROPERTY_SETTER(renegSecondsSet, RenegSecondsSet, bool)
DEFINE_PROPERTY_SETTER(compLzo, CompLzo, bool)
DEFINE_PROPERTY_SETTER(protoTcp, ProtoTcp, bool)
DEFINE_PROPERTY_SETTER(dev, Dev, const QString &)
DEFINE_PROPERTY_SETTER(devType, DevType, OpenvpnConnection::DevType)
DEFINE_PROPERTY_SETTER(devTypeSet, DevTypeSet, bool)
DEFINE_PROPERTY_SETTER(tunnelMtu, TunnelMtu, int)
DEFINE_PROPERTY_SETTER(tunnelMtuSet, TunnelMtuSet, bool)
DEFINE_PROPERTY_SETTER(fragmentSize, FragmentSize, int)
DEFINE_PROPERTY_SETTER(fragmentSizeSet, FragmentSizeSet, bool)
DEFINE_PROPERTY_SETTER(mssFix, MssFix, bool)
DEFINE_PROPERTY_SETTER(remoteRandom, RemoteRandom, bool)

// Advanced security properties

DEFINE_PROPERTY_SETTER(cipher, Cipher, OpenvpnConnection::Cipher)
DEFINE_PROPERTY_SETTER(keysize, Keysize, int)
DEFINE_PROPERTY_SETTER(keysizeSet, KeysizeSet, bool)
DEFINE_PROPERTY_SETTER(auth, Auth, OpenvpnConnection::Auth)

// Advanced TLS auth properties

DEFINE_PROPERTY_SETTER(tlsRemote, TlsRemote, const QString &)
DEFINE_PROPERTY_SETTER(remoteCertTls, RemoteCertTls, OpenvpnConnection::TlsType)
DEFINE_PROPERTY_SETTER(remoteCertTlsSet, RemoteCertTlsSet, bool)
DEFINE_PROPERTY_SETTER(ta, Ta, const QString &)
DEFINE_PROPERTY_SETTER(taDir, TaDir, OpenvpnConnection::KeyDir)
DEFINE_PROPERTY_SETTER(taSet, TaSet, bool)

// Advanced proxy settings

DEFINE_PROPERTY_SETTER(proxyType, ProxyType, OpenvpnConnection::ProxyType)
DEFINE_PROPERTY_SETTER(proxyServer, ProxyServer, const QString &)
DEFINE_PROPERTY_SETTER(proxyPort, ProxyPort, int)
DEFINE_PROPERTY_SETTER(proxyRetry, ProxyRetry, bool)
DEFINE_PROPERTY_SETTER(proxyUsername, ProxyUsername, const QString &)
DEFINE_SECRET_PROPERTY_SETTER(proxyPassword, ProxyPassword, const QString &)

}
}
