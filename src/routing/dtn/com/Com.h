#ifndef __FLORA_ROUTING_DTN_COM_H_
#define __FLORA_ROUTING_DTN_COM_H_

#include "routing/dtn/contactplan/ContactPlan.h"
#include "topologycontrol/DtnTopologyControl.h"

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <fstream>
#include <iomanip>

#include "routing/dtn/MsgTypes.h"
#include "routing/DtnRoutingHeader_m.h"

namespace flora {
namespace routing {

using namespace std;
using namespace omnetpp;

class Com: public cSimpleModule
{
protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;

private:

	int eid_;
	ContactPlan* contactTopology_;
	topologycontrol::TopologyControlBase* topologyControl;
	double packetLoss_;

};

} // namespace flora
} // namespace routing
#endif /* __FLORA_ROUTING_DTN_COM_H_ */
