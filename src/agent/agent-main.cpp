/*
 * Copyright (C) 2014 Canonical, Ltd.
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
 *     Antti Kaijanmäki <antti.kaijanmaki@canonical.com>
 */

#include <notify-cpp/notification-manager.h>
#include <agent/KeyringCredentialStore.h>
#include <agent/SecretAgent.h>
#include <util/logging.h>
#include <util/unix-signal-handler.h>
#include <dbus-types.h>

#include <QCoreApplication>

#include <libintl.h>
#include <cstdlib>
#include <ctime>

#include <glib.h>

using namespace std;

int
main(int argc, char **argv)
{
    qInstallMessageHandler(util::loggingFunction);

    QCoreApplication app(argc, argv);
    DBusTypes::registerMetaTypes();
    Variant::registerMetaTypes();
    std::srand(std::time(0));

    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    util::UnixSignalHandler handler([]{
		QCoreApplication::exit(0);
	});
	handler.setupUnixSignalHandlers();

    auto agent = make_unique<agent::SecretAgent>(
            make_shared<notify::NotificationManager>(GETTEXT_PACKAGE),
            make_shared<agent::KeyringCredentialStore>(),
            QDBusConnection::systemBus(), QDBusConnection::sessionBus());

    return app.exec();
}
