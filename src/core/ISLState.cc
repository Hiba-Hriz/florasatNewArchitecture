/*
 * ISLState.cc
 *
 * Created on: Apr 20, 2023
 *     Author: Robin Ohs
 */

#include "ISLState.h"

namespace flora {
namespace core {

std::ostream &operator<<(std::ostream &ss, const ISLState &state) {
    switch (state) {
        case ISLState::WORKING:
            ss << Constants::ISL_STATE_WORKING;
            break;
        case ISLState::DISABLED:
            ss << Constants::ISL_STATE_DISABLED;
            break;
        default:
            throw omnetpp::cRuntimeError("Error in ISLState::string operator");
    }
    return ss;
}

ISLState from_str(const char *text) {
    if (!strcasecmp(text, Constants::ISL_STATE_WORKING))
        return ISLState::WORKING;
    else if (!strcasecmp(text, Constants::ISL_STATE_DISABLED))
        return ISLState::DISABLED;
    else
        throw cRuntimeError("Unknown text constant: %s", text);
}

std::string to_string(const ISLState &state) {
    std::ostringstream ss;
    ss << state;
    return ss.str();
}

}  // namespace core
}  // namespace flora