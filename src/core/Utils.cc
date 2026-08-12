/*
 * PacketGenerator.cc
 *
 *  Created on: Mar 23, 2023
 *      Author: Robin Ohs
 */

#include "Utils.h"

namespace flora {
namespace core {
namespace utils {

int randomNumber(omnetpp::cModule* mod, int start, int end, int excluded) {
    int rn = mod->intuniform(start, end);
    while (rn == excluded) {
        rn = mod->intuniform(start, end);
    }
    return rn;
}


}  // namespace utils
}  // namespace core
}  // namespace flora
