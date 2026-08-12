/*
 * TopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "DtnTopologyControl.h"

namespace flora {
namespace topologycontrol {

Define_Module(DtnTopologyControl);

void DtnTopologyControl::initialize(int stage) {
    TopologyControlBase::initialize(stage);
}

void DtnTopologyControl::updateTopology() {
    core::Timer timer = core::Timer();
    // update ISL links and groundlinks
    topologyChanged = false;
    // updateIntraSatelliteLinks();
    updateInterSatelliteLinks();
    updateGroundstationLinksDtn();

    EV << "TC: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
    // if there was any change to the topology, track current contacts
    if (topologyChanged)
        trackTopologyChange();
}

void DtnTopologyControl::updateIntraSatelliteLinks() {
    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    ASSERT(contactPlan != nullptr);
}

/**
 * Updates the links between each Groundstation and Satellites based on a ContactPlan instance.
 */
void DtnTopologyControl::updateGroundstationLinksDtn() {
    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    if (contactPlan == nullptr) {
        error("Error in DtnTopologyControl::updateGroundstationLinksDtn(): contactPlan is nullptr. Make sure the module exists.");
    }
    for (size_t gsId = 1; gsId < numGroundStations + 1; gsId++) {
        for (size_t satId = 0; satId < numSatellites; satId++) {

            vector<Contact> satContacts = contactPlan->getContactsBySrcDst(gsId, getShiftedSatelliteId(satId));
            for (size_t i = 0; i < satContacts.size(); i++) {
                Contact contact = satContacts.at(i);
                int shiftedSatId = getShiftedSatelliteId(satId);
                // EV << "Shifted Sat ID: " << shiftedSatId << " Ground Station ID: " << gsId << endl;
                if (isDtnContactStarting(contact)){ // ACHF - Verifica que la Conexion haya iniciado
                    linkGroundStationToSatDtn(gsId - 1, satId );
                } else if (isDtnContactTakingPlace(gsId, shiftedSatId, contact)) { // Verifica que la conexion se este dando
                    updateLinkGroundStationToSatDtn(gsId - 1, satId);
                } else if (isDtnContactEnding(gsId, shiftedSatId, contact)) { // Verifica que la conexion haya terminado
                    unlinkGroundStationToSatDtn(gsId - 1, satId);
                }
            }
        }
    }
}

/**
 * Checks if a contact between a GroundStation and Satellite is starting based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is starting
 */
bool DtnTopologyControl::isDtnContactStarting(Contact contact) {
    return contact.getStart() == simTime().dbl();
}

/**
 * Checks if a contact between a GroundStation and Satellite is taking place based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is taking place
 */
bool DtnTopologyControl::isDtnContactTakingPlace(int gsId, int satId, Contact contact) {
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId && contact.getEnd() > simTime().dbl() && contact.getStart() < simTime().dbl();
}

/**
 * Checks if a contact between a GroundStation and Satellite is ending based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is ending
 */
bool DtnTopologyControl::isDtnContactEnding(int gsId, int satId, Contact contact) {
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId && contact.getEnd() == simTime().dbl();
}

