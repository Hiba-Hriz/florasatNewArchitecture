/*
 * WalkerType.cc
 *
 * Created on: Jan 24, 2023
 *     Author: Robin Ohs
 */

#include "WalkerType.h"

namespace flora {
namespace core {
namespace WalkerType {

WalkerType parseWalkerType(std::string value) {
    if (value == Constants::WALKERTYPE_DELTA) {
        return WalkerType::DELTA;
    } else if (value == Constants::WALKERTYPE_STAR) {
        return WalkerType::STAR;
    }
    throw cRuntimeError("Error in WalkerType.parseWalkerType(): Could not find provided WalkerType.");
}

std::ostream &operator<<(std::ostream &ss, const WalkerType &walkerType) {
    switch (walkerType) {
        case DELTA:
            ss << Constants::WALKERTYPE_DELTA;
            break;
        case STAR:
            ss << Constants::WALKERTYPE_STAR;
            break;
        default:
            throw cRuntimeError("Error in WalkerType string operator: Unsupported type.");
    }
    return ss;
}

std::string to_string(const WalkerType &walkerType) {
    std::ostringstream ss;
    ss << walkerType;
    return ss.str();
}

}  // namespace WalkerType
}  // namespace core
}  // namespace flora