/*
 * GroundstationInfo.cc
 *
 * Created on: May 30, 2023
 *     Author: Sebastian Montoya
 */

#include "GroundStationRoutingBase.h"

namespace flora {
namespace ground {

Define_Module(GroundStationRoutingBase);

void GroundStationRoutingBase::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        groundStationId = getIndex();
        mobility = check_and_cast<GroundStationMobility*>(getSubmodule("gsMobility"));
        satellites = std::set<int>();
    }
}

cGate* GroundStationRoutingBase::getInputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
}

cGate* GroundStationRoutingBase::getOutputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
}

double GroundStationRoutingBase::getLongitude() const {
    return mobility->getLongitude();
}

double GroundStationRoutingBase::getLatitude() const {
    return mobility->getLatitude();
}

double GroundStationRoutingBase::getAltitude() const {
    return 0;
}

const std::set<int>& GroundStationRoutingBase::getSatellites() const {
    return satellites;
}

void GroundStationRoutingBase::addSatellite(int satId) {
    ASSERT(!core::utils::set::contains(satellites, satId));
    satellites.emplace(satId);
}

void GroundStationRoutingBase::removeSatellite(int satId) {
    ASSERT(core::utils::set::contains(satellites, satId));
    satellites.erase(satId);
}

bool GroundStationRoutingBase::isConnectedToAnySat() {
    return !satellites.empty();
}

bool GroundStationRoutingBase::isConnectedTo(int satId) {
    return core::utils::set::contains(satellites, satId);
}

std::string to_string(const GroundStationRoutingBase& gs) {
    std::ostringstream ss;
    ss << gs;
    return ss.str();
}

}  // namespace ground
}  // namespace flora
