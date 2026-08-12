/*
 * PacketState.h
 *
 *  Created on: Mar 13, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_METRICS_PACKETSTATE_H_
#define __FLORA_METRICS_PACKETSTATE_H_

#include <omnetpp.h>
#include <string>

namespace flora {
namespace metrics {
namespace PacketState {

/** @brief Enum to indicate the state of a packet that will be recorded. */
enum Type {
    DELIVERED,
    WRONG_DELIVERED,
    DROPPED,
    UNROUTABLE,
    EXPIRED,
};

static const Type All[] = {DELIVERED, WRONG_DELIVERED, DROPPED, UNROUTABLE, EXPIRED};

std::string to_string(PacketState::Type state);

}  // namespace PacketState
}  // namespace metrics
}  // namespace flora

#endif  // __FLORA_METRICS_PACKETSTATE_H_