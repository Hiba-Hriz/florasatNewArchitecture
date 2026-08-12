/*
 * ConstellationTopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "ConstellationTopologyControl.h"

namespace flora {
namespace topologycontrol {

Define_Module(ConstellationTopologyControl);

void ConstellationTopologyControl::initialize(int stage) {
    TopologyControlBase::initialize(stage);

    // Constellation related initialization
}

void ConstellationTopologyControl::updateTopology() {
    core::Timer timer = core::Timer();
    // update ISL links and groundlinks
    topologyChanged = false;
    updateIntraSatelliteLinks();
    updateInterSatelliteLinks();
    updateGroundstationLinks();

    EV << "TC: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
    // if there was any change to the topology, track current contacts
    if (topologyChanged)
        trackTopologyChange();
}

void ConstellationTopologyControl::updateIntraSatelliteLinks() {
    // iterate over planes
    for (size_t plane = 0; plane < planeCount; plane++) {
        // iterate over sats in plane
        for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++) {
            int index = planeSat + plane * satsPerPlane;
            int isLastSatInPlane = index % satsPerPlane == satsPerPlane - 1;

            // get the two satellites we want to connect.
            // If we have the last in plane, we connect it to the first of the plane
            SatelliteRoutingBase *curSat = satellites.at(index);
            ASSERT(curSat != nullptr);
            int nextId = isLastSatInPlane ? plane * satsPerPlane : index + 1;
            SatelliteRoutingBase *otherSat = satellites.at(nextId);
            ASSERT(otherSat != nullptr);

            // connect the satellites
            connectSatellites(curSat, otherSat, isldirection::Direction::ISL_UP);
        }
    }
}

void ConstellationTopologyControl::updateGroundstationLinks() {
    // iterate over groundstations
    for (size_t gsId = 0; gsId < numGroundStations; gsId++) {
        GroundStationRoutingBase *gs = groundStations.at(gsId);
        ASSERT(gs != nullptr);
        for (size_t satId = 0; satId < numSatellites; satId++) {
            SatelliteRoutingBase *sat = satellites.at(satId);

            ASSERT(sat != nullptr);

            bool isOldConnection = gs->isConnectedTo(satId);

            // if is not in range continue with next sat
            if (sat->getElevation(*gs) < minimumElevation) {
                // if they were previous connected, delete that connection
                if (isOldConnection) {
                    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                    cGate *uplink = gs->getOutputGate(connection.gsGateIndex);
                    cGate *downlink = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
                    deleteChannel(uplink);
                    deleteChannel(downlink);
                    gsSatConnections.erase(std::pair<int, int>(gsId, satId));
                    gs->removeSatellite(satId);
                    topologyChanged = true;
                }
                // in all cases continue with next sat
                continue;
            }
            double delay = sat->getDistance(*gs) * groundlinkDelay;

            // if they were previous connected, update channel with new delay
            if (isOldConnection) {
                GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                cGate *uplinkO = gs->getOutputGate(connection.gsGateIndex);
                cGate *uplinkI = gs->getInputGate(connection.gsGateIndex);
                cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
                cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
            }
            // they were not previous connected, create new channel between gs and sat
            else {
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
        }
    }
}

void ConstellationTopologyControl::updateInterSatelliteLinks() {
    // if inter-plane ISL is not enabled/available
    if (interPlaneIslDisabled) return;

    switch (walkerType) {
        case WalkerType::DELTA:
            updateISLInWalkerDelta();
            break;
        case WalkerType::STAR:
            updateISLInWalkerStar();
            break;
        default:
            error("Error in ConstellationTopologyControl::updateInterSatelliteLinks(): Unexpected WalkerType '%s'.", to_string(walkerType).c_str());
    }
}

void ConstellationTopologyControl::updateISLInWalkerDelta() {
    for (size_t index = 0; index < numSatellites; index++) {
        SatelliteRoutingBase *curSat = satellites.at(index);
        ASSERT(curSat != nullptr);

        // sat (o, i) = i-th sat in plane o
        // F = interplanefacing angle
        int satPlane = curSat->getPlane();
        SatelliteRoutingBase *rightSat = (satPlane != planeCount - 1)
                                             ? findSatByPlaneAndNumberInPlane(satPlane + 1, curSat->getNumberInPlane())
                                             : findSatByPlaneAndNumberInPlane(0, (curSat->getNumberInPlane() + interPlaneSpacing) % satsPerPlane);
        ASSERT(rightSat != nullptr);

        if (curSat->isAscending()) {
            // if next plane partner is descending, connection is not possible
            if (rightSat->isDescending()) {
                // if we were connected to that satellite on right
                if (curSat->hasRightSat() && curSat->getRightSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_RIGHT);
                }
                // if we were connected to that satellite on left
                else if (curSat->hasLeftSat() && curSat->getLeftSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_LEFT);
                }
            } else {
                connectSatellites(curSat, rightSat, isldirection::Direction::ISL_RIGHT);
            }
        }
        // sat ist descending
        else {
            // if next plane partner is not descending, connection is not possible
            if (rightSat->isAscending()) {
                // if we were connected to that satellite on right
                if (curSat->hasLeftSat() && curSat->getLeftSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_LEFT);
                }
                // if we were connected to that satellite on right
                else if (curSat->hasRightSat() && curSat->getRightSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_RIGHT);
                }
            } else {
                connectSatellites(curSat, rightSat, isldirection::Direction::ISL_LEFT);
            }
        }
    }
}

void ConstellationTopologyControl::updateISLInWalkerStar() {
    for (size_t index = 0; index < numSatellites; index++) {
        SatelliteRoutingBase *curSat = satellites.at(index);
        ASSERT(curSat != nullptr);
        int satPlane = curSat->getPlane();

        bool isLastPlane = satPlane == planeCount - 1;

        // if last plane stop because we reached the seam
        if (isLastPlane)
            break;
        int rightIndex = (index + satsPerPlane) % numSatellites;
        SatelliteRoutingBase *nextPlaneSat = satellites.at(rightIndex);
        ASSERT(nextPlaneSat != nullptr);

        // if inter-plane isl is disabled for any satellite, just check to disconnect them
        bool curSatIsAscending = curSat->isAscending();
        bool nextPlaneSatIsAscending = nextPlaneSat->isAscending();
        if (!curSat->isInterPlaneISLEnabled() || !nextPlaneSat->isInterPlaneISLEnabled() || curSatIsAscending != nextPlaneSatIsAscending) {
            // if we were connected to that satellite on right
            if (curSat->hasRightSat() && curSat->getRightSatId() == nextPlaneSat->getId()) {
                disconnectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_RIGHT);
            }
            // if we were connected to that satellite on left
            else if (curSat->hasLeftSat() && curSat->getLeftSatId() == nextPlaneSat->getId()) {
                disconnectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_LEFT);
            }
        }
        // sats are moving up
        else if (curSatIsAscending) {
            connectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_RIGHT);
        }
        // sats are moving down
        else {
            connectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_LEFT);
        }
    }
}
}  // namespace topologycontrol
}  // namespace flora
