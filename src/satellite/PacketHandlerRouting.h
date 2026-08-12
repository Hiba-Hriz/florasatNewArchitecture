/*
 * PacketHandlerRouting.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_PACKETHANDLERROUTING_H_
#define __FLORA_SATELLITE_PACKETHANDLERROUTING_H_

#include <omnetpp.h>

#include "ChannelBusyHeader_m.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "mobility/INorad.h"
#include "mobility/NoradA.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"

using namespace omnetpp;

namespace flora {
namespace satellite {

class PacketHandlerRouting : public cSimpleModule {
   protected:
    int satIndex = -1;
    int maxHops = -1;

    routing::RoutingBase *routing = nullptr;
    cMessage *selfMsg = nullptr;

   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    void routeMessage(Packet *msg);
    void sendMessage(cGate *gate, Packet *pkt);
    cGate *getGate(isldirection::ISLDirection routingResult);
    bool checkMaxHopCountReached(Packet *pkt);
    void handlePacketMetadata(inet::Ptr<RoutingHeader> frame);
};

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_PACKETHANDLERROUTING_H_