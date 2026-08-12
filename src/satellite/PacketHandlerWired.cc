/*
 * PacketHandlerWired.cc
 *
 *  Created on: Jun 30, 2022
 *      Author: diego
 */

#include "PacketHandlerWired.h"
#include <cmath>
#include <queue>
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "mobility/NoradA.h"
#include "mobility/INorad.h"
#include "LoRaPhy/LoRaRadioControlInfo_m.h"
#include "LoRaPhy/LoRaPhyPreamble_m.h"
#include "LoRa/LoRaTagInfo_m.h"
#include "LoRaApp/LoRaAppPacket_m.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
namespace flora{

Define_Module(PacketHandlerWired);

void PacketHandlerWired::initialize(int stage)
{
    EV << "ACHF - PacketHandler \n";

    if (stage == 0)
    {
        rx1Delay = par("rx1Delay").doubleValue();
        rx2Delay = par("rx2Delay").doubleValue();
        localPort = par("localPort");
        destPort = par("destPort");
        globalGrid = par("globalGrid");
        numOfGroundStations = getSystemModule()->par("nrOfGS");
        LoRa_GWPacketReceived = registerSignal("LoRa_GWPacketReceived");
    }

    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        noradModule = check_and_cast<INorad*>(getParentModule()->getSubmodule("NoradModule"));
        if (NoradA* noradAModule = dynamic_cast<NoradA*>(noradModule))
        {
            satIndex = noradAModule->getSatelliteNumber();
            planes = noradAModule->getNumberOfPlanes();
            int satPerPlane = noradAModule->getSatellitesPerPlane();
            numOfSatellites = planes * satPerPlane;
            maxHops = planes + satPerPlane - 1;
            satPlane = trunc(satIndex/satPerPlane);
            int minSat = satPerPlane * satPlane;
            int maxSat = minSat + satPerPlane - 1;

            // satellite index of right ISL
            if (satPlane < planes-1)
                satRightIndex = satIndex + satPerPlane;

            else if (globalGrid && satPlane == planes-1)
                satRightIndex = satIndex % satPerPlane;

            // satellite index of left ISL
            if (0 < satPlane)
                satLeftIndex = satIndex - satPerPlane;

            else if (globalGrid && satPlane == 0)
                satLeftIndex = satIndex + (planes-1)*satPerPlane;

            // satellite index of up ISL
            if (satIndex+1 <= maxSat)
                satUpIndex = satIndex+1;

            else if (globalGrid && satIndex == maxSat)
                satUpIndex = minSat;

            // satellite index of down ISL
            if (minSat <= satIndex-1)
                satDownIndex = satIndex-1;

            else if (globalGrid && satIndex == minSat)
                satDownIndex = maxSat;
        }
        else
            throw cRuntimeError("NoradTLE mobility is not implemented yet");


        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
    }
}

void PacketHandlerWired::handleMessage(cMessage *msg)
{
    // --- UPLINK: packet received from LoRa radio (EndDevice -> Satellite) ---
    if (msg->arrivedOn("lowerLayerLoRaIn")) {
        EV << "SAT-GW: uplink received from LoRa node, forwarding to NetworkServer\n";

        auto pkt = check_and_cast<Packet *>(msg);
        processLoraMACPacket(pkt);
        // Tag the packet with this satellite's address so the NS knows
        // which gateway received it (used for gateway selection in ADR)
        auto addrTag = pkt->addTagIfAbsent<L3AddressInd>();
        L3Address myAddr = L3Address(Ipv4Address(satIndex));
        addrTag->setSrcAddress(myAddr);

        send(msg, "toNS");
    }
    // --- DOWNLINK: ACK or ADR command received from NetworkServer ---
    else if (msg->arrivedOn("fromNS")) {
        EV << "SAT-GW: downlink received from NetworkServer, scheduling for LoRa transmission\n";
        auto pkt = check_and_cast<Packet *>(msg);
        // Compute RX window timing and schedule transmission
        // (does NOT send immediately => schedules a future self-message)
        scheduleDownlinkToNode(pkt);
    }
    // --- SELF-MESSAGE: scheduled downlink transmission window has arrived ---
    else if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "scheduledDownlink") == 0) {
            EV << "SAT-GW: RX window open, transmitting downlink to LoRa node\n";

            // Retrieve the packet stored when the event was scheduled
            auto pkt = check_and_cast<Packet *>((cObject*)msg->getContextPointer());

            // Send to LoRaGWNic which will transmit over the radio medium
            send(pkt, "lowerLayerLoRaOut");
            delete msg; // clean up the self-message (packet is now owned by send)
        }
    }
}

