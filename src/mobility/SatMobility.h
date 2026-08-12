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

// This is a modificated version of leosatellites.mobility.SatelliteMobility.
// It does not rely on self messages for updates, instead updates are triggered from the outside.
// This is required as if the satellite count grows to huge numbers, hundreds of self messages do not scale well.
// SatMobilityOrchestrator is used to trigger updates for all satellites.

#ifndef __FLORA_MOBILITY_SATMOBILITY_H_
#define __FLORA_MOBILITY_SATMOBILITY_H_

#include "inet/common/INETDefs.h"
#include "inet/common/INETMath.h"
#include "inet/common/geometry/common/CanvasProjection.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/Quaternion.h"
#include "inet/mobility/base/LineSegmentsMobilityBase.h"
#include "mobility/INorad.h"

namespace flora {
namespace mobility {

class SatMobility : public inet::MobilityBase {
   public:
   protected:
    int mapX, mapY;
    inet::Coord velocity;
    inet::Quaternion angularVelocity;
    inet::Coord currentPosition;
    inet::Quaternion currentOrientation;
    simtime_t lastUpdate;
    bool faceForward;

   public:
    SatMobility();
    // used to update the satellite positions from outside
    void updatePosition(SimTime currentTime);
    // returns x-position of satellite on playground (not longitude!)
    virtual double getPositionX() const { return lastPosition.x; };
    // returns y-position of satellite on playground (not latitude!)
    virtual double getPositionY() const { return lastPosition.y; };

    // returns horizontal satellite position (x) in canvas
    double getXCanvas(double lon) const;
    // returns vertical satellite position (y) in canvas
    double getYCanvas(double lat) const;

    virtual const inet::Coord& getCurrentPosition() override;
    virtual const inet::Coord& getCurrentVelocity() override;
    virtual const inet::Quaternion& getCurrentAngularPosition() override;
    virtual const inet::Quaternion& getCurrentAngularVelocity() override;
    virtual const inet::Coord& getCurrentAcceleration() override { throw cRuntimeError("Invalid operation"); }
    virtual const inet::Quaternion& getCurrentAngularAcceleration() override { throw cRuntimeError("Invalid operation"); }

   protected:
    virtual void initialize(int stage) override;
    virtual void handleSelfMessage(cMessage *message) override;
    virtual void orient();
};

}  // namespace mobility
}  // namespace flora

#endif /* __FLORA_MOBILITY_FASTERSATMOBILITY_H_ */
