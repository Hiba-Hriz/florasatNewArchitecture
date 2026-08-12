/*
 * DtnUdpTrafficGenerator.h
 *
 *  Created on: May 30, 2023
 *      Author: Sebastian Montoya
 */

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

#ifndef __FLORA_TRAFFIC_DTNTRAFFICGENERATOR_H_
#define __FLORA_TRAFFIC_DTNTRAFFICGENERATOR_H_

#include <vector>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "ground/DtnTransportHeader_m.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "ground/DtnPacketGenerator.h"

namespace flora {
namespace traffic {

using namespace omnetpp;
using namespace inet;

class DtnTrafficGenerator : public cSimpleModule
{
    protected:
        const char *packetName = nullptr;

        // statistics
        int appBundleSent;
        int appBundleReceived;
        int appBundleReceivedHops;
        int appBundleReceivedDelay;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

    private:
        int eid_;
        bool enable;
        std::vector<int> bundlesNumber;
        std::vector<int> destinationsEid;
        std::vector<int> sizes;
        std::vector<double> starts;
        void parseBundlesNumber();
        void parseDestinationsEid();
        void parseSizes();
        void parseStarts();

    public:
        DtnTrafficGenerator(){};
        ~DtnTrafficGenerator(){};
        int getEid() const;
        virtual vector<int> getBundlesNumber();
        virtual vector<int> getDestinationsEid();
        virtual vector<int> getSizes();
        virtual vector<double> getStarts();

};

}  // namespace traffic
}  // namespace flora

#endif  // __FLORA_TRAFFIC_DTNUDPTRAFFICGENERATOR_H_
