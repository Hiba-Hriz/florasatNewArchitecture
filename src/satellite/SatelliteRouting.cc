/*
 * SatelliteRouting.cc
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#include "satellite/SatelliteRouting.h"

namespace flora {
namespace satellite {

Define_Module(SatelliteRouting);

void SatelliteRouting::initialize(int stage) {
    SatelliteRoutingBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        // subscribe to dropped packets
        subscribe(packetDroppedSignal, this);
        subscribe(packetReceivedSignal, this);

        // init vars
        numReceived = 0;
        WATCH(numReceived);
        numDroppedMaxHop = 0;
        WATCH(numDroppedMaxHop);
        numDroppedFullQueue = 0;
        WATCH(numDroppedFullQueue);

        receivedCountStats.setName("receivedCountStats");
        droppedMaxHopCountStats.setName("droppedMaxHopCountStats");
        droppedFullQueueCountStats.setName("droppedFullQueueCountStats");
    }
}

void SatelliteRouting::finish() {
    SatelliteRoutingBase::finish();

    EV << "Received: " << numReceived << endl;
    EV << "Dropped: " << numDroppedFullQueue + numDroppedMaxHop << "(FullQueue: " << numDroppedFullQueue << "; MaxHops: " << numDroppedFullQueue << ")" << endl;

    recordScalar("#received", numReceived);
    recordScalar("#droppedFullQueue", numDroppedFullQueue);
    recordScalar("#droppedMaxHops", numDroppedMaxHop);
}

void SatelliteRouting::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) {
    if (signalID == packetDroppedSignal) {
        auto pkt = check_and_cast<inet::Packet *>(obj);
        auto reason = check_and_cast<inet::PacketDropDetails *>(details);
        handlePacketDropped(pkt, reason);
    } else if (signalID == packetReceivedSignal) {
        auto pkt = check_and_cast<inet::Packet *>(obj);
        handlePacketReceived(pkt);
    }
}

void SatelliteRouting::handlePacketDropped(inet::Packet *pkt, inet::PacketDropDetails *reason) {
    EV_INFO << "Dropped: " << pkt << EV_ENDL;
    if (reason->getReason() == PacketDropReason::HOP_LIMIT_REACHED) {
        numDroppedMaxHop++;
        droppedMaxHopCountStats.record(numDroppedMaxHop);
    } else if (reason->getReason() == PacketDropReason::QUEUE_OVERFLOW) {
        numDroppedFullQueue++;
        droppedFullQueueCountStats.record(numDroppedFullQueue);
    } else {
        error("Unhandled drop reason: %d", reason->getReason());
    }
}

void SatelliteRouting::handlePacketReceived(inet::Packet *pkt) {
    numReceived++;
    receivedCountStats.record(numReceived);
}

}  // namespace satellite
}  // namespace flora