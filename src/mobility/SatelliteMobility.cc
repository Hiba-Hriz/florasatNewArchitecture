#include "SatelliteMobility.h"

namespace flora {

Define_Module(SatelliteMobility);

SatelliteMobility::SatelliteMobility() {
    noradModule = nullptr;
    mapX = 0;
    mapY = 0;
    transmitPower = 0.0;
}

void SatelliteMobility::initialize(int stage) {
    // noradModule must be initialized before LineSegmentsMobilityBase calling setTargetPosition() in its initialization at stage 1
    if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT) {
        noradModule->initializeMobility(nextChange);
    }
    LineSegmentsMobilityBase::initialize(stage);
    noradModule = check_and_cast<INorad*>(getParentModule()->getSubmodule("NoradModule"));
    if (noradModule == nullptr) {
        error("Error in SatelliteMobility::initializeMobility(): Cannot find module Norad.");
    }

    // get current time as an integral value holding the num of secs since 00:00, Jan 1 1970 UTC
    // std::time_t timestamp =  1577904000;              // 01-01-2020 18:40:00 UTC
    // std::tm* currentTime = std::gmtime(&timestamp);   // convert timestamp into structure holding a calendar date and time
    // noradModule->setJulian(currentTime);

    mapX = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
    mapY = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));

    EV << "initializing SatelliteMobility stage " << stage << endl;
    WATCH(lastPosition);

    effectiveSlant = par("effectiveSlant");
    displaySpanArea = par("displaySpanArea");
    if (stage == INITSTAGE_LAST && displaySpanArea) {
        refreshArea = new cMessage("refreshArea");
        polygon = new cPolygonFigure("polygon");
        polygon->setNumPoints(nPoints);
        polygon->setSmooth(true);
        polygon->setFilled(true);
        polygon->setLineWidth(1);
        polygon->setFillOpacity(0.15);
        polygon->setLineColor(cFigure::RED);
        polygon->setFillColor(cFigure::RED);
        setAllPoints();
        networkCanvas = getParentModule()->getParentModule()->getCanvas();
        networkCanvas->addFigure(polygon);

        EV << "satellite position is lat= " << getLatitude() << "; lon= " << getLongitude() << endl;
    }
}

void SatelliteMobility::initializePosition() {
    nextChange = simTime();
    LineSegmentsMobilityBase::initializePosition();
}

bool SatelliteMobility::isOnSameOrbitalPlane(double raan2, double inclination2) {
    if (NoradA* noradAModule = dynamic_cast<NoradA*>(noradModule)) {
        double raan = noradAModule->getRaan();
        double inclination = noradAModule->getInclination();
        if ((inclination == inclination2) && (raan == raan2)) {
            return true;
        }
    }
    return false;
}

double SatelliteMobility::getAltitude() const {
    return noradModule->getAltitude();
}

double SatelliteMobility::getElevation(const double& refLatitude, const double& refLongitude,
                                       const double& refAltitude) const {
    return noradModule->getElevation(refLatitude, refLongitude, refAltitude);
}

double SatelliteMobility::getAzimuth(const double& refLatitude, const double& refLongitude,
                                     const double& refAltitude) const {
    return noradModule->getAzimuth(refLatitude, refLongitude, refAltitude);
}

double SatelliteMobility::getDistance(const double& refLatitude, const double& refLongitude,
                                      const double& refAltitude) const {
    return noradModule->getDistance(refLatitude, refLongitude, refAltitude);
}

double SatelliteMobility::getLongitude() const {
    return noradModule->getLongitude();
}

double SatelliteMobility::getLatitude() const {
    return noradModule->getLatitude();
}

void SatelliteMobility::setTargetPosition() {
    nextChange += updateInterval.dbl();
    noradModule->updateTime(nextChange);
    lastPosition.x = getXCanvas(getLongitude());  // x canvas position, longitude projection
    lastPosition.y = getYCanvas(getLatitude());   // y canvas position, latitude projection
    lastPosition.z = getAltitude();               // real satellite altitude in km
    targetPosition.x = lastPosition.x;
    targetPosition.y = lastPosition.y;
    targetPosition.z = lastPosition.z;
    EV << "[DEBUG] t=" << nextChange
           << " lat=" << getLatitude()
           << " lon=" << getLongitude()
           << " alt=" << getAltitude() << endl;
}

