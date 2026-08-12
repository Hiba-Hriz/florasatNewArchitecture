/*
 * SatelliteRouting.cc
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#include "satellite/DtnSatelliteRouting.h"

namespace flora {
namespace satellite {

Define_Module(DtnSatelliteRouting);

void DtnSatelliteRouting::initialize(int stage) {
    SatelliteRoutingBase::initialize(stage);
}

void DtnSatelliteRouting::finish() {
    SatelliteRoutingBase::finish();
}

}  // namespace satellite
}  // namespace flora
