/*
 * TopologyControlBase.h
 *
 * Created on: May 05, 2023
 *     Author: Robin Ohs
 */

#include "TopologyControlBase.h"

namespace flora {
namespace topologycontrol {

Register_Abstract_Class(TopologyControlBase);

TopologyControlBase::TopologyControlBase() : topologyChanged(false),
                                             satsPerPlane(0),
                                             numGroundStations(0),
                                             numSatellites(0),
                                             planeCount(0),
                                             interPlaneSpacing(1),
                                             minimumElevation(10.0),
                                             numGroundLinks(40),
                                             groundlinkDatarate(0.0),
                                             groundlinkDelay(0.0),
                                             islDatarate(0.0),
                                             islDelay(0.0),
                                             interPlaneIslDisabled(false),
                                             walkerType(WalkerType::UNINITIALIZED),
                                             updateTimer(nullptr),
                                             updateIntervalParameter(0) {
}

TopologyControlBase::~TopologyControlBase() {
    cancelAndDeleteClockEvent(updateTimer);
}

void TopologyControlBase::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        // loaded properties
        updateIntervalParameter = &par("updateInterval");
        updateTimer = new ClockEvent("UpdateTimer");
        walkerType = WalkerType::parseWalkerType(par("walkerType"));
        interPlaneIslDisabled = par("interPlaneIslDisabled");
        islDelay = par("islDelay");
        islDatarate = par("islDatarate");
        groundlinkDelay = par("groundlinkDelay");
        groundlinkDatarate = par("groundlinkDatarate");
        numGroundLinks = par("numGroundLinks");
        minimumElevation = par("minimumElevation");
        interPlaneSpacing = par("interPlaneSpacing");
        planeCount = par("planeCount");
        EV << "TC: Loaded parameters: "
           << "updateInterval: " << updateIntervalParameter << "; "
           << "islDelay: " << islDelay << "; "
           << "islDatarate: " << islDatarate << "; "
           << "minimumElevation: " << minimumElevation << endl;
    } else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
        numSatellites = getSystemModule()->getSubmoduleVectorSize("loRaGW");
        numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");

        // calculated properties
        satsPerPlane = numSatellites / planeCount;

        // validate state
        VALIDATE(walkerType != WalkerType::UNINITIALIZED);
        VALIDATE(numGroundLinks > 0);
        VALIDATE(interPlaneSpacing <= planeCount - 1 && interPlaneSpacing >= 0);
        VALIDATE(numSatellites > 0);
        VALIDATE(planeCount > 0);
        VALIDATE(satsPerPlane > 0);
        VALIDATE(satsPerPlane * planeCount == numSatellites);
        VALIDATE(numGroundStations > 0);

        loadSatellites();
        loadGroundstations();

        if (satellites.size() == 0) {
            error("Error in TopologyControl::initialize(): No satellites found.");
            return;
        }

        updateTopology();

        if (!updateTimer->isScheduled()) {
            scheduleUpdate();
        }
    }
}

void TopologyControlBase::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updateTopology();
        scheduleUpdate();
    } else
        error("TopologyControlBase: Unknown message.");
}

void TopologyControlBase::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

GroundStationRoutingBase *const TopologyControlBase::getGroundstationInfo(int gsId) const {
    ASSERT(gsId >= 0 && gsId < numGroundStations);
    return groundStations.at(gsId);
}

SatelliteRoutingBase *const TopologyControlBase::getSatellite(int satId) const {
    ASSERT(satId >= 0 && satId < numSatellites);
    return satellites.at(satId);
}

std::unordered_map<int, SatelliteRoutingBase *> const &TopologyControlBase::getSatellites() const {
    return satellites;
}

GsSatConnection const &TopologyControlBase::getGroundstationSatConnection(int gsId, int satId) const {
    return gsSatConnections.at(std::pair<int, int>(gsId, satId));
}

