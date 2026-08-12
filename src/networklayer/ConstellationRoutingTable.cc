/*
 * ConstellationRoutingTable.cc
 *
 *  Created on: Apr 24, 2023
 *      Author: Robin Ohs
 */

#include "ConstellationRoutingTable.h"

namespace flora {
namespace networklayer {

Define_Module(ConstellationRoutingTable);

void ConstellationRoutingTable::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        int numGs = getSystemModule()->getSubmoduleVectorSize("groundStation");
        for (size_t i = 0; i < numGs; i++) {
            cModule* gs = getSystemModule()->getSubmodule("groundStation", i);
            auto address = L3Address(gs->par("localAddress").stringValue());
            addEntry(i, address);
        }
    }
}

void ConstellationRoutingTable::addEntry(int gsId, L3Address address) {
    if (entries.find(gsId) != entries.end()) {
        error("Error in ConstellationRoutingTable::addEntry: Entry for %d already present.", gsId);
    }
    entries.emplace(gsId, address);
}

void ConstellationRoutingTable::removeEntry(int gsId, L3Address address) {
    if (entries.find(gsId) == entries.end()) {
        error("Error in ConstellationRoutingTable::addEntry: Entry for %d not present.", gsId);
    }
    entries.erase(gsId);
}

L3Address ConstellationRoutingTable::getAddressOfGroundstation(int gsId) {
    if (entries.find(gsId) == entries.end()) {
        error("Error in ConstellationRoutingTable::getAddressOfGroundstation: Entry for %d not present.", gsId);
    }
    return entries.at(gsId);
}

int ConstellationRoutingTable::getGroundstationFromAddress(L3Address address) {
    for (auto t : entries) {
        if (t.second == address) {
            return t.first;
        }
    }
    throw new cRuntimeError("Error in ConstellationRoutingTable::getGroundstationFromAddress: Entry for %s not present.", address.str().c_str());
}

}  // namespace networklayer
}  // namespace flora
