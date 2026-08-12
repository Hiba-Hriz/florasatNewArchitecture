#include "cEcef.h"
#include "globals.h"
#include "ccoord.h"
#include <math.h>
//////////////////////////////////////////////////////////////////////
// cEcef(cCoordGeo&)
// Calculate the ECEF coordinates of a location, for simple distance calculations.
// Measurement is in meters unlike OS3 being in kilometers, therefore the altitude is translated.
// Reference: https://www2.unb.ca/gge/Pubs/LN39.pdf (GEODETIC POSITIONS COMPUTATIONS by E. J. KRAKIWSKY and D. B. THOMSON)
// Written by Aiden Valentine
cEcef::cEcef() {
    x = 0;
    y = 0;
    z = 0;
    latitude = 0;
    longitude = 0;
    altitude = 0;
}

/**
 * Constructor which, upon being given longitude,latitude and altitude coordinates, calculates the ECEF coordinates.
 */
cEcef::cEcef(cCoordGeo geoCoordinates) {
    double phi = geoCoordinates.m_Lat*0.0174533; //degrees to radians
    double lambda = geoCoordinates.m_Lon*0.0174533;
    double h = geoCoordinates.m_Alt*1000; //km to m
    double NPhi = SEMI_MAJOR_AXIS / sqrt(1-e2*sqr(sin(phi)));

    x = (NPhi+h)*cos(phi)*cos(lambda);
    y = (NPhi+h)*cos(phi)*sin(lambda);
    z = (NPhi*(1-e2)+h)*sin(phi);

    double p = sqrt(sqr(x) + sqr(y));
    double theta = atan2(z * kR, p * kB);

    latitude = atan2(z + kEd2 * kB * pow(sin(theta), 3), p - e2  * kR * pow(cos(theta), 3)) / RADS_PER_DEG;
    longitude = atan2(y, x) / RADS_PER_DEG;
    altitude = (p / cos(latitude * RADS_PER_DEG)) - (kR / sqrt(1.0 - e2 * pow(sin(latitude * RADS_PER_DEG), 2)));
}

cEcef::cEcef(double xx, double yy, double zz, int n)
{
    x=xx;
    y=yy;
    z=zz;

    double p = sqrt(sqr(x) + sqr(y));
    double theta = atan2(z * kR, p * kB);

    latitude = atan2(z + kEd2 * kB * pow(sin(theta), 3), p - e2  * kR * pow(cos(theta), 3)) / RADS_PER_DEG;
    longitude = atan2(y, x) / RADS_PER_DEG;
    altitude = (p / cos(latitude * RADS_PER_DEG)) - (kR / sqrt(1.0 - e2 * pow(sin(latitude * RADS_PER_DEG), 2)));
}

cEcef::cEcef(double latitude, double longitude, double altitude): cEcef(cCoordGeo(latitude, longitude, altitude)) {}

/**
 * Get Distances between two ECEF coordinates using Pythagoras' Theorem
 */
double cEcef::getDistance(cEcef receiverEcef) {
    double x2 = receiverEcef.getX();
    double y2 = receiverEcef.getY();
    double z2 = receiverEcef.getZ();
    return sqrt(sqr(x2-x) + sqr(y2-y) + sqr(z2-z));
}
