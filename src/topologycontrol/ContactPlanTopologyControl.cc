/*
 * TopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "ContactPlanTopologyControl.h"
#include <cmath>

namespace flora {
namespace topologycontrol {

Define_Module(ContactPlanTopologyControl);

void ContactPlanTopologyControl::initialize(int stage) {
    TopologyControlBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        maxRangeIsl = par("maxRangeIsl");
        maxRangeGs = par("maxRangeGs");
        contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
        ASSERT(contactPlan != nullptr);
    }
}

void ContactPlanTopologyControl::updateTopology() {
    core::Timer timer = core::Timer();
    // update ISL links and groundlinks
    topologyChanged = false;
    if (!interPlaneIslDisabled){
        updateInterSatelliteLinks();
    }
    updateGroundstationLinks();

    EV << "TC: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
    // if there was any change to the topology, track current contacts
    if (topologyChanged)
        trackTopologyChange();
}


/**
 * Updates the links between each Groundstation and Satellites based on a ContactPlan instance.
 */
void ContactPlanTopologyControl::updateGroundstationLinks() {
    EV << "updateGroundstationLinks: starting" << endl;
    for (size_t gsId = 0; gsId < numGroundStations; gsId++) {
        GroundStationRoutingBase *gs = groundStations.at(gsId);
        for (size_t satId = 0; satId < numSatellites; satId++) {
            SatelliteRoutingBase *sat = satellites.at(satId);
            if (isGroundStationContactValid(gs, sat) && contactStarts.count(std::pair<int, int>(gsId, getShiftedSatelliteId(satId))) == 0){
                EV << "Starting Contact: " << endl;
                if (simTime().dbl() != 0.0) {
                    contactStarts.emplace(std::pair<int, int>(gsId, getShiftedSatelliteId(satId)), simTime().dbl());
                }
            } else if (!isGroundStationContactValid(gs, sat) && contactStarts.count(std::pair<int, int>(gsId, getShiftedSatelliteId(satId))) > 0) {
                EV << "Ending Contact: " << endl;
                double startTime = contactStarts.at(std::pair<int, int>(gsId, getShiftedSatelliteId(satId)));
                double endTime = simTime().dbl();
                contactPlan->addContact(startTime, endTime, gsId + 1, getShiftedSatelliteId(satId) + 1, 1000, 1);
                contactPlan->addContact(startTime, endTime, getShiftedSatelliteId(satId) + 1, gsId + 1, 1000, 1);
                contactPlan->addRange(startTime, endTime, gsId + 1, getShiftedSatelliteId(satId) + 1, 0, 1);
                contactStarts.erase(std::pair<int, int>(gsId, getShiftedSatelliteId(satId)));
            }
        }
    }
    contactPlan->updateContactRanges();
    contactPlan->sortContactIdsBySrcByStartTime();
    contactPlan->printContactPlan();
}

void ContactPlanTopologyControl::updateInterSatelliteLinks() {
    EV << "updateInterSatelliteLinks: starting" << endl;
    for (size_t sat1Id = 0; sat1Id < numSatellites; sat1Id++) {
        SatelliteRoutingBase *sat1 = satellites.at(sat1Id);
        for (size_t sat2Id = 0; sat2Id < numSatellites; sat2Id++) {
            SatelliteRoutingBase *sat2 = satellites.at(sat2Id);
            int shiftedSat1 = getShiftedSatelliteId(sat1Id);
            int shiftedSat2 = getShiftedSatelliteId(sat2Id);
            if (isIslContactValid(sat1, sat2) && (contactStarts.count(std::pair<int, int>(shiftedSat1, shiftedSat2)) == 0 || contactStarts.count(std::pair<int, int>(shiftedSat2, shiftedSat1)) == 0)){
                EV << "Starting Contact: " << endl;
                if (simTime().dbl() != 0.0) {
                    contactStarts.emplace(std::pair<int, int>(shiftedSat1, shiftedSat2), simTime().dbl());
                }
            } else if (!isIslContactValid(sat1, sat2) && contactStarts.count(std::pair<int, int>(shiftedSat1, shiftedSat2)) > 0 && contactStarts.count(std::pair<int, int>(shiftedSat2, shiftedSat1)) > 0) {
                EV << "Ending Contact: " << endl;
                double startTime = contactStarts.at(std::pair<int, int>(shiftedSat1, shiftedSat2));
                double endTime = simTime().dbl();
                contactPlan->addContact(startTime, endTime, shiftedSat1 + 1, shiftedSat2 + 1, 1000, 1);
                contactPlan->addContact(startTime, endTime, shiftedSat2 + 1,shiftedSat1 + 1, 1000, 1);
                contactPlan->addRange(startTime, endTime, shiftedSat1 + 1, shiftedSat2 + 1, 0, 1);
                contactStarts.erase(std::pair<int, int>(shiftedSat1, shiftedSat2));
            }
        }
    }
    contactPlan->updateContactRanges();
    contactPlan->sortContactIdsBySrcByStartTime();
    contactPlan->printContactPlan();
}

bool ContactPlanTopologyControl::isGroundStationContactValid(GroundStationRoutingBase *gs, SatelliteRoutingBase *sat){
    double distance = sat->getDistance(*gs);
    EV << "Distance: " << distance << " (KM)" << " GS: " << gs->getId() + 1 << " Sat: " << getShiftedSatelliteId(sat->getId()) + 1 << endl;
    return distance < maxRangeGs;
}

bool ContactPlanTopologyControl::isIslContactValid(SatelliteRoutingBase *sat1, SatelliteRoutingBase *sat2){
    double distance = sat1->getDistance(*sat2);
    EV << "Distance: " << distance << " (KM)" << " Sat 1: " << getShiftedSatelliteId(sat1->getId()) + 1 << " Sat 2: " << getShiftedSatelliteId(sat2->getId()) + 1 << endl;
    return distance < maxRangeIsl;
}


int ContactPlanTopologyControl::getShiftedSatelliteId(int satId) {
    return numGroundStations + satId;
}

}  // namespace topologycontrol
}  // namespace flora
