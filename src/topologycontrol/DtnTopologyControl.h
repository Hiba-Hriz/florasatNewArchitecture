/*
 * DtnTopologyControl.h
 *
 * Created on: May 17, 2023
 *     Author: Sebastian Montoya
 */

#ifndef __FLORA_TOPOLOGYCONTROL_DTNTOPOLOGYCONTROL_H_
#define __FLORA_TOPOLOGYCONTROL_DTNTOPOLOGYCONTROL_H_

#include "core/Timer.h"
#include "routing/dtn/contactplan/ContactPlan.h"
#include "topologycontrol/TopologyControlBase.h"
#include <iostream>
#include <map>
#include <utility>

using namespace omnetpp;
using namespace flora::satellite;
using namespace flora::core;

namespace flora {
namespace topologycontrol {

class DtnTopologyControl : public TopologyControlBase {

   protected:
    virtual void initialize(int stage) override;
    virtual void updateTopology() override;

   private:
    void updateIntraSatelliteLinks();
    void updateInterSatelliteLinks();
    void updateGroundstationLinksDtn();
    void updateISLInWalkerDelta();
    void updateISLInWalkerStar();
    void linkGroundStationToSatDtn(int gsId, int satId);
    void unlinkGroundStationToSatDtn(int gsId, int satId);
    void updateLinkGroundStationToSatDtn(int gsId, int satId);
    bool isDtnContactStarting(Contact contact);
    bool isDtnContactTakingPlace(int gsId, int satId, Contact contact);
    bool isDtnContactEnding(int gsId, int satId, Contact contact);
    int getShiftedSatelliteId(int satId);
    std::map<std::pair<int, int>, bool> satConnections;
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_DTNTOPOLOGYCONTROL_H_
