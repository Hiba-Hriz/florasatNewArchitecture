/*
 * SatMobilityOrchestrator.cc
 *
 *  Created on: Feb 16, 2023
 *      Author: Robin Ohs
 */

#include "SatMobilityOrchestrator.h"

namespace flora {
namespace mobility {

Define_Module(SatMobilityOrchestrator);

SatMobilityOrchestrator::SatMobilityOrchestrator() : updateTimer(nullptr),
                                         updateIntervalParameter(0) {
}

SatMobilityOrchestrator::~SatMobilityOrchestrator() {
    cancelAndDeleteClockEvent(updateTimer);
}

void SatMobilityOrchestrator::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        updateIntervalParameter = &par("updateInterval");
        updateTimer = new inet::ClockEvent("UpdateTimer");
    } else if (stage == inet::INITSTAGE_GROUP_MOBILITY) {
        satMobVector = loadSatMobilities();
        updatePositions();
        if (!updateTimer->isScheduled()) {
            scheduleUpdate();
        }
    }
}

SatMobVector SatMobilityOrchestrator::loadSatMobilities() {
    cModule *parent = getParentModule();
    if (parent == nullptr) {
        error("Error in SatMobilityOrchestrator::loadSatellites: Could not find parent.");
    }
    int numSats = parent->getSubmoduleVectorSize("loRaGW");
    SatMobVector satellites;
    for (size_t i = 0; i < numSats; i++) {
        cModule *sat = parent->getSubmodule("loRaGW", i);
        if (sat == nullptr) {
            error("Error in SatMobilityOrchestrator::loadSatellites: Could not find sat with id %d.", i);
        }
        SatMobility *satMobility = check_and_cast<SatMobility *>(sat->getSubmodule("satMobility"));
        if (satMobility == nullptr) {
            error("Error in SatMobilityOrchestrator::loadSatellites: Could not find sat mobility for sat %d.", i);
        }
        satellites.emplace_back(satMobility);

        INorad *noradModule = check_and_cast<INorad *>(sat->getSubmodule("NoradModule"));
        if (satMobility == nullptr) {
            error("Error in SatMobilityOrchestrator::loadSatellites: Could not find sat norad module for sat %d.", i);
        }
        // noradModule->initializeMobility(simTime());
    }
    return satellites;
}

// int SatMobilityOrchestrator::calculateSliceCount(int satMobilityCount) {
//     return 12;
// }

// void SatMobilityOrchestrator::createSatSlices(int sliceCount, Slice slice) {
//     // fill the map with empty vectors to store the slices
//     for (size_t i = 0; i < sliceCount; i++) {
//         Slice t;
//         satMobilitySlices.emplace(i, t);
//     }
//     // distribute ptrs to the sat mobilities between all slices
//     size_t index = 0;
//     while (!slice.empty()) {
//         inet::SatelliteMobility* satMobility = slice.back();
//         slice.pop_back();
//         satMobilitySlices.at(index).push_back(satMobility);
//         index = ++index % sliceCount;
//     }
// }

void SatMobilityOrchestrator::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updatePositions();
        scheduleUpdate();
    } else
        error("SatMobilityOrchestrator: Unknown message.");
}

void SatMobilityOrchestrator::updatePositions() {
    EV << "Update satellite positions." << endl;
    core::Timer timer = core::Timer();
    SimTime currentTime = simTime();
    for (auto sm : satMobVector) {
        sm->updatePosition(currentTime);
    }
    EV << "FSM: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
}

void SatMobilityOrchestrator::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

}  // namespace mobility
}  // namespace flora