/*
 * SatelliteRouting.h
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_SATELLITEROUTING_H_
#define __FLORA_SATELLITE_SATELLITEROUTING_H_

#include <omnetpp.h>

#include "SatelliteRoutingBase.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "routing/RoutingHeader_m.h"

using namespace omnetpp;
using namespace inet;

namespace flora {
namespace satellite {

class SatelliteRouting : public SatelliteRoutingBase, cListener {
   private:
    // stats
    long numDroppedMaxHop;
    cOutVector droppedMaxHopCountStats;

    long numDroppedFullQueue;
    cOutVector droppedFullQueueCountStats;

    long numReceived;
    cOutVector receivedCountStats;

   public:
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

   protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void finish() override;
    virtual void initialize(int stage) override;

   private:
    void handlePacketDropped(inet::Packet *pkt, inet::PacketDropDetails *reason);
    void handlePacketReceived(inet::Packet *pkt);
};

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_SATELLITEROUTING_H_