/*
 * PacketState.cc
 *
 *  Created on: Mar 13, 2023
 *      Author: Robin Ohs
 */
#include "PacketState.h"

namespace flora {
namespace metrics {
namespace PacketState {

std::string to_string(PacketState::Type state) {
    switch (state) {
        case DELIVERED:
            return "DELIVERED";
        case WRONG_DELIVERED:
            return "WRONG_DELIVERED";
        case DROPPED:
            return "DROPPED";
        case UNROUTABLE:
            return "UNROUTABLE";
        case EXPIRED:
            return "EXPIRED";
        default:
            throw omnetpp::cRuntimeError("Unhandled packet state");
    }
}

}  // namespace PacketState
}  // namespace metrics
}  // namespace flora