/**
 * Creates a link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::linkGroundStationToSatDtn(int gsId, int satId) {
    GroundStationRoutingBase *gs = groundStations.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    double delay = sat->getDistance(*gs) * groundlinkDelay; // delay of the channel between satellite and groundstation (ms)
    EV << "Create channel between GS " << gsId << " and SAT " << satId << endl;
    int freeIndexGs = -1;
    for (size_t i = 0; i < numGroundLinks; i++) {
        cGate *gate = gs->getOutputGate(i);
        if (!gate->isConnectedOutside()) {
            freeIndexGs = i;
            break;
        }
    }
    if (freeIndexGs == -1) {
        error("No free gs gate index found.");
    }

    int freeIndexSat = -1;
    for (size_t i = 0; i < numGroundLinks; i++) {
        const cGate *gate = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, i).first;
        if (!gate->isConnectedOutside()) {
            freeIndexSat = i;
            break;
        }
    }
    if (freeIndexSat == -1) {
        error("No free sat gate index found.");
    }
    cGate *uplinkO = gs->getOutputGate(freeIndexGs);
    cGate *uplinkI = gs->getInputGate(freeIndexGs);
    cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat).first;
    cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat).first;
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
    gsSatConnections.emplace(std::pair<int, int>(gsId, satId), GsSatConnection(gsId, satId, freeIndexGs, freeIndexSat));
    gs->addSatellite(satId);
    topologyChanged = true;
}

/**
 * Deletes the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::unlinkGroundStationToSatDtn(int gsId, int satId) {
    GroundStationRoutingBase *gs = groundStations.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
    cGate *uplink= gs->getOutputGate(connection.gsGateIndex);
    cGate *downlink = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    deleteChannel(uplink);
    deleteChannel(downlink);
    gsSatConnections.erase(std::pair<int, int>(gsId, satId));
    gs->removeSatellite(satId);
    topologyChanged = true;
}

/**
 * Update the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::updateLinkGroundStationToSatDtn(int gsId, int satId) {
    GroundStationRoutingBase *gs = groundStations.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    double delay = sat->getDistance(*gs) * groundlinkDelay; // delay of the channel between nearest satellite and groundstation (ms)
    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
    cGate *uplinkO = gs->getOutputGate(connection.gsGateIndex);
    cGate *uplinkI = gs->getInputGate(connection.gsGateIndex);
    cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
}

int DtnTopologyControl::getShiftedSatelliteId(int satId) {
    return numGroundStations + satId + 1;
}

void DtnTopologyControl::updateInterSatelliteLinks() {
    // if inter-plane ISL is not enabled/available
    if (interPlaneIslDisabled) return;

    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    ASSERT(contactPlan != nullptr);

    for (size_t satId = 0; satId < numSatellites; satId++) {
        vector<Contact> satContacts = contactPlan->getContactsBySrc(getShiftedSatelliteId(satId));
        for (size_t contactIndex = 0; contactIndex < satContacts.size(); contactIndex++) {
            Contact contact = satContacts.at(contactIndex);
            if (contact.getSourceEid() >= numGroundStations + 1 && contact.getDestinationEid() >= numGroundStations + 1) {
                int unshiftedCurSatId = contact.getSourceEid()- numGroundStations - 1;
                int unshiftedOtherSatId = contact.getDestinationEid() - numGroundStations - 1;
                SatelliteRoutingBase *curSat = satellites.at(unshiftedCurSatId);
                SatelliteRoutingBase *otherSat = satellites.at(unshiftedOtherSatId);
                if (isDtnContactStarting(contact)) {
                    if (curSat->isAscending()) {
                        if (otherSat->hasLeftSat()) {
                            disconnectSatellites(otherSat, otherSat->getLeftSat(), isldirection::Direction::ISL_LEFT);
                        }
                        if (curSat->hasRightSat()) {
                            disconnectSatellites(curSat, curSat->getRightSat(), isldirection::Direction::ISL_RIGHT);
                        }
                        connectSatellites(curSat, otherSat, isldirection::Direction::ISL_RIGHT);
                    } else {
                        if (otherSat->hasRightSat()) {
                            disconnectSatellites(otherSat, otherSat->getRightSat(), isldirection::Direction::ISL_RIGHT);
                        }
                        if (curSat->hasLeftSat()) {
                            disconnectSatellites(curSat, curSat->getLeftSat(), isldirection::Direction::ISL_LEFT);
                        }
                        connectSatellites(curSat, otherSat, isldirection::Direction::ISL_LEFT);
                    }
                } else if (isDtnContactEnding(contact.getSourceEid(), contact.getDestinationEid(), contact)) {
                    EV << "Disconnecting : " << contact.getSourceEid() << " " << contact.getDestinationEid() << endl;
                    if (curSat->hasRightSat() && curSat->getRightSatId() == otherSat->getId()) {
                        disconnectSatellites(curSat, otherSat, isldirection::Direction::ISL_RIGHT);
                    } else if (curSat->hasLeftSat() && curSat->getLeftSatId() == otherSat->getId()) {
                        disconnectSatellites(curSat, otherSat, isldirection::Direction::ISL_LEFT);
                    } else {
                        EV << "Disconnecting : " << endl;
                    }
                }
                topologyChanged = true;
            }
        }
    }
}

}  // namespace topologycontrol
}  // namespace flora
