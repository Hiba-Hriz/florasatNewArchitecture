/*
 * ChannelState.h
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_UTILITIES_CHANNELSTATE_H_
#define __FLORA_TOPOLOGYCONTROL_UTILITIES_CHANNELSTATE_H_

#include <omnetpp.h>
#include <string.h>

using namespace omnetpp;

namespace flora {
namespace topologycontrol {

/** @brief Used to indicate if there was a state change to a channel. UPDATED is equal to UNCHANGED. */
enum ChannelState {
    CREATED,
    DELETED,
    UPDATED,
    UNCHANGED,
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_UTILITIES_CHANNELSTATE_H_