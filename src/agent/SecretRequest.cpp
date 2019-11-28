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

#include <agent/SecretRequest.h>
#include <agent/SecretAgent.h>
#include <util/localisation.h>

#include <notify-cpp/notification-manager.h>

namespace agent
{

SecretRequest::SecretRequest(SecretAgent &secretAgent,
		const QVariantDictMap &connection,
		const QDBusObjectPath &connectionPath, const QString &settingName,
		const QStringList &hints, uint flags, const QDBusMessage &message,
		QObject *parent) :
		QObject(parent), m_secretAgent(secretAgent), m_connection(
				connection), m_connectionPath(connectionPath), m_settingName(
				settingName), m_hints(hints), m_flags(flags), m_message(
				message), m_menu() {

	// indicate to the notification-daemon, that we want to use snap-decisions
	QVariantMap notificationHints;
	notificationHints["x-canonical-snap-decisions"] = "true";
	notificationHints["x-canonical-private-button-tint"] = "true";
	notificationHints["x-canonical-non-shaped-icon"] = "true";
	notificationHints["x-canonical-snap-decisions-timeout"] = std::numeric_limits<std::int32_t>::max();

	QVariantMap menuModelActions;
	menuModelActions["notifications"] = m_menu.actionPath();

	QVariantMap menuModelPaths;
	menuModelPaths["busName"] = m_menu.busName();
	menuModelPaths["menuPath"] = m_menu.menuPath();
	menuModelPaths["actions"] = menuModelActions;

	notificationHints["x-canonical-private-menu-model"] = menuModelPaths;

	const QVariantMap &conn = m_connection[SecretAgent::NM_CONNECTION_SETTING_NAME];

	auto wirelessSecurity = m_connection.find(m_settingName);
	QString keyMgmt(
			wirelessSecurity->value(SecretAgent::NM_WIRELESS_SECURITY_KEY_MGMT).toString());

	QString title(_("Connect to \"%1\""));

	QString subject;
	if (keyMgmt == SecretAgent::NM_KEY_MGMT_WPA_NONE
			|| keyMgmt == SecretAgent::NM_KEY_MGMT_WPA_PSK) {
		subject = _("WPA");
	} else if (keyMgmt == SecretAgent::NM_KEY_MGMT_NONE) {
		subject = _("WEP");
	}

	m_notification = m_secretAgent.notifications()->notify(
			title.arg(conn[SecretAgent::NM_CONNECTION_ID].toString()), subject,
			"wifi-full-secure",
			QStringList() << "connect_id" << _("Connect") << "cancel_id"
					<< _("Cancel"), notificationHints, 0);

    connect(m_notification.get(), &notify::Notification::actionInvoked, this,
            &SecretRequest::actionInvoked);

    connect(m_notification.get(), &notify::Notification::closed, this,
            &SecretRequest::notificationClosed);

    m_notification->show();
}

SecretRequest::~SecretRequest() {
}

/* Called when the user submits a password */
void SecretRequest::actionInvoked(const QString &actionKey) {

    m_notification.reset();

	if (actionKey != "connect_id") {
		m_secretAgent.FinishGetSecrets(*this, true);
		return;
	}

	QString key("");
	key = m_menu.password();

	auto wirelessSecurity = m_connection.find(m_settingName);
	QString keyMgmt(
			wirelessSecurity->value(SecretAgent::NM_WIRELESS_SECURITY_KEY_MGMT).toString());

	if (keyMgmt == SecretAgent::NM_KEY_MGMT_WPA_NONE
			|| keyMgmt == SecretAgent::NM_KEY_MGMT_WPA_PSK) {
		wirelessSecurity->insert(SecretAgent::NM_WIRELESS_SECURITY_PSK, key);
	} else if (keyMgmt == SecretAgent::NM_KEY_MGMT_NONE) {
		wirelessSecurity->insert(SecretAgent::NM_WIRELESS_SECURITY_WEP_KEY0, key);
	}

	m_secretAgent.FinishGetSecrets(*this, false);
}

/* Called when the user closes the dialog */
void SecretRequest::notificationClosed(uint reason) {
	Q_UNUSED(reason);

	m_notification.reset();

	m_secretAgent.FinishGetSecrets(*this, true);
}

const QVariantDictMap & SecretRequest::connection() const {
	return m_connection;
}

const QDBusMessage & SecretRequest::message() const {
	return m_message;
}

const QDBusObjectPath & SecretRequest::connectionPath() const {
	return m_connectionPath;
}

}
