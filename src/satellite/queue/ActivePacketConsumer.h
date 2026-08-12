/*
 * ActivePacketConsumer.h
 *
 *  Created on: Mar 17, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_QUEUE_ACTIVEPACKETCONSUMER_H_
#define __FLORA_SATELLITE_QUEUE_ACTIVEPACKETCONSUMER_H_

#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/queueing/sink/ActivePacketSink.h"

using namespace inet;
using namespace inet::queueing;

namespace flora {
namespace satellite {
namespace queue {

class INET_API ActivePacketConsumer : public ActivePacketSink {
   protected:
    virtual void collectPacket() override;

   public:
    virtual void handleCanPullPacketChanged(cGate *gate) override;
};

}  // namespace queue
}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_QUEUE_ACTIVEPACKETCONSUMER_H_