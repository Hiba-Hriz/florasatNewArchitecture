#include "routing/dtn/com/Com.h"

namespace flora {
namespace routing {

Define_Module(Com);

void Com::initialize(int stage)
{
    if (stage == INITSTAGE_LAST)
    {
        // Store this node eid
        this->eid_ = this->getParentModule()->getIndex() + 1;
        // Store packetLoss probability
        this->packetLoss_ = par("packetLoss").doubleValue();
        this->contactTopology_ = check_and_cast<ContactPlan *>(getSystemModule()->getSubmodule("contactPlan"));
        this->topologyControl = check_and_cast<topologycontrol::TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
    }
}

void Com::handleMessage(cMessage *msg)
{
	if (msg->getKind() == BUNDLE || msg->getKind() == BUNDLE_CUSTODY_REPORT)
	{
	    EV << "MESSAGE RECEIVED IN COM" << endl;
		BundlePkt* bundle = check_and_cast<BundlePkt *>(msg);

		if (eid_ == bundle->getNextHopEid())
		{
		    EV << "DESTINATION MESSAGE" << endl;
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
		    EV << "OUTBOUND MESSAGE: " << " nextHopEID: " << bundle->getNextHopEid() << endl;
			// This is an outbound message, perform a delayed send
			int gateIndex = topologyControl->getGroundstationSatConnection(this->getParentModule()->getIndex(), bundle->getNextHopEid() - getSystemModule()->getSubmoduleVectorSize("groundStation") - 1).gsGateIndex;
			double linkDelay = contactTopology_->getRangeBySrcDst(eid_, bundle->getNextHopEid());
			EV << "GATE INDEX: " << gateIndex << endl;
			if (linkDelay == -1)
			{
				cout << "warning, range not available for nodes " << eid_ << "-" << bundle->getNextHopEid() << ", assuming range is 0" << endl;
				linkDelay = 0;
			}

			sendDelayed(bundle, linkDelay, "satelliteLink$o", gateIndex);
		}
	}
}

} // namespace routing
} // namespace flora

