/*
 * EventManager.cc
 *
 * Created on: May 23, 2023
 *     Author: Robin Ohs
 */

#include "EventManager.h"

namespace flora {
namespace events {

Define_Module(EventManager);

static const char *ATTR_DIR = "dir";
static const char *ATTR_VALUE = "value";
static const char *ATTR_SATID = "satid";
static const char *ATTR_TYPE = "type";

static const char *CMD_SET_ISL_STATE = "set-isl-state";

EventManager::EventManager() {
    ScenarioManager();
}

void EventManager::initialize() {
    ScenarioManager::initialize();

    topologyControl = check_and_cast<TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
}

void EventManager::processCommand(const cXMLElement *node) {
    std::string tag = node->getTagName();

    if (tag == CMD_SET_ISL_STATE) {
        processSetIslState(node);
    } else {
        ScenarioManager::processCommand(node);
    }
}

void EventManager::processSetIslState(const cXMLElement *node) {
    // validate that topologyControl is a module in this simulation
    if (topologyControl == nullptr) throw cRuntimeError("Error in EventManager::processSetIslState: topologyControl is nullptr. Command not possible. Is topology control a sub module of the system module?");

    // parse satellite id from node attribute
    int id = getAttributeIntValue(node, ATTR_SATID);
    VALIDATE(id >= 0 && id < topologyControl->getNumberOfSatellites());
    SatelliteRoutingBase *sat = topologyControl->getSatellite(id);

    // parse new state from node attribute
    ISLState state = from_str(getMandatoryFilledAttribute(*node, ATTR_VALUE));

    // check if it is only meant for send/recv
    Type type = parseType(node->getAttribute(ATTR_TYPE));

    const char *dir = node->getAttribute(ATTR_DIR);
    if (dir == nullptr) {
        if (type == Type::SEND || type == Type::BOTH) {
            sat->setISLSendState(isldirection::Direction::ISL_LEFT, state);
            sat->setISLSendState(isldirection::Direction::ISL_UP, state);
            sat->setISLSendState(isldirection::Direction::ISL_RIGHT, state);
            sat->setISLSendState(isldirection::Direction::ISL_DOWN, state);
        }
        if (type == Type::RECV || type == Type::BOTH) {
            sat->setISLRecvState(isldirection::Direction::ISL_LEFT, state);
            sat->setISLRecvState(isldirection::Direction::ISL_UP, state);
            sat->setISLRecvState(isldirection::Direction::ISL_RIGHT, state);
            sat->setISLRecvState(isldirection::Direction::ISL_DOWN, state);
        }
    } else {
        isldirection::Direction direction = isldirection::from_str(dir);
        if (type == Type::SEND || type == Type::BOTH) {
            sat->setISLSendState(direction, state);
        }
        if (type == Type::RECV || type == Type::BOTH) {
            sat->setISLRecvState(direction, state);
        }
    }
    topologyControl->updateTopology();
}

int EventManager::getAttributeIntValue(const cXMLElement *node, const char *attrName) {
    const char *attrStr = getMandatoryFilledAttribute(*node, attrName);
    return parseInt(attrStr);
}

int EventManager::parseInt(const char *text) {
    try {
        return std::stoi(text);
    } catch (std::exception &e) {
        throw cRuntimeError("No int instance found: %s", text);
    }
}

Type EventManager::parseType(const char *text) {
    if (text != nullptr) {
        if (!strcasecmp(text, "send"))
            return Type::SEND;
        else if (!strcasecmp(text, "recv"))
            return Type::RECV;
        else
            throw cRuntimeError("Unknown type constant: %s", text);
    }
    return Type::BOTH;
}

}  // namespace events
}  // namespace flora