/*
 * RandomRouting.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_RANDOMROUTING_H_
#define __FLORA_ROUTING_RANDOMROUTING_H_

#include <omnetpp.h>

#include "core/Utils.h"
#include "inet/common/packet/Packet.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"

namespace flora {
namespace routing {

class RandomRouting : public RoutingBase {
   public:
    ISLDirection routePacket(inet::Ptr<RoutingHeader> frame, cModule *callerSat) override;
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_RANDOMROUTING_H_
