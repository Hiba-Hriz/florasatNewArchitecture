/*
 * Constants.h
 *
 * Created on: Apr 19, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_CORE_CONSTANTS_H_
#define __FLORA_CORE_CONSTANTS_H_

namespace flora {
namespace core {

class Constants {
   public:
    constexpr static char const* ISL_CHANNEL_NAME = "IslChannel";
    constexpr static char const* ISL_UP_NAME = "up";
    constexpr static char const* ISL_DOWN_NAME = "down";
    constexpr static char const* ISL_LEFT_NAME = "left";
    constexpr static char const* ISL_RIGHT_NAME = "right";
    constexpr static char const* SAT_GROUNDLINK_NAME = "groundLink";
    constexpr static char const* GS_SATLINK_NAME = "satelliteLink";

    constexpr static char const* ISL_STATE_WORKING = "WORKING";
    constexpr static char const* ISL_STATE_DISABLED = "DISABLED";

    constexpr static char const* WALKERTYPE_DELTA = "DELTA";
    constexpr static char const* WALKERTYPE_STAR = "STAR";
};

}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_CONSTANTS_H_