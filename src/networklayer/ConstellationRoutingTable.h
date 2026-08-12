/*
 * ConstellationRoutingTable.h
 *
 *  Created on: Apr 24, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_NETWORKLAYER_CONSTELLATIONROUTINGTABLE_H_
#define __FLORA_NETWORKLAYER_CONSTELLATIONROUTINGTABLE_H_

#include <omnetpp.h>

#include <unordered_map>

#include "inet/networklayer/common/L3Address.h"

using namespace omnetpp;
using namespace inet;

namespace flora {
namespace networklayer {

class ConstellationRoutingTable : public cSimpleModule {
   protected:
    std::unordered_map<int, L3Address> entries;

   public:
    void addEntry(int gsId, L3Address address);
    void removeEntry(int gsId, L3Address address);
    L3Address getAddressOfGroundstation(int gsId);
    int getGroundstationFromAddress(L3Address address);

   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

}  // namespace networklayer
}  // namespace flora

#endif  // __FLORA_NETWORKLAYER_CONSTELLATIONROUTINGTABLE_H_
