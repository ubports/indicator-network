/*
 * Copyright (C) 2013 Canonical, Ltd.
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

#include <agent/SecretAgent.h>
#include <agent/SecretRequest.h>
#include <AgentManagerInterface.h>
#include <notify-cpp/notification-manager.h>
#include <SecretAgentAdaptor.h>

#include <NetworkManager.h>
#include <stdexcept>

#define NM_SECRET_AGENT_CAPABILITY_NONE 0

#define NM_SECRET_AGENT_GET_SECRETS_FLAG_NONE 0
#define NM_SECRET_AGENT_GET_SECRETS_FLAG_ALLOW_INTERACTION 1
#define NM_SECRET_AGENT_GET_SECRETS_FLAG_REQUEST_NEW 2
#define NM_SECRET_AGENT_GET_SECRETS_FLAG_USER_REQUESTED 4

using namespace std;

namespace agent
{

class SecretAgent::Priv : public QObject {
	Q_OBJECT

public:
	Priv(notify::NotificationManager::SPtr notificationManager,
			agent::CredentialStore::SPtr credentialStore,
			const QDBusConnection &systemConnection,
			const QDBusConnection &sessionConnection) :
			m_systemConnection(systemConnection),
			m_sessionConnection(sessionConnection),
			m_managerWatcher(NM_DBUS_SERVICE, m_systemConnection),
			m_agentManager(NM_DBUS_SERVICE, NM_DBUS_PATH_AGENT_MANAGER, m_systemConnection),
			m_notifications(notificationManager),
			m_credentialStore(credentialStore),
			m_request(nullptr) {
	}

	void saveSecret(const QString& id, const QString& uuid,
			const QString& settingName, const QString& settingKey,
			const QString& secret, const QString& inputDisplayName =
					QString()) {

		// TODO: Don't save always-ask or system-owned secrets

		QString displayName(inputDisplayName);
		if (displayName.isEmpty()) {
			static const QString DISPLAY_NAME("Network secret for %s/%s/%s");
			displayName = DISPLAY_NAME.arg(id, settingName, settingKey);
		}

		m_credentialStore->save(uuid, settingName, settingKey, displayName, secret);
	}

	static bool isSecret(const QString& settingName, const QString& key) {
		static const QMap<QString, QSet<QString>> KNOWN_SECRETS{
			{QString::fromUtf8(NM_802_1X_SETTING_NAME), {NM_802_1X_PASSWORD, NM_802_1X_PRIVATE_KEY_PASSWORD,
					NM_802_1X_PHASE2_PRIVATE_KEY_PASSWORD, NM_802_1X_PIN}},
			{QString::fromUtf8(NM_ADSL_SETTING_NAME), {NM_ADSL_PASSWORD}},
			{QString::fromUtf8(NM_CDMA_SETTING_NAME), {NM_CDMA_PASSWORD}},
			{QString::fromUtf8(NM_GSM_SETTING_NAME), {NM_GSM_PASSWORD, NM_GSM_PIN}},
			{QString::fromUtf8(NM_PPPOE_SETTING_NAME), {NM_PPPOE_PASSWORD}},
			{QString::fromUtf8(NM_WIRELESS_SECURITY_SETTING_NAME), {NM_WIRELESS_SECURITY_WEP_KEY0,
					NM_WIRELESS_SECURITY_WEP_KEY1, NM_WIRELESS_SECURITY_WEP_KEY2, NM_WIRELESS_SECURITY_WEP_KEY3,
					NM_WIRELESS_SECURITY_PSK, NM_WIRELESS_SECURITY_LEAP_PASSWORD}}
		};
		auto it = KNOWN_SECRETS.constFind(settingName);
		if (it != KNOWN_SECRETS.constEnd()) {
			if (it->contains(key)) {
				return true;
			}
		}
		return false;
	}

	void saveSettings(const QString& id, const QString& uuid,
			const QString& settingName, const QVariantMap& setting) {

		QMapIterator<QString, QVariant> iter(setting);
		while (iter.hasNext()) {
			iter.next();
			if (isSecret(settingName, iter.key())) {
				saveSecret(id, uuid, settingName, iter.key(),
						iter.value().toString());
			}
		}
	}

	void saveVpnSettings(const QString& id, const QString& uuid,
			const QString& settingName, const QVariantMap& setting) {
		static const QString DISPLAY_NAME{"VPN %1 secret for %2/%3/%4"};

		QString serviceType = setting[NM_VPN_SERVICE_TYPE].toString();
		QStringMap secrets;
		auto dbusArgument = qvariant_cast<QDBusArgument>(setting[NM_VPN_SECRETS]);
		dbusArgument >> secrets;
		QMapIterator<QString, QString> iter(secrets);
		while(iter.hasNext()) {
			iter.next();
			saveSecret(id, uuid, settingName, iter.key(), iter.value(),
					DISPLAY_NAME.arg(iter.key(), id, serviceType, NM_VPN_SETTING_NAME));
		}
	}

public Q_SLOTS:
	void serviceOwnerChanged(const QString &name, const QString &oldOwner,
			const QString &newOwner)
	{
		Q_UNUSED(name)
		Q_UNUSED(oldOwner)
		if (!newOwner.isEmpty()) {
			auto reply = m_agentManager.RegisterWithCapabilities(
					"com.canonical.indicator.SecretAgent",
					NM_SECRET_AGENT_CAPABILITY_NONE);
			reply.waitForFinished();
			if (reply.isError()) {
				qCritical() << reply.error().message();
			}
		}
	}

public:
	QDBusConnection m_systemConnection;

	QDBusConnection m_sessionConnection;

	QDBusServiceWatcher m_managerWatcher;

	OrgFreedesktopNetworkManagerAgentManagerInterface m_agentManager;

	notify::NotificationManager::SPtr m_notifications;

	CredentialStore::SPtr m_credentialStore;

	std::shared_ptr<SecretRequest> m_request;
};

SecretAgent::SecretAgent(notify::NotificationManager::SPtr notificationManager,
		agent::CredentialStore::SPtr credentialStore,
		const QDBusConnection &systemConnection,
		const QDBusConnection &sessionConnection, QObject *parent) :
		QObject(parent), d(new Priv(notificationManager, credentialStore, systemConnection, sessionConnection))
	{
	// Memory managed by Qt
	new SecretAgentAdaptor(this);

	if (!d->m_systemConnection.registerObject(NM_DBUS_PATH_SECRET_AGENT, this)) {
		throw logic_error(
				"Unable to register user secret agent object on DBus");
	}

	// Watch for NM restarting (or starting after we do)
	connect(&d->m_managerWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
			d.get(), &Priv::serviceOwnerChanged);

	auto reply = d->m_agentManager.RegisterWithCapabilities(
						"com.canonical.indicator.SecretAgent",
						NM_SECRET_AGENT_CAPABILITY_NONE);
	reply.waitForFinished();
	if (reply.isError()) {
		qCritical() << reply.error().message();
	}
}

SecretAgent::~SecretAgent() {
	auto reply = d->m_agentManager.Unregister();
	reply.waitForFinished();
	if (reply.isError()) {
		qCritical() << reply.error().message();
	}
}

/**
 * Example call:
 * [Argument: a{sa{sv}}
 *   {
 *     "802-11-wireless" = [Argument: a{sv} {
 *       "security" = [Variant(QString): "802-11-wireless-security"],
 *       "ssid" = [Variant(QByteArray): {83, 119, 97, 108, 108, 111, 119, 115, 32, 66, 97, 114, 110}],
 *       "mode" = [Variant(QString): "infrastructure"],
 *       "mac-address" = [Variant(QByteArray): {8, 212, 43, 19, 139, 130}]
 *     }],
 *     "connection" = [Argument: a{sv} {
 *       "id" = [Variant(QString): "Swallows Barn"],
 *       "uuid" = [Variant(QString): "40fdd8b6-e119-41ae-87a3-7bfc8044f753"],
 *       "type" = [Variant(QString): "802-11-wireless"]
 *     }],
 *     "ipv4" = [Argument: a{sv} {
 *       "addresses" = [Variant: [Argument: aau {}]],
 *       "dns" = [Variant: [Argument: au {}]],
 *       "method" = [Variant(QString): "auto"],
 *       "routes" = [Variant: [Argument: aau {}]]
 *     }],
 *     "802-11-wireless-security" = [Argument: a{sv} {
 *       "auth-alg" = [Variant(QString): "open"],
 *       "key-mgmt" = [Variant(QString): "wpa-psk"]
 *     }],
 *     "ipv6" = [Argument: a{sv} {
 *       "addresses" = [Variant: [Argument: a(ayuay) {}]],
 *       "dns" = [Variant: [Argument: aay {}]],
 *       "method" = [Variant(QString): "auto"],
 *       "routes" = [Variant: [Argument: a(ayuayu) {}]]
 *     }]
 *   }
 * ],
 * [ObjectPath: /org/freedesktop/NetworkManager/Settings/0],
 * "802-11-wireless-security",
 * {},
 * 5
 */
