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

#include <nmofono/wwan/modem.h>

#include <ofono/dbus.h>
#include <QDebug>

#define slots
#include <qofono-qt5/qofonomodem.h>
#include <qofono-qt5/qofonosimmanager.h>
#include <qofono-qt5/qofononetworkregistration.h>
#include <qofono-qt5/qofonoconnectionmanager.h>
#undef slots

using namespace std;

namespace nmofono
{
namespace wwan
{
namespace
{

static Modem::ModemStatus str2status(const QString& str)
{
    if (str == "unregistered")
        return Modem::ModemStatus::unregistered;
    if (str == "registered")
        return Modem::ModemStatus::registered;
    if (str == "searching")
        return Modem::ModemStatus::searching;
    if (str == "denied")
        return Modem::ModemStatus::denied;
    if (str == "unknown" || str.isEmpty())
        return Modem::ModemStatus::unknown;
    if (str == "roaming")
        return Modem::ModemStatus::roaming;

    qWarning() << ": Unknown status" << str;
    return Modem::ModemStatus::unknown;
}

static Modem::Bearer str2technology(const QString& str)
{
    if (str.isEmpty() || str == "none")
        return Modem::Bearer::notAvailable;
    if (str == "gprs" || str == "gsm")
        return Modem::Bearer::gprs;
    if (str == "edge")
        return Modem::Bearer::edge;
    if (str == "umts")
        return Modem::Bearer::umts;
    if (str == "hspa" || str == "hsupa" || str == "hsdpa")
        return Modem::Bearer::hspa;
    if (str == "hspap")
        return Modem::Bearer::hspa_plus;
    if (str == "lte")
        return Modem::Bearer::lte;

    qWarning() << "Unknown technology" << str;
    return Modem::Bearer::notAvailable;
}
}

class Modem::Private : public QObject, public std::enable_shared_from_this<Private>
{
    Q_OBJECT

public:
    Modem& p;

    bool m_online;

    shared_ptr<QOfonoModem> m_ofonoModem;
    Modem::SimStatus m_simStatus;
    Modem::PinType m_requiredPin;
    RetriesType m_retries;

    bool m_simStatusSet = false;
    bool m_requiredPinSet = false;
    bool m_retriesSet = false;

    QString m_serial;
    bool m_serialSet = false;
    bool m_presentSet = false;
    bool m_present = false;
    bool m_readyFired = false;

    QString m_operatorName;
    Modem::ModemStatus m_status;
    int8_t m_strength;
    Modem::Bearer m_bearer;

    bool m_dataEnabled;

    QString m_simIdentifier;
    int m_index = -1;

    QSet<QString> m_interfaces;

    Sim::Ptr m_sim;

    shared_ptr<QOfonoConnectionManager> m_connectionManager;
    shared_ptr<QOfonoNetworkRegistration> m_networkRegistration;
    shared_ptr<QOfonoSimManager> m_simManager;

    QTimer m_updatedTimer;

    bool m_shouldTriggerUnlock = false;

    Private(Modem& parent, shared_ptr<QOfonoModem> ofonoModem)
        : p(parent), m_ofonoModem{ofonoModem}
    {
        connect(m_ofonoModem.get(), &QOfonoModem::onlineChanged, this, &Private::update);
        setOnline(m_ofonoModem->online());

        connect(m_ofonoModem.get(), &QOfonoModem::interfacesChanged, this, &Private::interfacesChanged);
        interfacesChanged(m_ofonoModem->interfaces());

        connect(m_ofonoModem.get(), &QOfonoModem::serialChanged, this, &Private::serialChanged);
        serialChanged(m_ofonoModem->serial());

        /// @todo hook up with system-settings to allow changing the identifier.
        ///       for now just provide the defaults
        auto path = m_ofonoModem->modemPath();
        if (path.endsWith("_0")) {
            setSimIdentifier("SIM 1");
            m_index = 1;
        } else if (path.endsWith("_1")) {
            setSimIdentifier("SIM 2");
            m_index = 2;
        } else {
            setSimIdentifier(path);
        }

        // Throttle the updates using a timer
        m_updatedTimer.setInterval(0);
        m_updatedTimer.setSingleShot(true);
        connect(&m_updatedTimer, &QTimer::timeout, this, &Private::fireUpdate);
    }

public Q_SLOTS:
    void fireUpdate()
    {
        Q_EMIT p.updated(p);

        if (p.isReadyToUnlock() && m_shouldTriggerUnlock)
        {

            qDebug() << "SIM ready to unlock:" << p.simIdentifier();
            m_shouldTriggerUnlock = false;
            Q_EMIT p.readyToUnlock(p.name());
        }

        if (m_serialSet && !m_readyFired)
        {
            if (!m_simManager)
            {
                return;
            }

            if (!m_sim)
            {
                if (!m_presentSet)
                {
                    return;
                }
            }

            m_readyFired = true;
            Q_EMIT p.ready();
        }
    }

