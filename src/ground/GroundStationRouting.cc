/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "GroundStationRouting.h"

namespace flora {
namespace ground {

Define_Module(GroundStationRouting);

void GroundStationRouting::initialize(int stage) {
    GroundStationRoutingBase::initialize(stage);
}

}  // namespace ground
}  // namespace flora
