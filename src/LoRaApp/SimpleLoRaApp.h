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
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __LORA_OMNET_SIMPLELORAAPP_H_
#define __LORA_OMNET_SIMPLELORAAPP_H_

#include <omnetpp.h>
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/LifecycleOperation.h"

#include "LoRaAppPacket_m.h"
#include "LoRa/LoRaMacControlInfo_m.h"
#include "inet/common/geometry/common/Coord.h"
using namespace omnetpp;
using namespace inet;

namespace flora {

/**
 * TODO - Generated class
 */
class SimpleLoRaApp : public cSimpleModule, public ILifecycle
{
    protected:
        void initialize(int stage) override;
        void finish() override;
        int numInitStages() const override { return NUM_INIT_STAGES; }
        void handleMessage(cMessage *msg) override;
        virtual bool handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback) override;

        void handleMessageFromLowerLayer(cMessage *msg);
        void sendUplinkPacket();
        void sendDownMgmtPacket();
        inet::Hz txLoRaCF;

        simtime_t firstSentTime;   // temps du 1er envoi de CE paquet
        bool isFirstTransmission;  // flag pour distinguer 1er envoi vs retransmission
        //bool usingAck = true;
        bool receivedAck = true;

        int numberOfPacketsToSend;
        int sentPackets;
        int sentPacketsWithVisibility = 0;
        int receivedAckPackets;
        int receivedADRCommands;
        int lastSentMeasurement;
        simtime_t timeToFirstPacket;
        simtime_t timeToNextPacket;

        simtime_t ackTimeout;
        simtime_t timer;

        cMessage *configureLoRaParameters;
        cMessage *sendUplink;

        cMessage *endAckTime;
        cMessage *synchronizer;
        cMessage *joining;
        cMessage *joiningAns;

        //history of sent packets;
        cOutVector sfVector;
        cOutVector tpVector;


        //variables to control ADR
        cOutVector cfVector;
        bool evaluateADRinNode;
        int ADR_ACK_CNT = 0;
        int ADR_ACK_LIMIT =  64;
        int ADR_ACK_DELAY = 32;
        bool sendNextPacketWithADRACKReq = false;




        void increaseSFIfPossible();

    public:
        SimpleLoRaApp() {}
        virtual ~SimpleLoRaApp();
        simsignal_t LoRa_AppPacketSent;
        void adaptSFToDistance();
        simtime_t lastSentTime = 0;
        simtime_t rttSum = 0;
                int rttCount = 0;
                bool waitingForAck = false;
                bool isGatewayVisible();
        //LoRa physical layer parameters
        double loRaTP;
        units::values::Hz loRaCF;
        int loRaSF;
        units::values::Hz loRaBW;
        int loRaCR;
        bool loRaUseHeader;
        int pingSlot;
        bool usingAck;
        //bool classA;
        //bool classB;
};

}

#endif
