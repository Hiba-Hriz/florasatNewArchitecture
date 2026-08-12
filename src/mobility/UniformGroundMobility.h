/*
 * UniformGroundMobility.h
 *
 *  Created on: Mar 24, 2022
 *      Author: diego
 */

#ifndef MOBILITY_UNIFORMGROUNDMOBILITY_H_
#define MOBILITY_UNIFORMGROUNDMOBILITY_H_

#include <omnetpp.h>

#include "inet/mobility/static/StationaryMobility.h"

using namespace inet;

namespace flora {

//-----------------------------------------------------
// Class: UniformGroundMobility
//
// Positions a LoRa node on ground at a specific lat/long
//-----------------------------------------------------
class UniformGroundMobility : public StationaryMobility {
   public:
    UniformGroundMobility();

    double getLongitude() const;
    double getLatitude() const;
    virtual Coord& getCurrentPosition() override;

    // returns the Euclidean distance from ground station to reference point - Implemented by Aiden Valentine
    virtual double getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

   protected:
    virtual void initialize(int) override;
    virtual void setInitialPosition() override;

    double latitude, longitude;              // Geographic coordinates
    double centerLatitude, centerLongitude;  // Central reference point
    double deploymentRadius;                 // Radius in degrees
    double mapx, mapy;                       // size of canvas map
};

}  // namespace flora

#endif /* MOBILITY_UNIFORMGROUNDMOBILITY_H_ */
