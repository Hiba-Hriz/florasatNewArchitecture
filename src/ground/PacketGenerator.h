/*
 * PacketGenerator.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_GROUND_PACKETGENERATOR_H_
#define __FLORA_GROUND_PACKETGENERATOR_H_

#include <omnetpp.h>

#include "TransportHeader_m.h"
#include "core/Utils.h"
#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "metrics/MetricsCollector.h"
#include "networklayer/ConstellationRoutingTable.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"
#include "routing/core/DijkstraShortestPath.h"
#include "topologycontrol/TopologyControlBase.h"
#include "ground/PacketGeneratorBase.h"

using namespace omnetpp;
using namespace inet;

namespace flora {

class PacketGenerator : public PacketGeneratorBase {

   protected:
    virtual void initialize(int stage) override;

   protected:
    virtual void handleMessage(cMessage *msg) override;
    void encapsulate(Packet *packet, int dstGs, int firstSat, int lastSat);
    void decapsulate(Packet *packet);
};

}  // namespace flora

#endif  // __FLORA_GROUND_PACKETGENERATOR_H_