    void serialChanged(const QString &value) {
        if (m_serial == value)
        {
            return;
        }
        if (m_serialSet)
        {
            qWarning() << "Unexpected update on SERIAL: " << m_serial << ", " << value;
        }

        m_serial = value;
        m_serialSet = true;

        update();
    }

    void connectionManagerChanged(shared_ptr<QOfonoConnectionManager> conmgr)
    {
        if (conmgr == m_connectionManager)
        {
            return;
        }

        m_connectionManager = conmgr;
        if (m_connectionManager)
        {
            connect(m_connectionManager.get(),
                    &QOfonoConnectionManager::poweredChanged, this,
                    &Private::update);

            connect(m_connectionManager.get(),
                    &QOfonoConnectionManager::bearerChanged, this,
                    &Private::update);
        }

        update();
    }

    void networkRegistrationChanged(shared_ptr<QOfonoNetworkRegistration> netreg)
    {
        if (m_networkRegistration == netreg)
        {
            return;
        }

        m_networkRegistration = netreg;
        if (m_networkRegistration)
        {
            connect(m_networkRegistration.get(),
                    &QOfonoNetworkRegistration::nameChanged, this,
                    &Private::update);

            connect(m_networkRegistration.get(),
                    &QOfonoNetworkRegistration::statusChanged, this,
                    &Private::update);

            connect(m_networkRegistration.get(),
                    &QOfonoNetworkRegistration::strengthChanged, this,
                    &Private::update);

            connect(m_networkRegistration.get(),
                    &QOfonoNetworkRegistration::technologyChanged, this,
                    &Private::update);
        }

        update();
    }

    void simManagerChanged(shared_ptr<QOfonoSimManager> simmgr)
    {
        if (m_simManager == simmgr)
        {
            return;
        }

        m_simManager = simmgr;
        if (m_simManager)
        {
            connect(m_simManager.get(),
                    &QOfonoSimManager::presenceChanged, this,
                    &Private::update);

            connect(m_simManager.get(),
                    &QOfonoSimManager::pinRequiredChanged, this,
                    &Private::update);

            connect(m_simManager.get(),
                    &QOfonoSimManager::pinRetriesChanged, this,
                    &Private::update);

            connect(m_simManager.get(),
                    &QOfonoSimManager::enterPinComplete, this,
                    &Private::enterPinComplete);

            connect(m_simManager.get(),
                    &QOfonoSimManager::resetPinComplete, this,
                    &Private::resetPinComplete);

            connect(m_simManager.get(),
                    &QOfonoSimManager::presenceChanged, this,
                    &Private::presentChanged);
        }

        update();
    }

    void presentChanged()
    {
        m_presentSet = true;
        m_present = m_simManager->present();
        update();
    }

    void update()
    {
        setOnline(m_ofonoModem->online());

        if (m_simManager) {
            // update requiredPin
            switch(m_simManager->pinRequired())
            {
            case QOfonoSimManager::PinType::NoPin:
                setRequiredPin(PinType::none);
                break;
            case QOfonoSimManager::PinType::SimPin:
                setRequiredPin(PinType::pin);
                break;
            case QOfonoSimManager::PinType::SimPuk:
                setRequiredPin(PinType::puk);
                break;
            default:
                throw std::runtime_error("Ofono requires a PIN we have not been prepared to handle (" +
                                         to_string(m_simManager->pinRequired()) +
                                         "). Bailing out.");
            }

            m_requiredPinSet = true;

            bool retriesWasSet = true;
            // update retries
            RetriesType tmp;
            QVariantMap retries = m_simManager->pinRetries();
            QMapIterator<QString, QVariant> i(retries);
            while (i.hasNext()) {
                i.next();
                QOfonoSimManager::PinType type = (QOfonoSimManager::PinType) i.key().toInt();
                int count = i.value().toInt();
                if (count < 0)
                {
                    retriesWasSet = false;
                }
                switch(type)
                {
                    case QOfonoSimManager::PinType::SimPin:
                        tmp[Modem::PinType::pin] = count;
                        break;
                    case QOfonoSimManager::PinType::SimPuk:
                        tmp[Modem::PinType::puk] = count;
                        break;
                    default:
                        break;
                }
            }
            setRetries(tmp);

            m_retriesSet = retriesWasSet;

            // update simStatus
            bool present = m_simManager->present();
            if (!present)
            {
                setSimStatus(SimStatus::missing);
            }
            else if (m_requiredPin == PinType::none)
            {
                setSimStatus(SimStatus::ready);
            }
            else
            {
                if (m_retries.count(PinType::puk) != 0
                        && m_retries.at(PinType::puk) == 0)
                {
                    setSimStatus(SimStatus::permanentlyLocked);
                }
                else
                {
                    setSimStatus(SimStatus::locked);
                }
            }

            m_simStatusSet = true;

        } else {
            setRequiredPin(PinType::none);
            setRetries({});
            setSimStatus(SimStatus::not_available);

            m_requiredPinSet = false;
            m_retriesSet = false;
            m_simStatusSet = false;
        }


        if (m_connectionManager)
        {
            setDataEnabled(m_connectionManager->powered());
            setBearer(str2technology(m_connectionManager->bearer()));
        }
        else
        {
            setDataEnabled(false);
            setBearer(Modem::Bearer::notAvailable);
        }

        if (m_networkRegistration)
        {
            setOperatorName(m_networkRegistration->name());
            setStatus(str2status(m_networkRegistration->status()));
            setStrength((int8_t)m_networkRegistration->strength());

            // If the bearer couldn't be identified earlier then try again
            // using the org.ofono.NetworkRegistration interface
            if (m_bearer == Modem::Bearer::notAvailable)
            {
                setBearer(str2technology(m_networkRegistration->technology()));
            }
        }
        else
        {
            setOperatorName("");
            setStatus(Modem::ModemStatus::unknown);
            setStrength(-1);
        }

        m_updatedTimer.start();
    }

