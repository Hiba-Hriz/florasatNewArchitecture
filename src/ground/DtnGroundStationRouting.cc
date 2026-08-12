/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "DtnGroundStationRouting.h"

namespace flora {
namespace ground {

Define_Module(DtnGroundStationRouting);

void DtnGroundStationRouting::initialize(int stage) {
    GroundStationRoutingBase::initialize(stage);
}

}  // namespace ground
}  // namespace flora
