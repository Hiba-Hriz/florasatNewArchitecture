/*
 * PositionAwareBase.h
 *
 * Created on: Apr 19, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_POSITIONAWAREBASE_H_
#define __FLORA_TOPOLOGYCONTROL_POSITIONAWAREBASE_H_

#include "libnorad/cEcef.h"
#include "libnorad/cSite.h"

namespace flora {
namespace core {

class PositionAwareBase {
   public:
   /** Returns the latitude of this entity. */
    virtual double getLatitude() const = 0;
   /** Returns the longitude of this entity. */
    virtual double getLongitude() const = 0;
   /** Returns the altitude of this entity. */
    virtual double getAltitude() const = 0;

    /** Returns the distance in km from this entity to a reference entity. */
    double getDistance(const PositionAwareBase &other);
};

}  // namespace core
}  // namespace flora

#endif
