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

#include "SimpleLoRaApp.h"
#include "inet/common/packet/Packet.h"
#include "inet/mobility/static/StationaryMobility.h"
#include "LoRa/LoRaTagInfo_m.h"
#include "mobility/UniformGroundMobility.h"
#include "inet/common/geometry/common/Coord.h"
#include <cmath>
#include "../mobility/SatelliteMobility.h"
#include "LoRa/LoRaMac.h"
namespace flora {

Define_Module(SimpleLoRaApp);

SimpleLoRaApp::~SimpleLoRaApp()
{
    cancelAndDelete(endAckTime);
    cancelAndDelete(sendUplink);
}

void SimpleLoRaApp::initialize(int stage)
{
    EV << "ACHF - Esta es la primera SimpleLoraApp \n";
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_APPLICATION_LAYER)
    {

        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus*>(findContainingNode(
                this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");

        firstSentTime = 0;
        // Parameters
        timeToFirstPacket = par("timeToFirstPacket");
        timeToNextPacket = par("timeToNextPacket");
        ackTimeout = par("ackTimeout"); //acknowledgment timeout
        timer = par("timer"); //timer used for acknowledgment implementation

        ADR_ACK_LIMIT = par("ADR_ACK_LIMIT");
        ADR_ACK_DELAY = par("ADR_ACK_DELAY");

        // Messages
        endAckTime = new cMessage("acknowledgment Timeout"); //signal to notice about end of ACK time
        sendUplink = new cMessage("sendMeasurements");

        //synchronizer = new cMessage("Ask for device local time"); //signal for future use
        //joining = new cMessage("Try to reach GW"); //signal for future use
        //joiningAns = new cMessage("Can't reach GW ? .."); //signal for future use

        // Schedule first packet
        scheduleAt(simTime() + timeToFirstPacket, sendUplink);
        timer = simTime() + timeToFirstPacket;



        // Initialize  metrics
        receivedAckPackets = 0;
        sentPackets = 0;
        receivedADRCommands = 0;
        receivedAck = false;
        // Initialize parameters
        numberOfPacketsToSend = par("numberOfPacketsToSend");
        loRaTP = par("initialLoRaTP").doubleValue();
        //loRaCF = units::values::Hz(par("initialLoRaCF").doubleValue());
        loRaSF = par("initialLoRaSF");
        loRaBW = inet::units::values::Hz(par("initialLoRaBW").doubleValue());
        loRaCR = par("initialLoRaCR");
        loRaUseHeader = par("initialUseHeader");
        pingSlot = par("initialPingSlot");
        evaluateADRinNode = par("evaluateADRinNode");
        // Metrics
        LoRa_AppPacketSent = registerSignal("LoRa_AppPacketSent");
        sfVector.setName("SF Vector");
        tpVector.setName("TP Vector");
        cfVector.setName("CFVector");
        WATCH(receivedAck);
    }
}


void SimpleLoRaApp::finish()
{
    cModule *loRaNodeModule = getContainingNode(this);

    if (UniformGroundMobility *uniformMobility = dynamic_cast<UniformGroundMobility*>(loRaNodeModule->getSubmodule("mobility")))
    {
        recordScalar("Longitude", uniformMobility->getLongitude());
        recordScalar("Latitude", uniformMobility->getLatitude());
    }
    else
    {
        StationaryMobility *mobility = check_and_cast<StationaryMobility*>(loRaNodeModule->getSubmodule("mobility"));
        Coord coord = mobility->getCurrentPosition();
        recordScalar("positionX", coord.x);
        recordScalar("positionY", coord.y);
    }

    recordScalar("receivedAck", receivedAck);
    if (rttCount > 0)
        recordScalar("Latency", rttSum / rttCount);
    recordScalar("RTTCount", rttCount);
    recordScalar("finalTP", loRaTP);
    recordScalar("finalSF", loRaSF);
    recordScalar("sentPackets", sentPackets);
    recordScalar("receivedAckPackets", receivedAckPackets);
    recordScalar("receivedADRCommands", receivedADRCommands);

}

void SimpleLoRaApp::handleMessage(cMessage *msg)
{
    // Either ack timer expired, or send measurement
    if (msg->isSelfMessage())
    {

        // New packet to send
        if (msg == sendUplink)
        {
            //delete msg;

            // Send new packet
            sendUplinkPacket();

            // More packets to send
                        if (numberOfPacketsToSend == 0 || sentPackets < numberOfPacketsToSend)
                        {
                            // Schedule next packet
                            if (sendUplink->isScheduled())
                                cancelEvent(sendUplink);
                            scheduleAt(simTime() + timeToNextPacket, sendUplink);

                            // Schedule next packet ack timeout
                            if (par("usingAck").boolValue())
                            {
                                if (endAckTime->isScheduled())
                                    cancelEvent(endAckTime);
                                scheduleAt(simTime() + timeToNextPacket, endAckTime);
                                timer = timeToNextPacket + timer;
                            }
                        }
        }
    }

    // Otherwise it is a message from outside (lower layer)
    else
    {
        handleMessageFromLowerLayer(msg);
        delete msg;
    }
}

void SimpleLoRaApp::handleMessageFromLowerLayer(cMessage *msg)
{
    auto pkt = check_and_cast<Packet*>(msg);
    const auto &packet = pkt->peekAtFront<LoRaAppPacket>();
    int receivedType = packet->getMsgType();
        int expectedType = flora::AppPacketType::ACK;

        EV << ">>> APP RECV: msgType=" << receivedType
           << " | Expected ACK=" << expectedType << endl;

        EV << ">>> Comparison result: " << (receivedType == expectedType ? "TRUE" : "FALSE") << endl;
    // Received ACK

        if (packet->getMsgType() == ACK) {
            simtime_t rtt = simTime() - firstSentTime;
            rttSum += rtt;
            rttCount++;


            cancelEvent(endAckTime);
            receivedAckPackets++;

            receivedAck = true;
            ADR_ACK_CNT = 0;
            //  lire les options ADR
            if (evaluateADRinNode) {
                LoRaOptions opts = packet->getOptions();
                int newSF = opts.getLoRaSF();
                double newTP = opts.getLoRaTP();

                EV << "ADR options received: SF=" << newSF << " TP=" << newTP << endl;

                if (newSF != -1 && newSF != loRaSF) {
                    EV << ">>> Applying new SF: " << loRaSF << " => " << newSF << endl;
                    loRaSF = newSF;
                    receivedADRCommands++;


                }
                if (newTP != -1 && newTP != loRaTP) {
                    EV << ">>> Applying new TP: " << loRaTP << " => " << newTP << endl;
                    loRaTP = newTP;

                }
            }        }

    // Received beacon
    if (packet->getMsgType() == Beacon)
    {
        //receive and set the pingSlot value to the value read from the GW beacon message
        pingSlot = packet->getOptions().getPingSlot();
        EV << pingSlot << endl;
    }

    // Received TxConfig
    if (packet->getMsgType() == TXCONFIG)
    {
        ADR_ACK_CNT = 0;
        if (evaluateADRinNode)
        {
            if (simTime() >= getSimulation()->getWarmupPeriod())
                receivedADRCommands++;

            if (packet->getOptions().getLoRaTP() != -1)
                loRaTP = packet->getOptions().getLoRaTP();

            if (packet->getOptions().getLoRaSF() != -1)
                loRaSF = packet->getOptions().getLoRaSF();

            EV << "New TP " << loRaTP << endl;
            EV << "New SF " << loRaSF << endl;
        }
    }
}

bool SimpleLoRaApp::handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

void SimpleLoRaApp::sendUplinkPacket()
{
    EV << "=== ATTEMPTING TO SEND PACKET ===" << endl;
        if (!isGatewayVisible())
            {
                EV << "Gateway not visible - skipping packet transmission" << endl;
                return;
            }

        EV << " Gateway IS VISIBLE - sending packet" << endl;

    auto uplinkPacket = new Packet("DataFrame");
        uplinkPacket->setKind(DATA);

        auto payload = makeShared<LoRaAppPacket>();
        payload->setChunkLength(B(par("payloadSize").intValue()));

        lastSentMeasurement = rand();
        payload->setSampleMeasurement(lastSentMeasurement);

        if (evaluateADRinNode && sendNextPacketWithADRACKReq)
        {
            auto opt = payload->getOptions();
            opt.setADRACKReq(true);
            payload->setOptions(opt);
            sendNextPacketWithADRACKReq = false;
        }

        auto loraTag = uplinkPacket->addTagIfAbsent<LoRaTag>();
        loraTag->setBandwidth(loRaBW);
        double frequencies[] = {868.1e6, 868.3e6, 868.5e6};
        int randomIndex = intuniform(0, 2);
        loRaCF = units::values::Hz(frequencies[randomIndex]);
        loraTag->setCenterFrequency(loRaCF);
        loraTag->setSpreadFactor(loRaSF);
        loraTag->setCodeRendundance(loRaCR);
        loraTag->setPower(mW(math::dBmW2mW(loRaTP)));

        cfVector.record(loRaCF.get() / 1e6);
        sfVector.record(loRaSF);
        tpVector.record(loRaTP);

        uplinkPacket->insertAtBack(payload);

            sentPackets++;
            firstSentTime = simTime();
        send(uplinkPacket, "appOut");
        emit(LoRa_AppPacketSent, loRaSF);
        if (par("usingAck").boolValue()) {
        EV << "=== ADR DEBUG START ===" << endl;
        EV << "evaluateADRinNode: " << evaluateADRinNode << endl;
        EV << "receivedAck: " << receivedAck << endl;
        if (evaluateADRinNode && receivedAck == false)
        {
            ADR_ACK_CNT++;
            EV << "!!! ADR ACTIVE - ADR_ACK_CNT: " << ADR_ACK_CNT << endl;
            EV << "!!! ADR_ACK_LIMIT: " << ADR_ACK_LIMIT << endl;
            EV << "!!! ADR_ACK_DELAY: " << ADR_ACK_DELAY << endl;
            EV << "!!! Current SF: " << loRaSF << endl;
            EV << "=== ADR NODE DEBUG ===" << endl;
            EV << "ADR_ACK_CNT: " << ADR_ACK_CNT << endl;
            EV << "ADR_ACK_LIMIT: " << ADR_ACK_LIMIT << endl;
            EV << "ADR_ACK_DELAY: " << ADR_ACK_DELAY << endl;
            EV << "Current SF: " << loRaSF << endl;
            EV << "Current TP: " << loRaTP << endl;
            EV << ADR_ACK_CNT << endl;
            if (ADR_ACK_CNT == ADR_ACK_LIMIT){
                sendNextPacketWithADRACKReq = true;
                EV << "!!! Setting ADR ACK Request flag" << endl;}

            if (ADR_ACK_CNT >= ADR_ACK_LIMIT + ADR_ACK_DELAY)
            {
                ADR_ACK_CNT = 0;
                increaseSFIfPossible();
                EV << "!!! ADR TRIGGERED - SF increased to: " << loRaSF << endl;
                EV << "!!! ADR TRIGGERED - TP: " << loRaTP << endl;
                EV << "i'm working on the ADRNode " << endl;
                EV << loRaSF << endl;
                EV << loRaTP << endl;
            }
        }

        }
}

void SimpleLoRaApp::increaseSFIfPossible()
{
    if (loRaSF < 12)
        loRaSF++;
}

bool SimpleLoRaApp::isGatewayVisible()
{
    cModule* network = getSimulation()->getSystemModule();
    cModule* gateway = network->getSubmodule("loRaGW", 0);
    cModule* nodeMobility = getContainingNode(this)->getSubmodule("mobility");
    cModule* gwMobility = gateway->getSubmodule("mobility");

    auto uniformMob = dynamic_cast<UniformGroundMobility*>(nodeMobility);
    double nodeLat = uniformMob->getLatitude();
    double nodeLon = uniformMob->getLongitude();
    double nodeAlt = 0.0; //noeuds au sol;

    auto satMob = dynamic_cast<SatelliteMobility*>(gwMobility);
    double satLat = satMob->getLatitude();
    double satLon = satMob->getLongitude();
    double satAlt = satMob->getAltitude();
    double distance = satMob->getDistance(nodeLat, nodeLon, nodeAlt);
    double elevation = satMob->getElevation(nodeLat, nodeLon, nodeAlt);
    double minElevation = par("minElevationAngle").doubleValue();
    double maxDistance = par("maxDistance").doubleValue();
    bool visible = (elevation >= minElevation) && (distance <= maxDistance);

    EV << "--- Visibility Check ---" << endl;
    EV << "Node (lat, lon, alt): (" << nodeLat << ", " << nodeLon << ", " << nodeAlt << ")" << endl;
    EV << "Satellite (lat, lon, alt): (" << satLat << ", " << satLon << ", " << satAlt << ")" << endl;
    EV << "Distance: " << distance << " km" << endl;
    EV << "Elevation: " << elevation << "  (min: " << minElevation << " )" << endl;
    EV << "Max distance allowed: " << maxDistance << " km" << endl;
    EV << "VISIBLE: " << (visible ? "YES  " : "NO  ") << endl;

    return visible;
}


} //end namespace inet
