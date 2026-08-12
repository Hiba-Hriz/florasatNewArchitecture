/*
 * UniformGroundMobility.cc
 *
 *  Created on: Mar 24, 2022
 *      Author: diego
 */

#include "UniformGroundMobility.h"

#include <cmath>

#include "libnorad/cEcef.h"
#include "libnorad/globals.h"

namespace flora {

Define_Module(UniformGroundMobility);

UniformGroundMobility::UniformGroundMobility() {
    longitude = 0.0;
    latitude = 0.0;
    centerLatitude = 0.0;
    centerLongitude = 0.0;
    deploymentRadius = 0.0;
    mapx = 0;
    mapy = 0;
}

void UniformGroundMobility::initialize(int stage) {
    StationaryMobility::initialize(stage);
    EV << "initializing UniformGroundMobility stage " << stage << endl;
    if (stage == 0) {
        mapx = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
        mapy = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));
        centerLatitude = par("centerLatitude");
        centerLongitude = par("centerLongitude");
        deploymentRadius = par("deploymentRadius");

        double r = XKMPER_WGS72;  // earth radius
        double randomRadius = deploymentRadius * sqrt(cComponent::uniform(0, 1));
        double randomAngle = cComponent::uniform(0, TWOPI);

        // reference center point on Earth
        cEcef *P = new cEcef(centerLatitude, centerLongitude, r);
        Coord *Px = new Coord(P->getX(), P->getY(), P->getZ());

        // null island on Earth
        cEcef *O = new cEcef(0, 0, r);
        Coord *Ox = new Coord(O->getX(), O->getY(), O->getZ());

        // get cross and dot products
        Coord cross = *Ox % *Px;
        double angle = Px->angle(*Ox);
        cross.normalize();

        // get rotation quaternion
        Quaternion *q = new Quaternion(cross, angle);

        double xi = (randomRadius / r) * cos(randomAngle) / RADS_PER_DEG;  // longitude
        double yi = (randomRadius / r) * sin(randomAngle) / RADS_PER_DEG;  // latitude

        cEcef *pointEcef = new cEcef(yi, xi, r);
        Coord *pointCoord = new Coord(pointEcef->getX(), pointEcef->getY(), pointEcef->getZ());

        Coord rotatedCoord = q->rotate(*pointCoord);
        cEcef *rotatedEcef = new cEcef(rotatedCoord.getX(), rotatedCoord.getY(), rotatedCoord.getZ(), 0);

        longitude = rotatedEcef->getLongitude();
        latitude = rotatedEcef->getLatitude();

        delete P;
        delete O;
        delete Px;
        delete Ox;
        delete pointEcef;
        delete pointCoord;
        delete rotatedEcef;
    }
}

double UniformGroundMobility::getDistance(const double &refLatitude, const double &refLongitude, const double &refAltitude) const {
    // could change altitude to real value
    cEcef ecefSourceCoord = cEcef(latitude, longitude, 0);
    cEcef ecefDestCoord = cEcef(refLatitude, refLongitude, refAltitude);
    return ecefSourceCoord.getDistance(ecefDestCoord);
}

double UniformGroundMobility::getLongitude() const {
    return longitude;
}

double UniformGroundMobility::getLatitude() const {
    return latitude;
}

Coord &UniformGroundMobility::getCurrentPosition() {
    return lastPosition;
}

void UniformGroundMobility::setInitialPosition() {
    lastPosition.x = ((mapx * longitude) / 360) + (mapx / 2);
    lastPosition.x = static_cast<int>(lastPosition.x) % static_cast<int>(mapx);
    lastPosition.y = ((-mapy * latitude) / 180) + (mapy / 2);
    lastPosition = Coord(lastPosition.x, lastPosition.y, 0);
}

}  // namespace flora
