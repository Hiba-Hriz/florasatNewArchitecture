/*
 * SatelliteRoutingBase.h
 *
 *  Created on: May 15, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_SATELLITEROUTINGBASE_H_
#define __FLORA_SATELLITE_SATELLITEROUTINGBASE_H_

#include <omnetpp.h>

#include "core/Constants.h"
#include "core/ISLDirection.h"
#include "core/ISLState.h"
#include "core/PositionAwareBase.h"
#include "core/Utils.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "mobility/NoradA.h"
#include "routing/RoutingHeader_m.h"

using namespace omnetpp;
using namespace inet;
using namespace flora::core;
using flora::core::Constants;

namespace flora {
namespace satellite {

class SatelliteRoutingBase : public cSimpleModule, public PositionAwareBase {
   private:
    int satId = -1;
    int satPlane;
    int satNumberInPlane;
    NoradA *noradModule;
    /** @brief The upper latitude to shut down inter-plane ISL. (Noth-Pole) */
    double upperLatitudeBound;
    /** @brief The lower latitude to shut down inter-plane ISL. (South-Pole) */
    double lowerLatitudeBound;

    SatelliteRoutingBase *leftSatellite = nullptr;
    SatelliteRoutingBase *rightSatellite = nullptr;
    SatelliteRoutingBase *upSatellite = nullptr;
    SatelliteRoutingBase *downSatellite = nullptr;

    ISLState leftSendState = ISLState::WORKING;
    ISLState leftRecvState = ISLState::WORKING;
    ISLState upSendState = ISLState::WORKING;
    ISLState upRecvState = ISLState::WORKING;
    ISLState rightSendState = ISLState::WORKING;
    ISLState rightRecvState = ISLState::WORKING;
    ISLState downSendState = ISLState::WORKING;
    ISLState downRecvState = ISLState::WORKING;

   public:
    /** @brief Returns the id of the satellite. */
    int getId() const { return satId; }
    /** @brief Returns the plane of the satellite. */
    int getPlane() const { return satPlane; }
    /** @brief Gives the number in the plane. The first sat in any plane has number 0.*/
    int getNumberInPlane() const { return satNumberInPlane; }

    std::pair<cGate *, ISLState> getInputGate(isldirection::Direction direction, int index = -1);
    std::pair<cGate *, ISLState> getOutputGate(isldirection::Direction direction, int index = -1);
    std::pair<cGate *, ISLState> getGate(isldirection::Direction direction, cGate::Type type, int index);
    /** @brief Connects the satellite to the other satellite. The return value indicates whether the connection is new or the channel params were updated.*/
    bool connect(SatelliteRoutingBase *other, isldirection::Direction direction);

    /** @brief Disconnects the satellite on the given direction. The return value indicates whether the connection was deleted or did not exist.*/
    bool disconnect(isldirection::Direction direction);
    //  /** @brief Connects the satellite via right Inter-Plane ISL to the other satellite. The return value indicates whether the connection is new or the channel params were updated.*/
    //  bool connectRight(SatelliteRoutingBase *other);

    bool hasLeftSat() const;
    bool hasUpSat() const;
    bool hasRightSat() const;
    bool hasDownSat() const;

    SatelliteRoutingBase *getLeftSat() const;
    SatelliteRoutingBase *getUpSat() const;
    SatelliteRoutingBase *getRightSat() const;
    SatelliteRoutingBase *getDownSat() const;

    void setLeftSat(SatelliteRoutingBase *newSat);
    void setUpSat(SatelliteRoutingBase *newSat);
    void setRightSat(SatelliteRoutingBase *newSat);
    void setDownSat(SatelliteRoutingBase *newSat);

    void removeLeftSat();
    void removeUpSat();
    void removeRightSat();
    void removeDownSat();

    int getLeftSatId() const;
    int getUpSatId() const;
    int getRightSatId() const;
    int getDownSatId() const;

    double getLeftSatDistance() const;
    double getUpSatDistance() const;
    double getRightSatDistance() const;
    double getDownSatDistance() const;

    double getLatitude() const override;
    double getLongitude() const override;
    double getAltitude() const override;

    ISLState getLeftSendState() const { return leftSendState; }
    ISLState getLeftRecvState() const { return leftRecvState; }
    ISLState getUpSendState() const { return upSendState; }
    ISLState getUpRecvState() const { return upRecvState; }
    ISLState getRightSendState() const { return rightSendState; }
    ISLState getRightRecvState() const { return rightRecvState; }
    ISLState getDownSendState() const { return downSendState; }
    ISLState getDownRecvState() const { return downRecvState; }

    void setLeftSendState(ISLState newState);
    void setLeftRecvState(ISLState newState);
    void setUpSendState(ISLState newState);
    void setUpRecvState(ISLState newState);
    void setRightSendState(ISLState newState);
    void setRightRecvState(ISLState newState);
    void setDownSendState(ISLState newState);
    void setDownRecvState(ISLState newState);

    void setISLSendState(isldirection::Direction direction, ISLState state);
    void setISLRecvState(isldirection::Direction direction, ISLState state);

    /** @brief Returns the elevation from this entity to a reference entity. */
    double getElevation(const PositionAwareBase &other) const;
    /** @brief Returns the azimuth from this entity to a reference entity. */
    double getAzimuth(const PositionAwareBase &other) const;
    /** @brief Returns whether the satellite is currently ascending. */
    bool isAscending() const;
    /** @brief Returns whether the satellite is currently descending. */
    bool isDescending() const;

    /** @brief Returns whether the satellite is currently able to create inter plane ISL connections. */
    bool isInterPlaneISLEnabled() const;

    friend std::ostream &operator<<(std::ostream &ss, const SatelliteRoutingBase &p) {
        ss << "{";
        ss << "\"satelliteId\": " << p.satId << ",";
        ss << "\"up\": " << p.upSatellite << ",";
        ss << "\"down\": " << p.downSatellite << ",";
        ss << "\"left\": " << p.leftSatellite << ",";
        ss << "\"right\": " << p.rightSatellite << ",";
        ss << "}";
        return ss;
    }

   protected:
    virtual void finish() override;
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    void setISLState(isldirection::Direction direction, bool send, ISLState state);

#ifndef NDEBUG
    virtual void refreshDisplay() const override;
#endif
};

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_SATELLITEROUTINGBASE_H_