void PacketHandlerWired::scheduleDownlinkToNode(Packet *pkt)
{
    EV << "NetworkServer scheduleDownlinkToNode" << endl;

    //  Extract all needed info BEFORE modifying the packet
    auto peekFrame = pkt->peekAtFront<LoRaMacFrame>();
    int        sf          = peekFrame->getLoRaSF();
    inet::Hz   bw          = peekFrame->getLoRaBW();
    int        cr          = peekFrame->getLoRaCR();
    inet::Hz   uplinkFreq  = peekFrame->getLoRaCF();

    MacAddress nodeAddr = peekFrame->getReceiverAddress();

    EV << "=== scheduleDownlinkToNode ===" << endl;
    EV << "  nodeAddr="     << nodeAddr    << endl;
    EV << "  simTime="      << simTime()   << endl;

    //  Timing computation
    double    altitudeM  = noradModule->getAltitude() * 1000.0;
    simtime_t propDelay  = altitudeM / 299792458.0;

    simtime_t gwReceptionEnd = simTime();

    simtime_t uplinkEndTime  = gwReceptionEnd - propDelay;

    EV << "  uplinkEndTime = " << uplinkEndTime << endl;
    EV << "  propDelay="  << propDelay  << " rx1Delay=" << rx1Delay << endl;

    simtime_t gatewayTxTime1 = uplinkEndTime + rx1Delay - propDelay; // instant ou le satellite doit commencer a emettre pour que le noeud recoive dans sa fenetre RX1

    simtime_t gatewayTxTime2 = uplinkEndTime + rx2Delay - propDelay;

    EV << "  propDelay="      << propDelay   << endl;
    EV << "  gatewayTxTime1=" << gatewayTxTime1 << " simTime=" << simTime() << endl;

    //  Rebuild packet as downlink ACK
    auto frame      = pkt->removeAtFront<LoRaMacFrame>();
    auto appPayload = pkt->removeAtFront<LoRaAppPacket>();

    frame->setPktType(DOWNLINK);
    frame->setLoRaSF(sf);
    frame->setLoRaBW(bw);
    frame->setLoRaCR(cr);
    frame->setLoRaCF(uplinkFreq);
    frame->setReceiverAddress(nodeAddr);

    appPayload->setMsgType(flora::AppPacketType::ACK);
    appPayload->setChunkLength(B(10));

    pkt->insertAtFront(appPayload);
    pkt->insertAtFront(frame);

    insertSatinRoute(pkt);

    EV << "ACK destine a: " << nodeAddr << endl;

    //  Schedule in RX1 or RX2
    static const inet::Hz rx2Frequency = inet::Hz(869525000);

    if (gatewayTxTime1 > simTime()) {
        auto loraTag = pkt->addTagIfAbsent<LoRaTag>();
        loraTag->setCenterFrequency(uplinkFreq);   // RX1 uses uplink frequency

        Packet  *pktCopy = pkt->dup();
        cMessage *txEvent = new cMessage("scheduledDownlink");
        txEvent->setContextPointer(pktCopy);
        scheduleAt(gatewayTxTime1, txEvent);
        rx1PacketCount++;
        delete pkt;
        return;
    }

    if (gatewayTxTime2 > simTime()) {

        auto frame2      = pkt->removeAtFront<LoRaMacFrame>();
        auto appPayload2 = pkt->removeAtFront<LoRaAppPacket>();

        frame2->setLoRaSF(12);           //  change par rapport a RX1
        frame2->setLoRaCF(rx2Frequency); //  change par rapport a RX1


        pkt->insertAtFront(appPayload2);
        pkt->insertAtFront(frame2);

        auto loraTag = pkt->addTagIfAbsent<LoRaTag>();
        loraTag->setCenterFrequency(rx2Frequency);

        rx2PacketCount++;
        EV << "  [RX2] SF12 - paquets RX2 envoyes jusqu'ici: " << rx2PacketCount << endl;

        Packet  *pktCopy = pkt->dup();
        cMessage *txEvent = new cMessage("scheduledDownlink");
        txEvent->setContextPointer(pktCopy);
        scheduleAt(gatewayTxTime2, txEvent);
        delete pkt;
        return;
    }

    EV << "Too late for both RX windows, dropping ACK for " << nodeAddr << endl;
    delete pkt;
}





