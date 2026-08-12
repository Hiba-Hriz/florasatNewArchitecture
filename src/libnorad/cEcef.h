#ifndef OS3_LIBNORAD_CECEF_H_
#define OS3_LIBNORAD_CECEF_H_

#include "libnorad/globals.h"
#include "libnorad/ccoord.h"
#include <math.h>

//-----------------------------------------------------
// Class: CEcef
//
// The Earth-Centered, Earth-Fixed Coordinate system translates a given nodes longitude, latitude and altitude
// data to x, y and z coordinates relative to the origin (the centre of the earth). This coordinate system was
// used as the cEcef coordinate system requires the current Julian time of a given node, which is not stored
// within the LUTMotionMobility class. This could have been implemented but it would require constant updating,
// potentially affecting the performance of the simulation depending on the amount of ground stations used.
// Written by Aiden Valentine
// References: https://uk.mathworks.com/help/aeroblks/llatoecefposition.html
//-----------------------------------------------------

const double kF = 1.0 / 298.26;
const double e2 = 6.6943799901377997e-3;
const double kR = 1000 * XKMPER_WGS72;
const double kB = kR * (1.0 - kF);
const double kEd2 = e2 * kR * kR / (kB * kB);

class cEcef {
    public:
        cEcef();
        cEcef(cCoordGeo geoCoordinates);
        cEcef(double x, double y, double z, int n);
        cEcef(double latitude, double longitude, double altitude);
        virtual ~cEcef(){};

        double getDistance(cEcef receiverEcef);

        double getX(){return x;};
        double getY(){return y;};
        double getZ(){return z;};
        double getLongitude(){return longitude;};
        double getLatitude(){return latitude;};
        double getAltitude(){return altitude;};

    private:
        double x;
        double y;
        double z;
        double longitude;
        double latitude;
        double altitude;
};

#endif /* OS3_LIBNORAD_CECEF_H_ */