QVariantDictMap SecretAgent::GetSecrets(const QVariantDictMap &connection,
		const QDBusObjectPath &connectionPath, const QString &settingName,
		const QStringList &hints, uint flags) {

	setDelayedReply(true);

	qDebug() << connectionPath.path() << settingName << hints << flags;

	// If we want a WiFi secret, and
	if (settingName == NM_WIRELESS_SECURITY_SETTING_NAME &&
			((flags & NM_SECRET_AGENT_GET_SECRETS_FLAG_ALLOW_INTERACTION) > 0) &&
			((flags & NM_SECRET_AGENT_GET_SECRETS_FLAG_USER_REQUESTED) > 0)
       ) {
		qDebug() << "Requesting secret from user";
		d->m_request.reset(new SecretRequest(*this, connection,
						connectionPath, settingName, hints, flags, message()));
	} else if (((flags == NM_SECRET_AGENT_GET_SECRETS_FLAG_NONE) ||
				(flags == NM_SECRET_AGENT_GET_SECRETS_FLAG_USER_REQUESTED))) {
		qDebug() << "Retrieving secret from keyring";

		bool isVpn = (settingName == NM_VPN_SETTING_NAME);

		QString uuid = connection[NM_CONNECTION_SETTING_NAME][NM_CONNECTION_UUID].toString();

		QStringMap secrets;

		try {
			secrets = d->m_credentialStore->get(uuid, settingName);
		} catch (domain_error& e) {
			d->m_systemConnection.send(
					message().createErrorReply(
							"org.freedesktop.NetworkManager.SecretAgent.InternalError",
							e.what()));
			return QVariantDictMap();
		}

		if (secrets.isEmpty()) {
			d->m_systemConnection.send(
					message().createErrorReply(
							"org.freedesktop.NetworkManager.SecretAgent.NoSecrets",
							"No secrets found for this connection."));
			return QVariantDictMap();
		}

		QVariantDictMap newConnection;

		if (isVpn) {
			newConnection[settingName][NM_VPN_SECRETS] = QVariant::fromValue(
					secrets);
		} else {
			QMapIterator<QString, QString> it(secrets);
			while (it.hasNext()) {
				it.next();
				newConnection[settingName][it.key()] = it.value();
			}
		}

		d->m_systemConnection.send(
				message().createReply(QVariant::fromValue(newConnection)));
	} else {
		qDebug() << "Can't get secrets for this connection";
		d->m_systemConnection.send(
				message().createErrorReply("org.freedesktop.NetworkManager.SecretAgent.NoSecrets",
						"No secrets found for this connection."));
	}

	return QVariantDictMap();
}