void PacketHandlerWired::processLoraMACPacket(Packet *pk)
{
    emit(LoRa_GWPacketReceived, 42);
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfReceivedPackets++;

    pk->trimFront();
    auto frame = pk->removeAtFront<LoRaMacFrame>();
    auto snirInd = pk->getTag<SnirInd>();
    auto signalPowerInd = pk->getTag<SignalPowerInd>();
    W w_rssi = signalPowerInd->getPower();
    double rssi = w_rssi.get()*1000;
    frame->setRSSI(math::mW2dBmW(rssi));
    frame->setSNIR(snirInd->getMinimumSnir());
    pk->insertAtFront(frame);
}

void PacketHandlerWired::sendPacket()
{
//    LoRaAppPacket *mgmtCommand = new LoRaAppPacket("mgmtCommand");
//    mgmtCommand->setMsgType(TXCONFIG);
//    LoRaOptions newOptions;
//    newOptions.setLoRaTP(uniform(0.1, 1));
//    mgmtCommand->setOptions(newOptions);
//
//    LoRaMacFrame *response = new LoRaMacFrame("mgmtCommand");
//    response->encapsulate(mgmtCommand);
//    response->setLoRaTP(pk->getLoRaTP());
//    response->setLoRaCF(pk->getLoRaCF());
//    response->setLoRaSF(pk->getLoRaSF());
//    response->setLoRaBW(pk->getLoRaBW());
//    response->setReceiverAddress(pk->getTransmitterAddress());
//    send(response, "lowerLayerOut");

}

void PacketHandlerWired::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfSentPacketsFromNodes++;
}

void PacketHandlerWired::finish()
{
    recordScalar("LoRa_GW_DER", double(counterOfReceivedPackets)/counterOfSentPacketsFromNodes);
    recordScalar("SatToGroundPkts", sentToGround);
    recordScalar("rcvdFromLoRa", counterOfReceivedPackets);
    recordScalar("rx2PacketCount", rx2PacketCount);
    recordScalar("rx1PacketCount", rx1PacketCount);
    recordScalar("rcvdFromLeftSat", rcvdFromLeftSat);
    recordScalar("rcvdFromDownSat", rcvdFromDownSat);
    recordScalar("rcvdFromRightSat", rcvdFromRightSat);
    recordScalar("rcvdFromUpSat", rcvdFromUpSat);


}

void PacketHandlerWired::SetupRoute(Packet *pkt, int macFrameType)
{
    pkt->trimFront();
    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int numHops = frame->getNumHop();

    frame->setPktType(macFrameType);
    frame->setNumHop(numHops + 1);
    frame->setRouteArraySize(maxHops);
    frame->setTimestampsArraySize(maxHops);
    frame->setRoute(numHops, satIndex);
    frame->setTimestamps(numHops, simTime());

    pkt->insertAtFront(frame);
}

bool PacketHandlerWired::groundStationAvailable()
{
    return satIndex == numOfSatellites-1;
}

bool PacketHandlerWired::loraNodeAvailable()
{
    return true;
}

void PacketHandlerWired::insertSatinRoute(Packet *pkt)
{
    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int numHops = frame->getNumHop();

    // Ensure arrays are large enough before writing
    if ((int)frame->getRouteArraySize() <= numHops)
        frame->setRouteArraySize(numHops + 1);
    if ((int)frame->getTimestampsArraySize() <= numHops)
        frame->setTimestampsArraySize(numHops + 1);

    frame->setNumHop(numHops + 1);
    frame->setRoute(numHops, satIndex);
    frame->setTimestamps(numHops, simTime());
    pkt->insertAtFront(frame);
}

