/*
 * DtnPacketHandlerRouting.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_DTN_PACKETHANDLERROUTING_H_
#define __FLORA_SATELLITE_DTN_PACKETHANDLERROUTING_H_

#include <omnetpp.h>
#include "routing/dtn/contactplan/ContactPlan.h"
#include "topologycontrol/DtnTopologyControl.h"

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iomanip>

#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "mobility/INorad.h"
#include "mobility/NoradA.h"
#include "routing/RoutingBase.h"
#include "routing/dtn/MsgTypes.h"
#include "routing/DtnRoutingHeader_m.h"

using namespace omnetpp;

namespace flora {
namespace satellite {

class DtnPacketHandlerRouting : public cSimpleModule {

   protected:
    int satIndex = -1;
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;

   private:
    int eid_;
    ContactPlan* contactTopology_;
    topologycontrol::DtnTopologyControl* topologyControl;
    double packetLoss_;
    isldirection::ISLDirection getSatISLDirection(int satId, int nextSatId);
    cGate *getGate(isldirection::ISLDirection routingResult);

};

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_DTN_PACKETHANDLERROUTING_H_
