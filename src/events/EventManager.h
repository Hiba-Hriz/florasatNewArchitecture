/*
 * EventManager.h
 *
 * Created on: May 23, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_EVENTS_EVENTMANAGER_H_
#define __FLORA_EVENTS_EVENTMANAGER_H_

#include "core/Utils.h"
#include "inet/common/XMLUtils.h"
#include "inet/common/scenario/ScenarioManager.h"
#include "topologycontrol/TopologyControlBase.h"

using namespace inet;
using namespace inet::xmlutils;
using namespace flora::topologycontrol;

namespace flora {
namespace events {

enum Type {
    SEND,
    RECV,
    BOTH
};

class EventManager : public ScenarioManager {
   public:
    virtual void initialize() override;
    EventManager();

   protected:
    void processCommand(const cXMLElement *node) override;

    // command processors
    virtual void processSetIslState(const cXMLElement *node);

    int getAttributeIntValue(const cXMLElement *node, const char *attrName);
    int parseInt(const char *text);
    Type parseType(const char *text);

   private:
    TopologyControlBase *topologyControl;
};

}  // namespace events
}  // namespace flora

#endif