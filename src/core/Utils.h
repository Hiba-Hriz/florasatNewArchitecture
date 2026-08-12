/*
 * PacketGenerator.h
 *
 *  Created on: Mar 23, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_CORE_UTILS_H_
#define __FLORA_CORE_UTILS_H_

#include <omnetpp.h>

namespace flora {
namespace core {
namespace utils {

/** @brief Gets a random number from the interval [start, end] that is not equal to the excluded number. */
int randomNumber(omnetpp::cModule* mod, int start, int end, int excluded);

/**
 * @brief The \opp version of C's assert() macro. If expr evaluates to false, an exception
 * will be thrown with file/line/function information.
 */
#define VALIDATE(expr) \
    ((void)((expr) ? 0 : (throw omnetpp::cRuntimeError("VALIDATE: Condition '%s' does not hold in function '%s' at %s:%d", #expr, __FUNCTION__, __FILE__, __LINE__), 0)))

#define VALIDATE_SET(expr) \
    ((void)((expr != -1) ? 0 : (throw omnetpp::cRuntimeError("VALIDATE_IS_SET: Condition '%s' does not hold in function '%s' at %s:%d", #expr, __FUNCTION__, __FILE__, __LINE__), 0)))

}  // namespace utils
}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_UTILS_H_