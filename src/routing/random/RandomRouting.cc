/*
 * RandomRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RandomRouting.h"

namespace flora {
namespace routing {

Define_Module(RandomRouting);

ISLDirection RandomRouting::routePacket(inet::Ptr<RoutingHeader> frame, cModule *callerSat) {
    int destinationGroundStation = frame->getDestinationGroundstation();
    int callerSatIndex = callerSat->getIndex();

    // check if connected to destination groundstation
    int groundlinkIndex = RoutingBase::getGroundlinkIndex(callerSatIndex, destinationGroundStation);
    if (groundlinkIndex != -1) {
        if (RoutingBase::hasConnection(callerSat, ISLDirection(Direction::ISL_DOWNLINK, groundlinkIndex))) {
            return ISLDirection(Direction::ISL_DOWNLINK, groundlinkIndex);
        } else {
            error("Error in RandomRouting::RoutePacket: There should be a connection between groundstation and satellite but is not connected.");
        }
    }

    // if not connected to destination find random
    int gate = intrand(4);
    if (gate == 0 && RoutingBase::hasConnection(callerSat, ISLDirection(Direction::ISL_DOWN, -1))) {
        return ISLDirection(Direction::ISL_DOWN, -1);
    } else if (gate == 1 && RoutingBase::hasConnection(callerSat, ISLDirection(Direction::ISL_UP, -1))) {
        return ISLDirection(Direction::ISL_UP, -1);
    } else if (gate == 2 && RoutingBase::hasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1))) {
        return ISLDirection(Direction::ISL_LEFT, -1);
    } else if (gate == 3 && RoutingBase::hasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1))) {
        return ISLDirection(Direction::ISL_RIGHT, -1);
    }
    return routePacket(frame, callerSat);
}

}  // namespace routing
}  // namespace flora