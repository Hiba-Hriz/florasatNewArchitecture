/*
 * DtnGroundStationRouting.h
 *
 *  Created on: May 30, 2023
 *      Author: Sebastian Montoya
 */

#ifndef __FLORA_GROUND_DTNGROUNDSTATIONROUTING_H_
#define __FLORA_GROUND_DTNGROUNDSTATIONROUTING_H_

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

class DtnGroundStationRouting : public GroundStationRoutingBase {
  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

}  // namespace ground
}  // namespace flora

#endif  // __FLORA_GROUND_DTNGROUNDSTATIONROUTING_H_
