/*
 * Routing.h
 *
 *  Created on: July 13, 2017
 *      Author: fraverta
 *
 * This file declares an interface that any routing method must implement.
 *
 */

#ifndef __FLORA_ROUTING_NET_ROUTING_H_
#define __FLORA_ROUTING_NET_ROUTING_H_

#include <routing/dtn/contactplan/ContactPlan.h>
#include <routing/dtn/SdrModel.h>
#include <map>
#include <queue>
#include <limits>
#include <algorithm>
#include <unordered_set>
#include "routing/DtnRoutingHeader_m.h"

namespace flora {
namespace routing {

using namespace omnetpp;
using namespace std;

class Routing
{
public:
	Routing(int eid, SdrModel * sdr)
	{
		eid_ = eid;
		sdr_ = sdr;
	}
	virtual ~Routing()
	{
	}

	/**
	 * Method that will be called by Dtn module when a message to other destination in the
	 * network  arrives.
	 */
	virtual void msgToOtherArrive(BundlePkt *bundle, double simTime) = 0;

	/**
	 * Method that will be called by Dtn module when a message to this node arrives.
	 *
	 * @Return - true if bundle must be send to app layer.
	 */
	virtual bool msgToMeArrive(BundlePkt *bundle) = 0;

	/**
	 * Method that will be called by Dtn module when a contact starts.
	 */
	virtual void contactStart(Contact *c) = 0;

	/**
	 * Method that will be called by Dtn module when a contact ends.
	 */
	virtual void contactEnd(Contact *c) = 0;

	virtual void  refreshForwarding(Contact * c) = 0;

	/**
	 * Method to be called in case a contact plan was updated for opp. routing.
	 */
	virtual void updateContactPlan(Contact* c) = 0;

	/**
	 * Method that will be called by Dtn module when some bundle is forwarded successfully
	 */
	virtual void successfulBundleForwarded(long bundleId, Contact * contact, bool sentToDestination)=0;


protected:
	//Endpoint id
	int eid_;

	//Sdr model to enqueue bundles for transmission
	SdrModel * sdr_;
};

}  // namespace routing
}  // namespace flora

#endif /* __FLORA_ROUTING_NET_ROUTING_H_ */
