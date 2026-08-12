//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 
import inet.common.INETDefs;
import inet.common.Units;
import inet.common.packet.chunk.Chunk;

cplusplus {{
using namespace inet;
#include "routing/dtn/CgrRoute.h"
typedef std::list<int> intList;
}}

class intList
{
    @existingClass;
}

namespace flora;

class routing::CgrRoute
{
    @existingClass;
}

packet BundlePkt{
		////////////////////////////////////////////////    
    	// Bundle Protocol fields (set by source node)
    	////////////////////////////////////////////////
    	long bundleId;
    	int sourceEid;
        int destinationEid;
        bool critical;
        simtime_t creationTimestamp;
        simtime_t ttl;
        bool returnToSender;
        bool custodyTransferRequested;
        
        // Source routing path
        routing::CgrRoute cgrRoute;
        
        // Bundle is a report
        bool bundleIsCustodyReport;
        bool custodyAccepted;
        long custodyBundleId;
        
        ////////////////////////////////////////////////
        // Bundle meta-data (set by intermediate nodes)
        ////////////////////////////////////////////////
        int custodianEid;
        int senderEid;
        int nextHopEid;
        int hopCount;
        intList visitedNodes;
        
        // Probabilistic routing meta-data
        int xmitCopiesCount;
        double dlvConfidence;
        
        //Spray And Wait number of copies variable
        int bundlesCopies;
        
        //Proactive CGR Routing specific use variable 
        // 0 to route by delivery time
        // 1 to route by hops
        // 2 to route by hops and delivery time (making a copy)
        int qos;
        
};

message TrafficGeneratorMsg{
    int bundlesNumber;
    int destinationEid;
    int size;
    double ttl;
    double interval;
};

message ContactMsg {
    int id;
    double dataRate;
    simtime_t start;
    simtime_t end;
    simtime_t duration;
    int sourceEid;
    int destinationEid;
};

message ForwardingMsgEnd {
    int neighborEid;
    int contactId;
    long bundleId;
    bool sentToDestination;
};

message ForwardingMsgStart {
    int neighborEid;
    int contactId;
};

message CustodyTimeout {
    int bundleId;
};