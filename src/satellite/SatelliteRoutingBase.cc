/*
 * SatelliteRoutingBase.cc
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#include "satellite/SatelliteRoutingBase.h"

namespace flora {
namespace satellite {

Define_Module(SatelliteRoutingBase);

void SatelliteRoutingBase::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        noradModule = check_and_cast<NoradA*>(getSubmodule("NoradModule"));
        satId = getIndex();
        lowerLatitudeBound = par("lowerLatitudeBound");
        upperLatitudeBound = par("upperLatitudeBound");

        ASSERT(lowerLatitudeBound >= -90 && lowerLatitudeBound <= 90);
        ASSERT(upperLatitudeBound >= -90 && upperLatitudeBound <= 90);
        ASSERT(upperLatitudeBound >= lowerLatitudeBound);
    } else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
        // calculate plane
        int satsPerPlane = noradModule->getSatellitesPerPlane();
        satPlane = (satId - (satId % satsPerPlane)) / satsPerPlane;
        ASSERT(satPlane < noradModule->getNumberOfPlanes());
        ASSERT(satPlane >= 0);

        // calculate number in plane
        satNumberInPlane = satId % satsPerPlane;
        ASSERT(satNumberInPlane >= 0);
        ASSERT(satNumberInPlane < satsPerPlane);
    }
}

void SatelliteRoutingBase::finish() {
}

#ifndef NDEBUG
void SatelliteRoutingBase::refreshDisplay() const {
    char buf[40];

    int leftId = -1;
    if (hasLeftSat()) {
        leftId = getLeftSatId();
    }
    int upId = -1;
    if (hasUpSat()) {
        upId = getUpSatId();
    }
    int rightId = -1;
    if (hasRightSat()) {
        rightId = getRightSatId();
    }
    int downId = -1;
    if (hasDownSat()) {
        downId = getDownSatId();
    }
    sprintf(buf, "left: %d up: %d right: %d down: %d", leftId, upId, rightId, downId);
    getDisplayString().setTagArg("t", 0, buf);
}
#endif

std::pair<cGate*, ISLState> SatelliteRoutingBase::getInputGate(isldirection::Direction direction, int index) {
    return getGate(direction, cGate::Type::INPUT, index);
}

std::pair<cGate*, ISLState> SatelliteRoutingBase::getOutputGate(isldirection::Direction direction, int index) {
    return getGate(direction, cGate::Type::OUTPUT, index);
}

std::pair<cGate*, ISLState> SatelliteRoutingBase::getGate(isldirection::Direction direction, cGate::Type type, int index) {
    cGate* gate;
    ISLState state;
    switch (direction) {
        case isldirection::Direction::ISL_LEFT:
            gate = gateHalf(Constants::ISL_LEFT_NAME, type, -1);
            state = type == cGate::Type::OUTPUT ? getLeftSendState() : getLeftRecvState();
            break;
        case isldirection::Direction::ISL_UP:
            gate = gateHalf(Constants::ISL_UP_NAME, type, -1);
            state = type == cGate::Type::OUTPUT ? getUpSendState() : getUpRecvState();
            break;
        case isldirection::Direction::ISL_RIGHT:
            gate = gateHalf(Constants::ISL_RIGHT_NAME, type, -1);
            state = type == cGate::Type::OUTPUT ? getRightSendState() : getRightRecvState();
            break;
        case isldirection::Direction::ISL_DOWN:
            gate = gateHalf(Constants::ISL_DOWN_NAME, type, -1);
            state = type == cGate::Type::OUTPUT ? getDownSendState() : getDownRecvState();
            break;
        case isldirection::Direction::ISL_DOWNLINK:
            gate = gateHalf(Constants::SAT_GROUNDLINK_NAME, type, index);
            state = ISLState::WORKING;
            break;
        default:
            error("Unexpected gate");
    }
    return std::make_pair(gate, state);
}

bool SatelliteRoutingBase::hasLeftSat() const {
    return leftSatellite != nullptr;
}
bool SatelliteRoutingBase::hasUpSat() const {
    return upSatellite != nullptr;
}
bool SatelliteRoutingBase::hasRightSat() const {
    return rightSatellite != nullptr;
}
bool SatelliteRoutingBase::hasDownSat() const {
    return downSatellite != nullptr;
}

void SatelliteRoutingBase::setLeftSat(SatelliteRoutingBase* newSat) {
    ASSERT(newSat != nullptr);
    ASSERT(leftSatellite == nullptr);
    leftSatellite = newSat;
}
void SatelliteRoutingBase::setUpSat(SatelliteRoutingBase* newSat) {
    ASSERT(newSat != nullptr);
    ASSERT(upSatellite == nullptr);
    upSatellite = newSat;
}
void SatelliteRoutingBase::setRightSat(SatelliteRoutingBase* newSat) {
    ASSERT(newSat != nullptr);
    ASSERT(rightSatellite == nullptr);
    rightSatellite = newSat;
}
void SatelliteRoutingBase::setDownSat(SatelliteRoutingBase* newSat) {
    ASSERT(newSat != nullptr);
    ASSERT(downSatellite == nullptr);
    downSatellite = newSat;
}

void SatelliteRoutingBase::removeLeftSat() {
    ASSERT(leftSatellite != nullptr);
    leftSatellite = nullptr;
}
void SatelliteRoutingBase::removeUpSat() {
    ASSERT(upSatellite != nullptr);
    upSatellite = nullptr;
}
void SatelliteRoutingBase::removeRightSat() {
    ASSERT(rightSatellite != nullptr);
    rightSatellite = nullptr;
}
void SatelliteRoutingBase::removeDownSat() {
    ASSERT(downSatellite != nullptr);
    downSatellite = nullptr;
}

SatelliteRoutingBase* SatelliteRoutingBase::getLeftSat() const {
    ASSERT(hasLeftSat());
    return leftSatellite;
}
SatelliteRoutingBase* SatelliteRoutingBase::getUpSat() const {
    ASSERT(hasUpSat());
    return upSatellite;
}
SatelliteRoutingBase* SatelliteRoutingBase::getRightSat() const {
    ASSERT(hasRightSat());
    return rightSatellite;
}
SatelliteRoutingBase* SatelliteRoutingBase::getDownSat() const {
    ASSERT(hasDownSat());
    return downSatellite;
}

int SatelliteRoutingBase::getLeftSatId() const {
    ASSERT(hasLeftSat());
    return leftSatellite->getId();
}
int SatelliteRoutingBase::getUpSatId() const {
    ASSERT(hasUpSat());
    return upSatellite->getId();
}
int SatelliteRoutingBase::getRightSatId() const {
    ASSERT(hasRightSat());
    return rightSatellite->getId();
}
int SatelliteRoutingBase::getDownSatId() const {
    ASSERT(hasDownSat());
    return downSatellite->getId();
}

double SatelliteRoutingBase::getLeftSatDistance() const {
    ASSERT(hasLeftSat());
    return leftSatellite->getDistance(*this);
}
double SatelliteRoutingBase::getUpSatDistance() const {
    ASSERT(hasUpSat());
    return upSatellite->getDistance(*this);
}
double SatelliteRoutingBase::getRightSatDistance() const {
    ASSERT(hasRightSat());
    return rightSatellite->getDistance(*this);
}
double SatelliteRoutingBase::getDownSatDistance() const {
    ASSERT(hasDownSat());
    return downSatellite->getDistance(*this);
}

void SatelliteRoutingBase::setLeftSendState(ISLState newState) {
    leftSendState = newState;
}
void SatelliteRoutingBase::setLeftRecvState(ISLState newState) {
    leftRecvState = newState;
}
void SatelliteRoutingBase::setUpSendState(ISLState newState) {
    upSendState = newState;
}
void SatelliteRoutingBase::setUpRecvState(ISLState newState) {
    upRecvState = newState;
}
void SatelliteRoutingBase::setRightSendState(ISLState newState) {
    rightSendState = newState;
}
void SatelliteRoutingBase::setRightRecvState(ISLState newState) {
    rightRecvState = newState;
}
void SatelliteRoutingBase::setDownSendState(ISLState newState) {
    downSendState = newState;
}
void SatelliteRoutingBase::setDownRecvState(ISLState newState) {
    downRecvState = newState;
}

void SatelliteRoutingBase::setISLSendState(isldirection::Direction direction, ISLState state) {
    setISLState(direction, true, state);
}
void SatelliteRoutingBase::setISLRecvState(isldirection::Direction direction, ISLState state) {
    setISLState(direction, false, state);
}
void SatelliteRoutingBase::setISLState(isldirection::Direction direction, bool send, ISLState state) {
    EV << "[" << satId << "] Set " << to_string(direction) << " to " << to_string(state) << endl;
    switch (direction) {
        case isldirection::ISL_LEFT:
            send ? setLeftSendState(state) : setLeftRecvState(state);
            break;
        case isldirection::ISL_UP:
            send ? setUpSendState(state) : setUpRecvState(state);
            break;
        case isldirection::ISL_RIGHT:
            send ? setRightSendState(state) : setRightRecvState(state);
            break;
        case isldirection::ISL_DOWN:
            send ? setDownSendState(state) : setDownRecvState(state);
            break;
        case isldirection::ISL_DOWNLINK:
            error("Groundlink is not supported.");
        default:
            break;
    }
}

double SatelliteRoutingBase::getLongitude() const {
    return noradModule->getLongitude();
}
double SatelliteRoutingBase::getLatitude() const {
    return noradModule->getLatitude();
}
double SatelliteRoutingBase::getAltitude() const {
    return noradModule->getAltitude();
}

double SatelliteRoutingBase::getElevation(const PositionAwareBase& other) const {
    return ((INorad*)noradModule)->getElevation(other.getLatitude(), other.getLongitude(), other.getAltitude());
}

double SatelliteRoutingBase::getAzimuth(const PositionAwareBase& other) const {
    return ((INorad*)noradModule)->getAzimuth(other.getLatitude(), other.getLongitude(), other.getAltitude());
}

bool SatelliteRoutingBase::isAscending() const {
    return noradModule->isAscending();
}

bool SatelliteRoutingBase::isDescending() const {
    return !noradModule->isAscending();
}

bool SatelliteRoutingBase::isInterPlaneISLEnabled() const {
    double latitude = getLatitude();
    return latitude <= upperLatitudeBound && latitude >= lowerLatitudeBound;
}

std::string to_string(const SatelliteRoutingBase& satRoutingBase) {
    std::ostringstream ss;
    ss << satRoutingBase;
    return ss.str();
}

}  // namespace satellite
}  // namespace flora
