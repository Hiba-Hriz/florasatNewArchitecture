//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __FLORA_TRAFFIC_UDPTRAFFICGENERATOR_H_
#define __FLORA_TRAFFIC_UDPTRAFFICGENERATOR_H_

#include <vector>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "ground/TransportHeader_m.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

namespace flora {
namespace traffic {

using namespace omnetpp;
using namespace inet;

class UdpTrafficGenerator : public ClockUserModuleMixin<ApplicationBase>
//, public UdpSocket::ICallback
{
   protected:
    enum SelfMsgKinds { START = 1,
                        SEND,
                        STOP };
    // parameters
    std::vector<L3Address> destAddresses;
    std::vector<std::string> destAddressStr;
    int localPort = -1, destPort = -1;
    clocktime_t startTime;
    clocktime_t stopTime;
    const char *packetName = nullptr;

    // state
    UdpSocket socket;
    ClockEvent *selfMsg = nullptr;
    L3Address appLocalAddress;

    // statistics
    int numSent = 0;
    int numReceived = 0;

   protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    virtual L3Address chooseDestAddr();
    virtual void sendPacket();
    virtual void processPacket(Packet *msg);
    // virtual void setSocketOptions();

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    // virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    // virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    // virtual void socketClosed(UdpSocket *socket) override;

   public:
    UdpTrafficGenerator() {}
    ~UdpTrafficGenerator();
};

}  // namespace traffic
}  // namespace flora

#endif  // __FLORA_TRAFFIC_UDPTRAFFICGENERATOR_H_