//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "GroundStationMobility.h"
#include "../libnorad/cEcef.h"

namespace flora {

Define_Module(GroundStationMobility);

void GroundStationMobility::initialize(int stage) {
    StationaryMobility::initialize(stage);
    EV << "initializing LUTMotionMobility stage " << stage << endl;
    if (stage == 0) {
        mapx = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
        mapy = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));
        latitude = par("latitude");
        longitude = par("longitude");
    }
}

double GroundStationMobility::getLongitude() const {
    return longitude;
}

double GroundStationMobility::getLatitude() const {
    return latitude;
}

double GroundStationMobility::getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude) const {
    cEcef ecefSourceCoord = cEcef(getLatitude(), getLongitude(), 0);  // could change altitude to real value
    cEcef ecefDestCoord = cEcef(refLatitude, refLongitude, refAltitude);     // could change altitude to real value
    return ecefSourceCoord.getDistance(ecefDestCoord);
}

Coord& GroundStationMobility::getCurrentPosition() {
    return lastPosition;
}

void GroundStationMobility::setInitialPosition() {
    lastPosition.x = ((mapx * longitude) / 360) + (mapx / 2);
    lastPosition.x = static_cast<int>(lastPosition.x) % static_cast<int>(mapx);

    lastPosition.y = ((-mapy * latitude) / 180) + (mapy / 2);
    lastPosition = Coord((((mapx * longitude) / 360) + (mapx / 2)), (((-mapy * latitude) / 180) + (mapy / 2)), 0);
}

}  // namespace flora
