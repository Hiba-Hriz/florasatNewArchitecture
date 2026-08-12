/*
 * ISLDirection.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_CORE_ISLDIRECTION_ISLDIRECTION_H_
#define __FLORA_CORE_ISLDIRECTION_ISLDIRECTION_H_

#include <omnetpp.h>

#include "core/Constants.h"

using namespace omnetpp;

namespace flora {
namespace core {
namespace isldirection {

enum Direction {
    ISL_LEFT,
    ISL_UP,
    ISL_RIGHT,
    ISL_DOWN,
    ISL_DOWNLINK
};

Direction getCounterDirection(Direction dir);

Direction from_str(const char* text);

std::ostream &operator<<(std::ostream &ss, const Direction &direction);

std::string to_string(const Direction &dir);

struct ISLDirection {
    Direction direction;
    int gateIndex;

    ISLDirection(Direction direction, int gateIndex)
        : direction(direction),
          gateIndex(gateIndex){};
};

}  // namespace isldirection
}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_ISLDIRECTION_ISLDIRECTION_H_