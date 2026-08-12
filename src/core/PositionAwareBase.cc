#include "PositionAwareBase.h"

namespace flora {
namespace core {

double PositionAwareBase::getDistance(const PositionAwareBase &other) {
    auto pos_1 = cEcef(getLatitude(), getLongitude(), getAltitude());
    auto pos_2 = cEcef(other.getLatitude(), other.getLongitude(), other.getAltitude());
    return pos_1.getDistance(pos_2) / 1000.0; 
}

}  // namespace core
}  // namespace flora
