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

#include <libintl.h>
#include <dbus-types.h>
#include <notify-cpp/notification-manager.h>
#include <agent/KeyringCredentialStore.h>
#include <agent/SecretAgent.h>
#include <util/unix-signal-handler.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <iostream>
#include <memory>

using namespace std;

class DummyCredentialStore : public agent::CredentialStore
{
public:
    DummyCredentialStore() {
    }

    ~DummyCredentialStore() {
    }

    void
    save (const QString&, const QString&, const QString&, const QString&,
          const QString&)
    {
    }

    QMap<QString, QString>
    get (const QString&, const QString&)
    {
        return QMap<QString, QString> ();
    }

    void
    clear (const QString&)
    {
    }

};

int main(int argc, char *argv[]) {
    QCoreApplication application(argc, argv);
    DBusTypes::registerMetaTypes();
    Variant::registerMetaTypes();

    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    util::UnixSignalHandler handler([]
    {
        QCoreApplication::exit(0);
    });
    handler.setupUnixSignalHandlers();

    auto secretAgent = make_unique<agent::SecretAgent>(
                make_shared<notify::NotificationManager>(GETTEXT_PACKAGE),
                make_shared<DummyCredentialStore>(),
                QDBusConnection::systemBus(), QDBusConnection::sessionBus());

    if (argc == 2 && QString("--print-address") == argv[1]) {
        cout << QDBusConnection::systemBus().baseService().toStdString()
                << endl;
    }

    return application.exec();
}