void PacketHandlerWired::forwardToGround(Packet *pkt)
{

    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int numHops = frame->getNumHop();
    int sourceSat = frame->getRoute(numHops-1);

    // ACHF
    EV << "*******************************" <<  endl;
    EV << "PacketHandlerWired forwardToGround" << endl;
    EV << "numHops: " << numHops << endl;
    EV << "sourceSat: " << sourceSat << endl;

    // if message comes from leftsat or downsat forward to ground
    if (sourceSat == satLeftIndex || sourceSat == satDownIndex)
    {
        EV << "Forwarding packet to ground station from satellite " << satIndex << ". Previous satellite hops:" << endl;
        for(int h=0; h<numHops; h++)
            EV << "In satellite " << frame->getRoute(h) << " at time " << frame->getTimestamps(h) << endl;

        frame->setNumHop(numHops + 1);
        frame->setRoute(numHops, satIndex);
        frame->setTimestamps(numHops, simTime());
        pkt->insertAtFront(frame);
        send(pkt, "lowerLayerGS$o");
        sentToGround++;
    }
    // or if it comes from lora forward to ground
    //else if (pkt->arrivedOn("lowerLayerLoRaIn"))
   // {
     //   pkt->insertAtFront(frame);
       // send(pkt, "lowerLayerGS$o");
        //sentToGround++;

    //    EV << "Forwarding packet to ground station from satellite " << satIndex << endl;
      //  EV << "No previous satellite hops, Packet reached local satellite at time " << frame->getTimestamps(0) << endl;
  //  }

    // in other case the packet was sent by a further satellite
    else
        delete pkt;
}

void PacketHandlerWired::forwardToNode(Packet *pkt)
{
    EV << "*******************************" <<  endl;
    EV << "PacketHandlerWired forwardToNode" << endl;
    insertSatinRoute(pkt);
    send(pkt, "lowerLayerLoRaOut");
}

void PacketHandlerWired::forwardToSatellite(Packet *pkt)
{
    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int macFrameType = frame->getPktType();
    int numHops = frame->getNumHop();
    int sourceSat = frame->getRoute(numHops-1);

    // ACHF
    EV << "*******************************" <<  endl;
    EV << "PacketHandlerWired forwardToSatellite" << endl;
    EV << "macFrameType: " << macFrameType << endl;
    EV << "numHops: " << numHops << endl;
    EV << "sourceSat: " << sourceSat << endl;
    EV << "satPlane: " << satPlane << endl;
    EV << "planes: " << planes << endl;
    EV << "UPLINK: " << UPLINK << endl;
    EV << "DOWNLINK: " << DOWNLINK << endl;

    if (sourceSat != satIndex)
    {
        frame->setNumHop(numHops + 1);
        frame->setRoute(numHops, satIndex);
        frame->setTimestamps(numHops, simTime());

        EV << "satIndex: " << satIndex << endl;
    }

    pkt->insertAtFront(frame);
    // forward all uplink packets to the right then up
    if (macFrameType == UPLINK)
    {
        if (satPlane == planes-1)
        {
            cGate *upGate = gate("up1$o");
            if (upGate->getTransmissionChannel()->isBusy())
                scheduleAt(upGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            else
                send(pkt->dup(), upGate);
        }

        else
        {
            cGate *rightGate = gate("right1$o");
            if (rightGate->getTransmissionChannel()->isBusy())
                scheduleAt(rightGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            else
                send(pkt->dup(), rightGate);
        }
    }

    else if (macFrameType == DOWNLINK)
    {
        if (satPlane == 0)
        {
            cGate *downGate = gate("down1$o");
            if (downGate->getTransmissionChannel()->isBusy())
                scheduleAt(downGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            else
                send(pkt->dup(), downGate);
        }
        else
        {
            cGate *leftGate = gate("left1$o");
            if (leftGate->getTransmissionChannel()->isBusy())
                scheduleAt(leftGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            else
                send(pkt->dup(), leftGate);
        }
    }

    if (sourceSat == satLeftIndex)
        rcvdFromLeftSat++;
    else if (sourceSat == satDownIndex)
        rcvdFromDownSat++;
    else if (sourceSat == satRightIndex)
        rcvdFromRightSat++;
    else if (sourceSat == satUpIndex)
        rcvdFromUpSat++;
}

} // namespace flora