void SecretAgent::FinishGetSecrets(SecretRequest &request, bool error) {
	if (error) {
		d->m_systemConnection.send(
				request.message().createErrorReply("org.freedesktop.NetworkManager.SecretAgent.NoSecrets",
						"No secrets found for this connection."));
	} else {
		d->m_systemConnection.send(
				request.message().createReply(
						QVariant::fromValue(request.connection())));
	}

	d->m_request.reset();
}

void SecretAgent::CancelGetSecrets(const QDBusObjectPath &connectionPath,
		const QString &settingName) {
	Q_UNUSED(settingName);

	if (d->m_request && d->m_request->connectionPath() == connectionPath) {
		d->m_request.reset();
	}
}

void SecretAgent::DeleteSecrets(const QVariantDictMap &connection,
		const QDBusObjectPath &connectionPath) {
	Q_UNUSED(connectionPath);

	QString uuid = connection[NM_CONNECTION_SETTING_NAME][NM_CONNECTION_UUID].toString();
	d->m_credentialStore->clear(uuid);
}

void SecretAgent::SaveSecrets(const QVariantDictMap &connection,
		const QDBusObjectPath &connectionPath) {
	Q_UNUSED(connectionPath);

	QString id = connection[NM_CONNECTION_SETTING_NAME][NM_CONNECTION_ID].toString();
	QString uuid = connection[NM_CONNECTION_SETTING_NAME][NM_CONNECTION_UUID].toString();
	QString type = connection[NM_CONNECTION_SETTING_NAME][NM_CONNECTION_TYPE].toString();

	if (type == NM_VPN_SETTING_NAME) {
		d->saveVpnSettings(id, uuid, NM_VPN_SETTING_NAME, connection[NM_VPN_SETTING_NAME]);
	} else {
		QMapIterator<QString, QVariantMap> iter(connection);
		while (iter.hasNext()) {
			iter.next();
			d->saveSettings(id, uuid, iter.key(), iter.value());
		}
	}
}

notify::NotificationManager::SPtr SecretAgent::notifications() {
	return d->m_notifications;
}

}

#include "SecretAgent.moc"

