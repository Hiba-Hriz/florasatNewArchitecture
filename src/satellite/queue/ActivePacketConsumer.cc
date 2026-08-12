/*
 * ActivePacketConsumer.cc
 *
 *  Created on: Mar 17, 2023
 *      Author: Robin Ohs
 */

#include "satellite/queue/ActivePacketConsumer.h"

namespace flora {
namespace satellite {
namespace queue {

Define_Module(ActivePacketConsumer);

void ActivePacketConsumer::collectPacket() {
    auto packet = provider->pullPacket(inputGate->getPathStartGate());
    take(packet);
    // EV_INFO << "Collecting packet" << packet << EV_ENDL;
    numProcessedPackets++;
    processedTotalLength += packet->getDataLength();
    send(packet, "out");
}

void ActivePacketConsumer::handleCanPullPacketChanged(cGate *gate) {
    Enter_Method("handleCanPullPacketChanged");
    if (!collectionTimer->isScheduled() && provider->canPullSomePacket(inputGate->getPathStartGate())) {
        scheduleCollectionTimer();
    }
}

}  // namespace queue
}  // namespace satellite
}  // namespace flora