/*
 * PacketGenerator.cc
 *
 *  Created on: May 27, 2023
 *      Author: Sebastian Montoya
 */

#include "DtnPacketGenerator.h"

namespace flora {

Define_Module(DtnPacketGenerator);

void DtnPacketGenerator::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {

        // Previous Code
        groundStationId = getParentModule()->getIndex();
        this->eid_ = getParentModule()->getIndex() + 1;
        numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");
        numSatellites = getSystemModule()->getSubmoduleVectorSize("loRaGW");
        topologycontrol = check_and_cast<topologycontrol::TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
        routingTable = check_and_cast<networklayer::ConstellationRoutingTable *>(getSystemModule()->getSubmodule("constellationRoutingTable"));
        contactPlan = check_and_cast<ContactPlan *>(getSystemModule()->getSubmodule("contactPlan"));

        // Register signals
        appBundleSent = registerSignal("appBundleSent");
        appBundleReceived = registerSignal("appBundleReceived");
        appBundleReceivedHops = registerSignal("appBundleReceivedHops");
        appBundleReceivedDelay = registerSignal("appBundleReceivedDelay");
    }
}

void DtnPacketGenerator::finish() {

}

void DtnPacketGenerator::handleMessage(cMessage *msg) {
    if (msg->arrivedOn("transportIn") && msg->getKind() == TRANSPORT_HEADER) {
        auto pkt = check_and_cast<inet::Packet *>(msg);
        auto trafficGenMsg = pkt->peekAtFront<DtnTransportHeader>();
        EV << "Bundles Number: " << trafficGenMsg->getBundlesNumber()
           << " Bundles Destination Id: " << trafficGenMsg->getDestinationEid()
           << " Bundle Size: " << trafficGenMsg->getSize()
           << " Bundle Interval: " << trafficGenMsg->getInterval()
           << " Bundle TTL: " << trafficGenMsg->getTtl()
           << " GroundStation Id: " << this->eid_<< endl;
        BundlePkt* bundle = new BundlePkt("bundle", BUNDLE);
        bundle->setSchedulingPriority(BUNDLE);

        // Bundle properties
        char bundleName[200];
        sprintf(bundleName, "Src:%d,Dst:%d(id:%d)", (int)this->eid_, (int)trafficGenMsg->getDestinationEid(), (int) bundle->getId());
        bundle->setBundleId(bundle->getId());
        bundle->setName(bundleName);
        bundle->setBitLength(trafficGenMsg->getSize() * 8);
        bundle->setByteLength(trafficGenMsg->getSize());

        // Bundle fields (set by source node)
        bundle->setSourceEid(this->eid_);
        bundle->setDestinationEid(trafficGenMsg->getDestinationEid());
        bundle->setReturnToSender(par("returnToSender"));
        bundle->setCritical(par("critical"));
        bundle->setCustodyTransferRequested(par("custodyTransfer"));
        bundle->setTtl(trafficGenMsg->getTtl());
        bundle->setCreationTimestamp(simTime());
        bundle->setQos(2);
        bundle->setBundleIsCustodyReport(false);

        // Bundle meta-data (set by intermediate nodes)
        bundle->setHopCount(0);
        bundle->setNextHopEid(0);
        bundle->setSenderEid(0);
        bundle->setCustodianEid(this->eid_);
        bundle->getVisitedNodesForUpdate().clear();
        routing::CgrRoute emptyRoute;
        emptyRoute.nextHop = EMPTY_ROUTE;
        bundle->setCgrRoute(emptyRoute);

        send(bundle, "dtnTransportInOut$o");
        emit(appBundleSent, true);
    }
    else if (msg->getKind() == BUNDLE)
    {
        BundlePkt* bundle = check_and_cast<BundlePkt *>(msg);
        char bundleName[10];
        sprintf(bundleName, "Src:%d,Dst:%d(id:%d)", bundle->getSourceEid() , bundle->getDestinationEid(), (int) bundle->getId());
        EV << "Bundle Received in DtnPacketGenerator " << bundleName << endl;

        int destinationEid = bundle->getDestinationEid();
        if (this->eid_ == destinationEid)
        {
            emit(appBundleReceived, true);
            emit(appBundleReceivedHops, bundle->getHopCount());
            emit(appBundleReceivedDelay, simTime() - bundle->getCreationTimestamp());
            delete msg;
        }
        else
        {
            throw cException("Error: message received in wrong destination");
        }
    }
}


std::vector<int> DtnPacketGenerator::getBundlesNumberVec()
{
    return this->bundlesNumber;
}

std::vector<int> DtnPacketGenerator::getDestinationEidVec()
{
    return this->destinationsEid;
}

std::vector<int> DtnPacketGenerator::getSizeVec()
{
    return this->sizes;
}

std::vector<double> DtnPacketGenerator::getStartVec()
{
    return this->starts;
}

}  // namespace flora
