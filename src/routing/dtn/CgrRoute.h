/*
 * CgrRoute.h
 *
 * Created on: May 27, 2023
 *     Author: Sebastian Montoya
 */


#ifndef __FLORA_ROUTING_CGRROUTE_H_
#define __FLORA_ROUTING_CGRROUTE_H_

#include <routing/dtn/contactplan/Contact.h>
#include <vector>

namespace flora {
namespace routing {

#define NO_ROUTE_FOUND (-1)
#define EMPTY_ROUTE (-2)

typedef struct
{
	bool filtered;
	int terminusNode;			// Destination node
	int nextHop; 				// Entry node
	double fromTime; 			// Init time
	double toTime;	 			// Due time (earliest contact end time among all)
	float confidence;
	double arrivalTime;
	double maxVolume; 			// In Bytes
	double residualVolume;		// In Bytes
	vector<Contact *> hops;	 	// Contact list
} CgrRoute;

}  // namespace routing
}  // namespace flora

#endif /* __FLORA_ROUTING_CGRROUTE_H_ */
