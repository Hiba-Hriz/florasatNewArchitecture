/*
 * GsSatConnection.h
 *
 * Created on: Feb 23, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_GSSATCONNECTIONINFO_H_
#define __FLORA_TOPOLOGYCONTROL_GSSATCONNECTIONINFO_H_

#include <omnetpp.h>

#include <sstream>

using namespace omnetpp;

namespace flora {
namespace topologycontrol {

struct GsSatConnection {
    int gsId;
    int gsGateIndex = -1;
    int satId;
    int satGateIndex = -1;

    GsSatConnection(int gsId, int satId, int gsGateIndex, int satGateIndex)
        : gsId(gsId),
          gsGateIndex(gsGateIndex),
          satId(satId),
          satGateIndex(satGateIndex){};

    friend std::ostream &operator<<(std::ostream &ss, const GsSatConnection &gs) {
        ss << "{";
        ss << "\"gsId\": " << gs.gsId << ",";
        ss << "\"gsGateIndex\": " << gs.gsGateIndex << ",";
        ss << "\"satId\": " << gs.satId << ",";
        ss << "\"satGateIndex\": " << gs.satGateIndex << ",";
        ss << "}";
        return ss;
    }
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_GSSATCONNECTIONINFO_H_