    void enterPinComplete(QOfonoSimManager::Error error, const QString &errorString)
    {
        if (error == QOfonoSimManager::Error::NoError)
        {
            Q_EMIT p.enterPinSuceeded();
        }
        else
        {
            Q_EMIT p.enterPinFailed(errorString);
        }
    }

    void resetPinComplete(QOfonoSimManager::Error error, const QString &errorString)
    {
        if (error == QOfonoSimManager::Error::NoError)
        {
            Q_EMIT p.resetPinSuceeded();
        }
        else
        {
            Q_EMIT p.resetPinFailed(errorString);
        }
    }

    void setOnline(bool online)
    {
        if (m_online == online)
        {
            return;
        }

        m_online = online;
        Q_EMIT p.onlineUpdated(m_online);
    }

    void setSimIdentifier(const QString& simIdentifier)
    {
        if (m_simIdentifier == simIdentifier)
        {
            return;
        }

        m_simIdentifier = simIdentifier;
        Q_EMIT p.simIdentifierUpdated(m_simIdentifier);
    }

    void setRequiredPin(Modem::PinType requiredPin)
    {
        if (m_requiredPin == requiredPin)
        {
            return;
        }

        m_requiredPin = requiredPin;
        Q_EMIT p.requiredPinUpdated(m_requiredPin);
    }

    void setRetries(const RetriesType& retries)
    {
        if (m_retries == retries)
        {
            return;
        }

        m_retries = retries;
        Q_EMIT p.retriesUpdated();
    }

    void setSimStatus(Modem::SimStatus simStatus)
    {
        if (m_simStatus == simStatus)
        {
            return;
        }

        m_simStatus = simStatus;
        Q_EMIT p.simStatusUpdated(m_simStatus);
    }

    void setOperatorName(const QString& operatorName)
    {
        if (m_operatorName == operatorName)
        {
            return;
        }

        m_operatorName = operatorName;
        Q_EMIT p.operatorNameUpdated(m_operatorName);
    }

    void setStatus(Modem::ModemStatus status)
    {
        if (m_status == status)
        {
            return;
        }

        m_status = status;
        Q_EMIT p.modemStatusUpdated(m_status);
    }

    void setStrength(int8_t strength)
    {
        if (m_strength == strength)
        {
            return;
        }

        m_strength = strength;
        Q_EMIT p.strengthUpdated(m_strength);
    }

    void setBearer(Modem::Bearer bearer)
    {
        if (m_bearer == bearer)
        {
            return;
        }

        m_bearer = bearer;
        Q_EMIT p.bearerUpdated(m_bearer);
    }

    void setDataEnabled(bool dataEnabled)
    {
        if (m_dataEnabled == dataEnabled)
        {
            return;
        }

        m_dataEnabled = dataEnabled;
        Q_EMIT p.dataEnabledUpdated(m_dataEnabled);
    }

