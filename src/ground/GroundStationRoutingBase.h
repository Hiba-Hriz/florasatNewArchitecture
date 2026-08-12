/*
 * GroundStationRouting.h
 *
 *  Created on: May 19, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_GROUND_GROUNDSTATIONROUTINGBASE_H_
#define __FLORA_GROUND_GROUNDSTATIONROUTINGBASE_H_

#include <omnetpp.h>

#include <set>
#include <sstream>

#include "core/Constants.h"
#include "core/PositionAwareBase.h"
#include "core/utils/SetUtils.h"
#include "mobility/GroundStationMobility.h"

using namespace omnetpp;
using namespace flora::core;

namespace flora {
namespace ground {

class GroundStationRoutingBase : public cSimpleModule, public PositionAwareBase {
   public:
    int getGroundStationId() const { return groundStationId; }
    cGate *getInputGate(int index);
    cGate *getOutputGate(int index);

    const std::set<int> &getSatellites() const;
    void removeSatellite(int satId);
    void addSatellite(int satId);
    bool isConnectedTo(int satId);
    bool isConnectedToAnySat();

    double getLatitude() const override;
    double getLongitude() const override;
    double getAltitude() const override;

    friend std::ostream &operator<<(std::ostream &ss, const GroundStationRoutingBase &gs) {
        ss << "{";
        ss << "\"groundStationId\": " << gs.groundStationId << ",";
        ss << "\"satellites\": "
           << "[";
        for (int satellite : gs.satellites) {
            ss << satellite << ",";
        }
        ss << "],";
        ss << "}";
        return ss;
    }

   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

   private:
    int groundStationId;
    std::set<int> satellites;
    GroundStationMobility *mobility;
};

}  // namespace ground
}  // namespace flora

#endif  // __FLORA_GROUND_GROUNDSTATIONROUTINGBASE_H_
