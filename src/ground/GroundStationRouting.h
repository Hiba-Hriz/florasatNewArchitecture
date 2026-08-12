/*
 * GroundStationRouting.h
 *
 *  Created on: May 19, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_GROUND_GROUNDSTATIONROUTING_H_
#define __FLORA_GROUND_GROUNDSTATIONROUTING_H_

#include <omnetpp.h>

#include <set>
#include <sstream>

#include "core/Constants.h"
#include "core/PositionAwareBase.h"
#include "core/utils/SetUtils.h"
#include "mobility/GroundStationMobility.h"
#include "ground/GroundStationRoutingBase.h"

using namespace omnetpp;
using namespace flora::core;

namespace flora {
namespace ground {

class GroundStationRouting : public GroundStationRoutingBase {
   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

}  // namespace ground
}  // namespace flora

#endif  // __FLORA_GROUND_GROUNDSTATIONROUTING_H_