double SatelliteMobility::getXCanvas(double lon) const {
    double x = mapX * lon / 360 + (mapX / 2);
    return static_cast<int>(x) % static_cast<int>(mapX);
}

double SatelliteMobility::getYCanvas(double lat) const {
    return ((-mapY * lat) / 180) + (mapY / 2);
}

void SatelliteMobility::move() {
    LineSegmentsMobilityBase::move();
    raiseErrorIfOutside();
}

void SatelliteMobility::handleSelfMessage(cMessage* msg) {
    moveAndUpdate();
    scheduleUpdate();
    if (displaySpanArea) {
        polygon->setVisible(false);
        removeAllPoints();
        setAllPoints();
        polygon->setVisible(true);
    }
}

void SatelliteMobility::removeAllPoints() {
    for (int i = 0; i < nPoints; i++)
        polygon->removePoint(0);
    polygon->setNumPoints(nPoints);
}

void SatelliteMobility::setAllPoints() {
    double alt = lastPosition.z;  // altitude
    double r = XKMPER_WGS72;      // earth radius
    double r2 = r * r;
    double alt2 = alt * alt;
    double hr = alt * r;
    double slant = sqrt(alt2 + 2 * hr);  // slant range
    double alpha = atan(slant / r);      // view angle
    double b, xi, yi, xCanvas, yCanvas;

    // satellite projection on Earth
    cEcef* P = new cEcef(getLatitude(), getLongitude(), r);
    Coord* Px = new Coord(P->getX(), P->getY(), P->getZ());

    // null island on Earth
    cEcef* O = new cEcef(0, 0, r);
    Coord* Ox = new Coord(O->getX(), O->getY(), O->getZ());

    // get cross and dot products
    Coord cross = *Ox % *Px;
    double angle = Px->angle(*Ox);
    cross.normalize();

    // get rotation quaternion
    Quaternion* q = new Quaternion(cross, angle);

    delete P;
    delete O;
    delete Px;
    delete Ox;

    // effective area considering max range given lora parameters
    double slantAngle = alpha;
    if (effectiveSlant > 0) {
        double x = 2 * r2 + 2 * hr;
        double z = (x + alt2 - effectiveSlant * effectiveSlant) / x;
        double beta = acos(z);
        slantAngle = beta;
    }

    // solve span area perimeter points centered around O, then apply rotation
    for (int i = 0; i < nPoints - 1; i++) {
        b = i * TWOPI / nPoints;
        xi = slantAngle * cos(b) / RADS_PER_DEG;  // longitude
        yi = slantAngle * sin(b) / RADS_PER_DEG;  // latitude

        cEcef* pointEcef = new cEcef(yi, xi, r);
        Coord* pointCoord = new Coord(pointEcef->getX(), pointEcef->getY(), pointEcef->getZ());

        Coord rotatedCoord = q->rotate(*pointCoord);
        cEcef* rotatedEcef = new cEcef(rotatedCoord.getX(), rotatedCoord.getY(), rotatedCoord.getZ(), 0);

        xCanvas = getXCanvas(rotatedEcef->getLongitude());
        yCanvas = getYCanvas(rotatedEcef->getLatitude());
        polygon->setPoint(i, cFigure::Point(xCanvas, yCanvas));

        delete pointEcef;
        delete pointCoord;
        delete rotatedEcef;

        // EV << "point lon: " << rotatedEcef->getLongitude() << "; point lat: " << rotatedEcef->getLatitude()<< endl;
    }

    polygon->setPoint(nPoints - 1, polygon->getPoint(0));
}

void SatelliteMobility::fixIfHostGetsOutside() {
    raiseErrorIfOutside();
}

}  // namespace flora