    void interfacesChanged(const QStringList& values)
    {
        QSet<QString> interfaces(values.toSet());

        auto toRemove = m_interfaces;
        toRemove.subtract(interfaces);

        auto toAdd = interfaces;
        toAdd.subtract(m_interfaces);

        m_interfaces = interfaces;

        for(const auto& interface: toRemove)
        {
            if (interface == OFONO_CONNECTION_MANAGER_INTERFACE)
            {
                connectionManagerChanged(
                        shared_ptr<QOfonoConnectionManager>());
            }
            else if (interface == OFONO_NETWORK_REGISTRATION_INTERFACE)
            {
                networkRegistrationChanged(
                        shared_ptr<QOfonoNetworkRegistration>());
            }
            else if (interface == OFONO_SIM_MANAGER_INTERFACE)
            {
                simManagerChanged(shared_ptr<QOfonoSimManager>());
            }
        }

        for (const auto& interface: toAdd)
        {
            if (interface == OFONO_CONNECTION_MANAGER_INTERFACE)
            {
                auto connmgr = make_shared<QOfonoConnectionManager>();
                connmgr->setModemPath(m_ofonoModem->modemPath());
                connectionManagerChanged(connmgr);
            }
            else if (interface == OFONO_NETWORK_REGISTRATION_INTERFACE)
            {
                auto netreg = make_shared<QOfonoNetworkRegistration>();
                netreg->setModemPath(m_ofonoModem->modemPath());
                networkRegistrationChanged(netreg);
            }
            else if (interface == OFONO_SIM_MANAGER_INTERFACE)
            {
                auto simmgr = make_shared<QOfonoSimManager>();
                simmgr->setModemPath(m_ofonoModem->modemPath());
                simManagerChanged(simmgr);
            }
        }
    }
};

Modem::Modem(shared_ptr<QOfonoModem> ofonoModem)
    : d{new Private(*this, ofonoModem)}
{
}

Modem::~Modem()
{}

void
Modem::enterPin(PinType type, const QString &pin)
{
    if (!d->m_simManager)
    {
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": no simManager.");
    }

    switch(type) {
    case PinType::none:
        break;
    case PinType::pin:
        d->m_simManager->enterPin(QOfonoSimManager::PinType::SimPin,
                                pin);
        break;
    case PinType::puk:
        d->m_simManager->enterPin(QOfonoSimManager::PinType::SimPuk,
                                pin);
        break;
    }
}


void
Modem::resetPin(PinType type, const QString &puk, const QString &pin)
{
    if (!d->m_simManager) {
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": no simManager.");
    }

    switch(type) {
    case PinType::none:
        break;
    case PinType::puk:
        d->m_simManager->resetPin(QOfonoSimManager::PinType::SimPuk,
                                puk,
                                pin);
        break;
    default:
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": Not Supported.");
    }
}

bool
Modem::online() const
{
    return d->m_online;
}

Modem::SimStatus
Modem::simStatus() const
{
    return d->m_simStatus;
}

Modem::PinType
Modem::requiredPin() const
{
    return d->m_requiredPin;
}

const Modem::RetriesType&
Modem::retries() const
{
    return d->m_retries;
}

const QString&
Modem::operatorName()  const
{
    return d->m_operatorName;
}

Modem::ModemStatus
Modem::modemStatus() const
{
    return d->m_status;
}

std::int8_t
Modem::strength() const
{
    return d->m_strength;
}

Modem::Bearer
Modem::bearer() const
{
    return d->m_bearer;
}

const QString&
Modem::simIdentifier() const
{
    return d->m_simIdentifier;
}

bool
Modem::dataEnabled() const
{
    return d->m_dataEnabled;
}

int
Modem::index() const
{
    return d->m_index;
}

QString
Modem::name() const
{
    return ofonoPath();
}

QString
Modem::ofonoPath() const
{
    return d->m_ofonoModem->modemPath();
}

WwanLink::WwanType
Modem::wwanType() const
{
    return WwanType::GSM;
}

Link::Type
Modem::type() const
{
    return Type::wwan;
}

std::uint32_t
Modem::characteristics() const
{
    return 0;
}

Link::Status
Modem::status() const
{
    Status status = Status::offline;

    switch (d->m_status)
    {
        case ModemStatus::denied:
        case ModemStatus::unregistered:
        case ModemStatus::unknown:
            status = Status::offline;
            break;
        case ModemStatus::registered:
        case ModemStatus::roaming:
            status = Status::connected;
            break;
        case ModemStatus::searching:
            status = Status::connecting;
            break;
    }

    return status;
}

Link::Id
Modem::id() const
{
    return 0;
}

bool
Modem::isReadyToUnlock() const
{
    return d->m_simStatusSet && d->m_requiredPinSet && d->m_retriesSet
            && (d->m_requiredPin != PinType::none);
}

void
Modem::notifyWhenReadyToUnlock()
{
    qDebug() << "Notify when ready to unlock" << simIdentifier();
    d->m_shouldTriggerUnlock = true;
}

Sim::Ptr Modem::sim() const
{
    return d->m_sim;
}

void
Modem::setSim(Sim::Ptr sim)
{
    if (d->m_sim == sim)
    {
        return;
    }
    d->m_sim = sim;
    Q_EMIT simUpdated();
    d->update();
}

QString
Modem::serial() const
{
    return d->m_serial;
}



}
}

#include "modem.moc"
