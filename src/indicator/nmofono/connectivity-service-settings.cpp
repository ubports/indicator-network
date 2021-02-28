/*
 * Copyright (C) 2016 Canonical, Ltd.
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

#include <nmofono/connectivity-service-settings.h>

using namespace std;
using namespace nmofono;

class ConnectivityServiceSettings::Private : public QObject
{
    Q_OBJECT
public:

    ConnectivityServiceSettings &p;
    unique_ptr<QSettings> m_settings;

    Private(ConnectivityServiceSettings &parent)
        : p(parent)
    {
    }

    virtual ~Private()
    {

    }

public Q_SLOTS:

Q_SIGNALS:

};

ConnectivityServiceSettings::ConnectivityServiceSettings(QObject *parent)
    : QObject(parent),
      d{new Private(*this)}
{
    if (qEnvironmentVariableIsSet("INDICATOR_NETWORK_SETTINGS_PATH"))
    {
        // For testing only
        QString path = QString::fromUtf8(qgetenv("INDICATOR_NETWORK_SETTINGS_PATH")) + "/config.ini";
        d->m_settings = make_unique<QSettings>(path, QSettings::IniFormat);
    }
    else
    {
        d->m_settings = make_unique<QSettings>(QSettings::IniFormat,
                                               QSettings::UserScope,
                                               "connectivity-service",
                                               "config");
    }
}

ConnectivityServiceSettings::~ConnectivityServiceSettings()
{

}

QVariant ConnectivityServiceSettings::mobileDataEnabled()
{
    return d->m_settings->value("MobileDataEnabled");
}

void ConnectivityServiceSettings::setMobileDataEnabled(bool value)
{
    d->m_settings->setValue("MobileDataEnabled", value);
}

QVariant ConnectivityServiceSettings::simForMobileData()
{
    return d->m_settings->value("SimForMobileData");
}

void ConnectivityServiceSettings::setSimForMobileData(const QString &iccid)
{
    d->m_settings->setValue("SimForMobileData", iccid);
}

QStringList ConnectivityServiceSettings::knownSims()
{
    QVariant ret;
    ret = d->m_settings->value("KnownSims");
    if (ret.isNull())
    {
        /* This is the first time we are running on a system.
         */
        setKnownSims(QStringList());
        return QStringList();
    }

    return ret.toStringList();
}

void ConnectivityServiceSettings::setKnownSims(const QStringList &list)
{
    d->m_settings->setValue("KnownSims", QVariant(list));
}

wwan::Sim::Ptr ConnectivityServiceSettings::createSimFromSettings(const QString &iccid)
{
    d->m_settings->beginGroup(QString("Sims/%1/").arg(iccid));
    QVariant imsi_var = d->m_settings->value("Imsi");
    QVariant primaryPhoneNumber_var = d->m_settings->value("PrimaryPhoneNumber");
    QVariant mcc_var = d->m_settings->value("Mcc");
    QVariant mnc_var = d->m_settings->value("Mnc");
    QVariant preferredLanguages_var = d->m_settings->value("PreferredLanguages");
    QVariant dataRoamingEnabled_var = d->m_settings->value("DataRoamingEnabled");
    d->m_settings->endGroup();

    if (iccid.isNull() ||
            imsi_var.isNull() ||
            primaryPhoneNumber_var.isNull() ||
            dataRoamingEnabled_var.isNull())
    {
        qWarning() << "Corrupt settings for SIM: " << iccid;
        d->m_settings->remove(QString("Sims/%1/").arg(iccid));
        return wwan::Sim::Ptr();
    }

    return wwan::Sim::Ptr(new wwan::Sim(iccid,
                                        imsi_var.toString(),
                                        primaryPhoneNumber_var.toString(),
                                        mcc_var.toString(),
                                        mnc_var.toString(),
                                        preferredLanguages_var.toStringList(),
                                        dataRoamingEnabled_var.toBool()));
}

void ConnectivityServiceSettings::saveSimToSettings(wwan::Sim::Ptr sim)
{
    d->m_settings->beginGroup(QString("Sims/%1/").arg(sim->iccid()));
    d->m_settings->setValue("Imsi", sim->imsi());
    d->m_settings->setValue("PrimaryPhoneNumber", sim->primaryPhoneNumber());
    d->m_settings->setValue("Mcc", sim->mcc());
    d->m_settings->setValue("Mnc", sim->mnc());
    d->m_settings->setValue("PreferredLanguages", QVariant(sim->preferredLanguages()));
    d->m_settings->setValue("DataRoamingEnabled", sim->dataRoamingEnabled());
    d->m_settings->endGroup();
    qDebug() << Q_FUNC_INFO << "ICCID:" << sim->iccid();
    qDebug() << "  IMSI" << sim->imsi();
    qDebug() << "  PrimaryPhoneNumber" << sim->primaryPhoneNumber();
    qDebug() << "  MCC" << sim->mcc();
    qDebug() << "  MNC" << sim->mnc();
    qDebug() << "  PreferredLanguages" << sim->preferredLanguages();
    qDebug() << "  DataRoamingEnabled" << sim->dataRoamingEnabled();
}

#include "connectivity-service-settings.moc"
