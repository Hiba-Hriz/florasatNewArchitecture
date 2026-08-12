#include "SatMobility.h"

namespace flora {
namespace mobility {

Define_Module(SatMobility);

SatMobility::SatMobility() {
    mapX = 0;
    mapY = 0;
}

void SatMobility::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        MobilityBase::initialize(0);
        WATCH(currentPosition);
        WATCH(lastPosition);
        WATCH(currentOrientation);
        WATCH(lastOrientation);
        mapX = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
        mapY = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));
    } else if (stage == inet::INITSTAGE_GROUP_MOBILITY) {
        (check_and_cast<INorad*>(getParentModule()->getSubmodule("NoradModule")))->initializeMobility(SimTime::ZERO);
    }
}

void SatMobility::handleSelfMessage(cMessage* message) {
    throw new cRuntimeError("Error in SatMobility::handleSelfMessage: this module should not receive messages.");
}

void SatMobility::updatePosition(SimTime currentTime) {
    INorad* norad = check_and_cast<INorad*>(getParentModule()->getSubmodule("NoradModule"));
    norad->updateTime(currentTime);
    lastPosition.x = currentPosition.x;
    lastPosition.y = currentPosition.y;
    lastPosition.z = currentPosition.z;
    currentPosition.x = getXCanvas(norad->getLongitude());  // x canvas position, longitude projection
    currentPosition.y = getYCanvas(norad->getLatitude());   // y canvas position, latitude projection
    currentPosition.z = norad->getAltitude();               // real satellite altitude in km

    velocity = (currentPosition - lastPosition) / (currentTime - lastUpdate).dbl();
    orient();

    raiseErrorIfOutside();
    emitMobilityStateChangedSignal();
}

void SatMobility::orient() {
    if (faceForward) {
        // determine orientation based on direction
        if (velocity != inet::Coord::ZERO) {
            inet::Coord direction = velocity;
            direction.normalize();
            auto alpha = inet::rad(atan2(direction.y, direction.x));
            auto beta = inet::rad(-asin(direction.z));
            auto gamma = inet::rad(0.0);
            lastOrientation = inet::Quaternion(inet::EulerAngles(alpha, beta, gamma));
        }
    }
}

double SatMobility::getXCanvas(double lon) const {
    double x = mapX * lon / 360 + (mapX / 2);
    return static_cast<int>(x) % static_cast<int>(mapX);
}

double SatMobility::getYCanvas(double lat) const {
    return ((-mapY * lat) / 180) + (mapY / 2);
}

const inet::Coord& SatMobility::getCurrentPosition() {
    updatePosition(simTime());
    return currentPosition;
}

const inet::Coord& SatMobility::getCurrentVelocity() {
    updatePosition(simTime());
    return velocity;
}

const inet::Quaternion& SatMobility::getCurrentAngularPosition() {
    updatePosition(simTime());
    return currentOrientation;
}

const inet::Quaternion& SatMobility::getCurrentAngularVelocity() {
    updatePosition(simTime());
    return angularVelocity;
}

}  // namespace mobility
}  // namespace flora
