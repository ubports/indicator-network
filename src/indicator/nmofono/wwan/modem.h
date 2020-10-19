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

#pragma once

#include <nmofono/wwan/wwan-link.h>

#include <nmofono/wwan/sim.h>

#include <map>
#include <memory>

#include <QString>

class QOfonoModem;

namespace nmofono
{
namespace wwan
{

class Modem: public WwanLink
{
    Q_OBJECT

    class Private;
    std::shared_ptr<Private> d;

public:
    enum class PinType
    {
        none,
        pin,
        puk
    };

    enum class SimStatus
    {
        missing,
        error,
        locked,
        permanentlyLocked,
        ready,
        not_available
    };

    enum class ModemStatus
    {
        unregistered,
        registered,
        searching,
        denied,
        unknown,
        roaming
    };

    enum class Bearer
    {
        notAvailable,
        gprs,
        edge,
        umts,
        hspa,
        hspa_plus,
        lte
    };

    typedef std::shared_ptr<Modem> Ptr;
    typedef std::weak_ptr<Modem> WeakPtr;

    Modem() = delete;

    explicit Modem(std::shared_ptr<QOfonoModem> ofonoModem);

    virtual ~Modem();

    void enterPin(PinType type,
                  const QString &pin);

    void resetPin(PinType type,
                  const QString &puk,
                  const QString &pin);

    Q_PROPERTY(bool online READ online NOTIFY onlineUpdated)
    bool online() const;

    Q_PROPERTY(Modem::SimStatus simStatus READ simStatus NOTIFY simStatusUpdated)
    SimStatus simStatus() const;

    Q_PROPERTY(Modem::PinType requiredPin READ requiredPin NOTIFY requiredPinUpdated)
    PinType requiredPin() const;

    typedef std::map<Modem::PinType, int> RetriesType;
    Q_PROPERTY(RetriesType retries READ retries NOTIFY retriesUpdated)
    const RetriesType &retries() const;

    Q_PROPERTY(QString operatorName READ operatorName NOTIFY operatorNameUpdated)
    const QString &operatorName() const;

    Q_PROPERTY(Modem::ModemStatus modemStatus READ modemStatus NOTIFY modemStatusUpdated)
    ModemStatus modemStatus() const;

    Q_PROPERTY(std::int8_t strength READ strength NOTIFY strengthUpdated)
    std::int8_t strength() const;

    Q_PROPERTY(Modem::Bearer bearer READ bearer NOTIFY bearerUpdated)
    Bearer bearer() const;

    Q_PROPERTY(bool dataEnabled READ dataEnabled NOTIFY dataEnabledUpdated)
    bool dataEnabled() const;

    Q_PROPERTY(QString simIdentifier READ simIdentifier NOTIFY simIdentifierUpdated)
    const QString &simIdentifier() const;

    int index() const;
    Sim::Ptr sim() const;
    void setSim(Sim::Ptr sim);
    QString serial() const;

    QString name() const override;
    QString ofonoPath() const;

    WwanType wwanType() const override;

    Type type() const override;

    std::uint32_t characteristics() const override;

    Status status() const override;

    bool isManaged() const override;

    Id id() const override;

    bool isReadyToUnlock() const;

    void notifyWhenReadyToUnlock();

Q_SIGNALS:
    void onlineUpdated(bool);

    void simStatusUpdated(SimStatus);

    void requiredPinUpdated(PinType);

    void retriesUpdated();

    void operatorNameUpdated(const QString &);

    void modemStatusUpdated(ModemStatus);

    void isManagedChanged(bool);

    void strengthUpdated(std::int8_t);

    void bearerUpdated(Bearer);

    void dataEnabledUpdated(bool);

    void simIdentifierUpdated(const QString &);

    void updated(const Modem& modem);

    void enterPinSuceeded();

    void enterPinFailed(const QString& error);

    void resetPinSuceeded();

    void resetPinFailed(const QString& error);

    bool readyToUnlock(const QString& name);

    void simUpdated();

    void ready();
};

}
}