SatelliteRoutingBase *const TopologyControlBase::findSatByPlaneAndNumberInPlane(int plane, int numberInPlane) const {
    int id = plane * satsPerPlane + numberInPlane;
    return satellites.at(id);
}

void TopologyControlBase::loadGroundstations() {
    groundStations.clear();
    for (size_t i = 0; i < numGroundStations; i++) {
        GroundStationRoutingBase *gs = check_and_cast<GroundStationRoutingBase *>(getParentModule()->getSubmodule("groundStation", i));
        EV << "TC: Loaded Groundstation " << i << endl;
        groundStations.emplace(i, gs);
    }
}

void TopologyControlBase::loadSatellites() {
    satellites.clear();
    for (size_t i = 0; i < numSatellites; i++) {
        SatelliteRoutingBase *sat = check_and_cast<SatelliteRoutingBase *>(getParentModule()->getSubmodule("loRaGW", i));
        satellites.emplace(i, sat);
    }
}

void TopologyControlBase::connectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction direction) {
    ASSERT(first != nullptr);
    ASSERT(second != nullptr);
    ASSERT(direction != isldirection::ISL_DOWNLINK);

    isldirection::Direction counterDirection = isldirection::getCounterDirection(direction);

    cGate *firstOut = first->getOutputGate(direction).first;
    cGate *firstIn = first->getInputGate(direction).first;
    cGate *secondOut = second->getOutputGate(counterDirection).first;
    cGate *secondIn = second->getInputGate(counterDirection).first;

    ASSERT(firstOut != nullptr);
    ASSERT(firstIn != nullptr);
    ASSERT(secondOut != nullptr);
    ASSERT(secondIn != nullptr);

    EV << "<><><><><><><><><><><><>" << endl;
    EV << "Connect " << first->getId() << " and " << second->getId() << " on " << to_string(direction) << endl;


    double distance = first->getDistance(*second);
    double delay = islDelay * distance;

    // Performs the following steps for each ISL direction (excluding groundlink)
    // 1. Delete the old connections if they are not the desired ones. If disconncts are happening, signal topology change.
    // 2. Create the satellite pair connections.
    // 3. Check if it is a new connection and if yes, signal topology change.
    // 4. Save the new satellite pair.
    switch (direction) {
        case isldirection::Direction::ISL_LEFT:
            // 1.
            if (second->hasRightSat() && second->getRightSatId() != first->getId()) {
                disconnectSatellites(second, second->getRightSat(), counterDirection);
            }
            if (first->hasLeftSat() && first->getLeftSatId() != second->getId()) {
                disconnectSatellites(first, first->getLeftSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getLeftSendState(), second->getRightRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getRightSendState(), first->getLeftRecvState());
            // 3.
            if (!first->hasLeftSat() || !second->hasRightSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setLeftSat(second);
            second->setRightSat(first);
            break;
        case isldirection::Direction::ISL_UP:
            // 1.
            if (second->hasDownSat() && second->getDownSatId() != first->getId()) {
                disconnectSatellites(second, second->getDownSat(), counterDirection);
            }
            if (first->hasUpSat() && first->getUpSatId() != second->getId()) {
                disconnectSatellites(first, first->getUpSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getUpSendState(), second->getDownRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getDownSendState(), first->getUpRecvState());
            // 3.
            if (!first->hasUpSat() || !second->hasDownSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setUpSat(second);
            second->setDownSat(first);
            break;
        case isldirection::Direction::ISL_RIGHT:
            // 1.
            if (second->hasLeftSat() && second->getLeftSatId() != first->getId()) {
                disconnectSatellites(second, second->getLeftSat(), counterDirection);
            }
            if (first->hasRightSat() && first->getRightSatId() != second->getId()) {
                disconnectSatellites(first, first->getRightSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getRightSendState(), second->getLeftRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getLeftSendState(), first->getRightRecvState());
            // 3.
            if (!first->hasRightSat() || !second->hasLeftSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setRightSat(second);
            second->setLeftSat(first);
            break;
        case isldirection::Direction::ISL_DOWN:
            // 1.
            if (second->hasUpSat() && second->getUpSatId() != first->getId()) {
                disconnectSatellites(second, second->getUpSat(), counterDirection);
            }
            if (first->hasDownSat() && first->getDownSatId() != second->getId()) {
                disconnectSatellites(first, first->getDownSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getDownSendState(), second->getUpRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getUpSendState(), first->getDownRecvState());
            // 3.
            if (!first->hasDownSat() || !second->hasUpSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setDownSat(second);
            second->setUpSat(first);
            break;
        default:
            error("Error in TopologyControlBase::connectSatellites: Should not reach default branch of switch.");
            break;
    }
}

void TopologyControlBase::disconnectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction direction) {
    ASSERT(direction != isldirection::ISL_DOWNLINK);

#ifndef NDEBUG
    EV << "Disconnect " << first->getId() << " and " << second->getId() << " on " << to_string(direction) << endl;
#endif

    cGate *firstOut = first->getOutputGate(direction).first;
    cGate *secondOut = second->getOutputGate(isldirection::getCounterDirection(direction)).first;

    ASSERT(firstOut != nullptr);
    ASSERT(secondOut != nullptr);

    firstOut->disconnect();
    secondOut->disconnect();

    topologyChanged = true;

    switch (direction) {
        case isldirection::Direction::ISL_LEFT:
            first->removeLeftSat();
            second->removeRightSat();
            break;
        case isldirection::Direction::ISL_UP:
            first->removeUpSat();
            second->removeDownSat();
            break;
        case isldirection::Direction::ISL_RIGHT:
            first->removeRightSat();
            second->removeLeftSat();
            break;
        case isldirection::Direction::ISL_DOWN:
            first->removeDownSat();
            second->removeUpSat();
            break;
        default:
            error("Error in TopologyControlBase::disconnectSatellites: Should not reach default branch of switch.");
            break;
    }
}

void TopologyControlBase::createSatelliteConnection(cGate *outGate, cGate *inGate, double delay, double datarate, ISLState outState, ISLState inState) {
    if (outState == ISLState::WORKING && inState == ISLState::WORKING) {
        updateOrCreateChannel(outGate, inGate, delay, islDatarate);
    } else {
        outGate->disconnect();
    }
}

ChannelState TopologyControlBase::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate) {
    ASSERT(outGate != nullptr);
    ASSERT(inGate != nullptr);
    ASSERT(delay > 0.0);
    ASSERT(datarate > 0.0);

    cGate *nextGate = outGate->getNextGate();
    if (nextGate != nullptr && nextGate->getId() == inGate->getId()) {
        ASSERT(outGate->isConnectedOutside());
#ifndef NDEBUG
        EV << "Update channel: " << ((SatelliteRoutingBase *)outGate->getOwnerModule())->getId() << "->" << ((SatelliteRoutingBase *)inGate->getOwnerModule())->getId() << endl;
#endif
        cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        return ChannelState::UPDATED;
    } else {
        ASSERT(!outGate->isConnectedOutside());
#ifndef NDEBUG
        EV << "Create channel: " << ((SatelliteRoutingBase *)outGate->getOwnerModule())->getId() << "->" << ((SatelliteRoutingBase *)inGate->getOwnerModule())->getId() << endl;
#endif
        cDatarateChannel *channel = cDatarateChannel::create(Constants::ISL_CHANNEL_NAME);
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        outGate->connectTo(inGate, channel);
        channel->callInitialize();
        return ChannelState::CREATED;
    }
}

ChannelState TopologyControlBase::deleteChannel(cGate *outGate) {
    if (outGate != nullptr && outGate->isConnectedOutside()) {
        outGate->disconnect();
        return ChannelState::DELETED;
    }
    return ChannelState::UNCHANGED;
}

}  // namespace topologycontrol
}  // namespace flora
