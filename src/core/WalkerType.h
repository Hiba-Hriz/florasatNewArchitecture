/*
 * WalkerType.h
 *
 * Created on: Jan 24, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_CORE_WALKERTYPE_H_
#define __FLORA_CORE_WALKERTYPE_H_

#include <omnetpp.h>
#include <string.h>

#include "Constants.h"

using namespace omnetpp;

namespace flora {
namespace core {
namespace WalkerType {

/** @brief The type of the walker constellation. */
enum WalkerType {
    UNINITIALIZED,
    DELTA,
    STAR,
};

WalkerType parseWalkerType(std::string value);

std::ostream &operator<<(std::ostream &ss, const WalkerType &state);

std::string to_string(const WalkerType &state);

}  // namespace WalkerType
}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_WALKERTYPE_H_