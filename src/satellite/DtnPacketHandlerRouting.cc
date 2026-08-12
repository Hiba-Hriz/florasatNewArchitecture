/*
 * PacketHandlerRouting.cc
 *
 *  Created on: Feb 08, 2022
 *      Author: Robin Ohs
 */

#include "DtnPacketHandlerRouting.h"

namespace flora {
namespace satellite {

Define_Module(DtnPacketHandlerRouting);

void DtnPacketHandlerRouting::initialize(int stage) {
    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        this->eid_ = getSystemModule()->getSubmoduleVectorSize("groundStation") + this->getParentModule()->getIndex() + 1;
        INorad *noradModule = check_and_cast<INorad *>(getParentModule()->getSubmodule("NoradModule"));
        if (NoradA *noradAModule = dynamic_cast<NoradA *>(noradModule)) {
            satIndex = noradAModule->getSatelliteNumber();
        }
        // Store packetLoss probability
        this->packetLoss_ = par("packetLoss").doubleValue();
        this->contactTopology_ = check_and_cast<ContactPlan *>(getSystemModule()->getSubmodule("contactPlan"));
        this->topologyControl = check_and_cast<topologycontrol::DtnTopologyControl *>(getSystemModule()->getSubmodule("topologyControl"));
    }
}

isldirection::ISLDirection  DtnPacketHandlerRouting::getSatISLDirection(int satId, int nextSatId) {
    auto callerSatModule = this->topologyControl->getSatellite(satId);
    if (callerSatModule->hasLeftSat() && callerSatModule->getLeftSatId() == nextSatId) {
        return isldirection::ISLDirection(isldirection::Direction::ISL_LEFT, -1);
    } else if (callerSatModule->hasUpSat() && callerSatModule->getUpSatId() == nextSatId) {
        return isldirection::ISLDirection(isldirection::Direction::ISL_UP, -1);
    } else if (callerSatModule->hasRightSat() && callerSatModule->getRightSatId() == nextSatId) {
        return isldirection::ISLDirection(isldirection::Direction::ISL_RIGHT, -1);
    } else if (callerSatModule->hasDownSat() && callerSatModule->getDownSatId() == nextSatId) {
        return isldirection::ISLDirection(isldirection::Direction::ISL_DOWN, -1);
    }
    throw new cRuntimeError("Next routing direction was not available in satellite.");
}

cGate *DtnPacketHandlerRouting::getGate(isldirection::ISLDirection routingResult) {
    cGate *outputGate = nullptr;
    switch (routingResult.direction) {
        case isldirection::Direction::ISL_DOWN:
            outputGate = gate("down1$o");
            break;
        case isldirection::Direction::ISL_UP:
            outputGate = gate("up1$o");
            break;
        case isldirection::Direction::ISL_LEFT:
            outputGate = gate("left1$o");
            break;
        case isldirection::Direction::ISL_RIGHT:
            outputGate = gate("right1$o");
            break;
        case isldirection::Direction::ISL_DOWNLINK:
            outputGate = gate("groundLink1$o", routingResult.gateIndex);
            break;
        default:
            error("Unexpected gate");
    }
    ASSERT(outputGate != nullptr);
    return outputGate;
}

void DtnPacketHandlerRouting::handleMessage(cMessage *msg)
{
    if (msg->getKind() == BUNDLE || msg->getKind() == BUNDLE_CUSTODY_REPORT)
    {
        EV << "MESSAGE RECEIVED IN SAT" << endl;
        BundlePkt* bundle = check_and_cast<BundlePkt *>(msg);

        if (eid_ == bundle->getNextHopEid())
        {
            // This is an inbound message, check if packet was lost on the way
            if (packetLoss_ > uniform(0, 1.0))
            {
                // Packet was lost in the way, delete it
                cout << simTime() << " Node " << eid_ << " Bundle id " << bundle->getBundleId() << " lost on the way!" << endl;
                delete bundle;
            }
            else
            {
                // received correctly, send to Dtn layer
                send(msg, "dtnTransportOut");
            }
        }
        else
        {
            // This is an outbound message, perform a delayed send
            if (bundle->getNextHopEid() > getSystemModule()->getSubmoduleVectorSize("groundStation")) {
                // Sat to Sat connection
                int nextSatId = bundle->getNextHopEid() - getSystemModule()->getSubmoduleVectorSize("groundStation") - 1;
                isldirection::ISLDirection  nextSatDirection = getSatISLDirection(this->getParentModule()->getIndex(), nextSatId);
                cGate* gate = getGate(nextSatDirection);
                double linkDelay = contactTopology_->getRangeBySrcDst(eid_, bundle->getNextHopEid());
                if (linkDelay == -1)
                {
                    cout << "warning, range not available for nodes " << eid_ << "-" << bundle->getNextHopEid() << ", assuming range is 0" << endl;
                    linkDelay = 0;
                }
                sendDelayed(bundle, linkDelay, gate);
            } else {
                EV << "MESSAGE TO GROUNDLINLK" << endl;
                int gateIndex = topologyControl->getGroundstationSatConnection(bundle->getNextHopEid() - 1, this->getParentModule()->getIndex()).satGateIndex;
                double linkDelay = contactTopology_->getRangeBySrcDst(eid_, bundle->getNextHopEid());
                if (linkDelay == -1)
                {
                    cout << "warning, range not available for nodes " << eid_ << "-" << bundle->getNextHopEid() << ", assuming range is 0" << endl;
                    linkDelay = 0;
                }

                sendDelayed(bundle, linkDelay, "groundLink$o", gateIndex);
            }
        }
    }
}


}  // namespace satellite
}  // namespace flora
