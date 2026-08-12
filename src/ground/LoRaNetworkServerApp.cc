/*
 * LoRaNetworkServerApp.cc
 *
 *  Created on: May 2, 2022
 *      Author: diego
 */

#define PI 3.14159265
#include "LoRaNetworkServerApp.h"
//#include "inet/networklayer/ipv4/IPv4Datagram.h"
//#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"

#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"

#include<fstream>
#include <cmath>
#include <iostream>
#include <string.h>
#include <math.h>

#include <cstring>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

using namespace std;

namespace flora {

Define_Module(LoRaNetworkServerApp);

void LoRaNetworkServerApp::initialize(int stage)
{
    if (stage == 0)
    {
        ASSERT(recvdPackets.size()==0);
        LoRa_ServerPacketReceived = registerSignal("LoRa_ServerPacketReceived");
        localPort = par("localPort");
        destPort = par("destPort");
        adrMethod = par("adrMethod").stdstringValue();
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {

        //startUDP();
        /*
                 * CHANGEMENT : L'appel a startUDP() a ete supprime ici.
                 * Dans l'ancienne version, le serveur utilisait un socket UDP pour recevoir les paquets
                 * des passerelles. Dans la nouvelle architecture, la communication
                 * entre la passerelle et le serveur se fait via des gates directes ("in"/"out"),
                 * ce qui ne necessite plus de socket UDP.
                 */
        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
        evaluateADRinServer = par("evaluateADRinServer");
        adrDeviceMargin = par("adrDeviceMargin");
        receivedRSSI.setName("Received RSSI");
        totalReceivedPackets = 0;
         totalUniqueSentPackets = 0;

        WorkWithAck = par("WorkWithAck");
        totalCase1 = 0;
        totalCase2 = 0;
        totalCase3 = 0;
        totalCase4 = 0;
        netServerDPAD = 0;
        dpadVector.setName("DPAD Vector");
        WATCH(netServerDPAD);


        //BeaconTimer = new cMessage("Beacon Timer");
        //scheduleAt(simTime() + 1, BeaconTimer);

        for(int i=0;i<6;i++)
        {
            counterUniqueReceivedPacketsPerSF[i] = 0;
            counterOfSentPacketsFromNodesPerSF[i] = 0;
        }
    }
}

void LoRaNetworkServerApp::startUDP()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
}

void LoRaNetworkServerApp::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("socketIn"))
        {
            auto pkt = check_and_cast<Packet *>(msg);
            const auto &frame  = pkt->peekAtFront<LoRaMacFrame>();
            if (frame == nullptr)
                throw cRuntimeError("Header error type");
            //LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(msg);
            if (simTime() >= getSimulation()->getWarmupPeriod())
                totalReceivedPackets++;
            updateKnownNodes(pkt);
            processLoraMACPacket(pkt);
        }

    else if (msg->arrivedOn("in")) {
            auto pkt = check_and_cast<Packet *>(msg);
            const auto& macFrame = pkt->peekAtFront<LoRaMacFrame>();
            if (macFrame == nullptr)
                throw cRuntimeError("Header error type");

            if (simTime() >= getSimulation()->getWarmupPeriod())
                totalReceivedPackets++;

            updateKnownNodes(pkt);
            processLoraMACPacket(pkt);
        }
    else if (msg->isSelfMessage()) {  //un message programme par le module lui-meme pour un traitement
            processScheduledPacket(msg);
        }
}


void LoRaNetworkServerApp::processLoraMACPacket(Packet *pk)
{
    const auto &frame = pk->peekAtFront<LoRaMacFrame>();

    if (isPacketProcessed(frame))
    {

        if (WorkWithAck)
        {
            EV << "Retransmission detectee pour seqNo="
               << frame->getSequenceNumber()
               << ", renvoi d'un ACK sans recompter" << endl;
            sendAckForRetransmission(pk);
        }
        else
        {
            delete pk;
        }
        return;
    }
    addPktToProcessingTable(pk);
}
void LoRaNetworkServerApp::sendAckForRetransmission(Packet *pk)
{
    const auto &frame = pk->peekAtFront<LoRaMacFrame>();

    auto mgmtPacket = makeShared<LoRaAppPacket>();
    mgmtPacket->setMsgType(ACK);
    mgmtPacket->setChunkLength(B(par("headerLength").intValue()));

    auto frameToSend = makeShared<LoRaMacFrame>();
    frameToSend->setChunkLength(B(par("headerLength").intValue()));
    frameToSend->setReceiverAddress(frame->getTransmitterAddress());
    frameToSend->setLoRaTP(math::dBmW2mW(14));
    frameToSend->setLoRaCF(frame->getLoRaCF());
    frameToSend->setLoRaSF(frame->getLoRaSF());
    frameToSend->setLoRaBW(frame->getLoRaBW());
    frameToSend->setLoRaCR(frame->getLoRaCR());
    frameToSend->setNumHop(frame->getNumHop());

    auto pktAux = new Packet("ACK-RETRANSMISSION");
    pktAux->insertAtFront(mgmtPacket);
    pktAux->insertAtFront(frameToSend);
    send(pktAux, "out");

    delete pk;
}
void LoRaNetworkServerApp::ensureNodeKnown(Packet* pkt)
{
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
    for (const auto &elem : knownNodes)
        if (elem.srcAddr == frame->getTransmitterAddress())
            return;

    knownNode newNode;
    newNode.srcAddr = frame->getTransmitterAddress();
    newNode.lastSeqNoProcessed = -1;
    newNode.framesFromLastADRCommand = 0;
    newNode.numberOfSentADRPackets = 0;
    newNode.historyAllSNIR = new cOutVector;
    newNode.historyAllSNIR->setName("Vector of SNIR per node");
    newNode.historyAllSNIR->record(math::fraction2dB(frame->getSNIR()));
    newNode.historyAllRSSI = new cOutVector;
    newNode.historyAllRSSI->setName("Vector of RSSI per node");
    newNode.historyAllRSSI->record(frame->getRSSI());
    newNode.receivedSeqNumber = new cOutVector;
    newNode.receivedSeqNumber->setName("Received Sequence number");
    newNode.calculatedSNRmargin = new cOutVector;
    newNode.calculatedSNRmargin->setName("Calculated SNRmargin in ADR");
    knownNodes.push_back(newNode);
}
void LoRaNetworkServerApp::finish()
{
    recordScalar("LoRa_NS_DER", double(counterUniqueReceivedPackets)/counterOfSentPacketsFromNodes);
    recordScalar("counterUniqueReceivedPackets", counterUniqueReceivedPackets);
    recordScalar("netServerDPAD", netServerDPAD);
    for(uint i=0;i<knownNodes.size();i++)
    {
        delete knownNodes[i].historyAllSNIR;
        delete knownNodes[i].historyAllRSSI;
        delete knownNodes[i].receivedSeqNumber;
        delete knownNodes[i].calculatedSNRmargin;
        recordScalar("Send ADR for node", knownNodes[i].numberOfSentADRPackets);
    }
    for (std::map<int,int>::iterator it=numReceivedPerNode.begin(); it != numReceivedPerNode.end(); ++it)
    {
        const std::string stringScalar = "numReceivedFromNode " + std::to_string(it->first);
        recordScalar(stringScalar.c_str(), it->second);
    }

    receivedRSSI.recordAs("receivedRSSI");
    recordScalar("totalReceivedPackets", totalReceivedPackets);
    recordScalar("totalCase1", totalCase1);
    recordScalar("totalCase2", totalCase2);
    recordScalar("totalCase3", totalCase3);
    recordScalar("totalCase4", totalCase4);

    while(!receivedPackets.empty()) {

        // receivedPackets.back().endOfWaiting->removeControlInfo();
        delete receivedPackets.back().rcvdPacket;
        if (receivedPackets.back().endOfWaiting->isScheduled())
            cancelAndDelete(receivedPackets.back().endOfWaiting);
        else
            delete receivedPackets.back().endOfWaiting;
        receivedPackets.pop_back();
    }

    knownNodes.clear();
    receivedPackets.clear();

    recordScalar("counterUniqueReceivedPacketsPerSF SF7", counterUniqueReceivedPacketsPerSF[0]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF8", counterUniqueReceivedPacketsPerSF[1]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF9", counterUniqueReceivedPacketsPerSF[2]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF10", counterUniqueReceivedPacketsPerSF[3]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF11", counterUniqueReceivedPacketsPerSF[4]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF12", counterUniqueReceivedPacketsPerSF[5]);
    if (counterOfSentPacketsFromNodesPerSF[0] > 0)
        recordScalar("DER SF7", double(counterUniqueReceivedPacketsPerSF[0]) / counterOfSentPacketsFromNodesPerSF[0]);
    else
        recordScalar("DER SF7", 0);

    if (counterOfSentPacketsFromNodesPerSF[1] > 0)
        recordScalar("DER SF8", double(counterUniqueReceivedPacketsPerSF[1]) / counterOfSentPacketsFromNodesPerSF[1]);
    else
        recordScalar("DER SF8", 0);

    if (counterOfSentPacketsFromNodesPerSF[2] > 0)
        recordScalar("DER SF9", double(counterUniqueReceivedPacketsPerSF[2]) / counterOfSentPacketsFromNodesPerSF[2]);
    else
        recordScalar("DER SF9", 0);

    if (counterOfSentPacketsFromNodesPerSF[3] > 0)
        recordScalar("DER SF10", double(counterUniqueReceivedPacketsPerSF[3]) / counterOfSentPacketsFromNodesPerSF[3]);
    else
        recordScalar("DER SF10", 0);

    if (counterOfSentPacketsFromNodesPerSF[4] > 0)
        recordScalar("DER SF11", double(counterUniqueReceivedPacketsPerSF[4]) / counterOfSentPacketsFromNodesPerSF[4]);
    else
        recordScalar("DER SF11", 0);

    if (counterOfSentPacketsFromNodesPerSF[5] > 0)
        recordScalar("DER SF12", double(counterUniqueReceivedPacketsPerSF[5]) / counterOfSentPacketsFromNodesPerSF[5]);
    else
        recordScalar("DER SF12", 0);
}

bool LoRaNetworkServerApp::isPacketProcessed(const Ptr<const LoRaMacFrame> &pkt)
{
    for (const auto &elem : knownNodes)
    {
        if (elem.srcAddr == pkt->getTransmitterAddress())
        {
            return elem.processedSeqNumbers.find(
                       pkt->getSequenceNumber()
                   ) != elem.processedSeqNumbers.end();
        }
    }
    return false;
}

void LoRaNetworkServerApp::updateKnownNodes(Packet* pkt)
{
    const auto & frame = pkt->peekAtFront<LoRaMacFrame>();
    bool nodeExist = false;

    for (auto &elem : knownNodes)
    {
        if(elem.srcAddr == frame->getTransmitterAddress())
        {
            nodeExist = true;
            if(elem.lastSeqNoProcessed < frame->getSequenceNumber())
                elem.lastSeqNoProcessed = frame->getSequenceNumber();
            break;
        }
    }

    if(!nodeExist)
    {
        knownNode newNode;
        newNode.srcAddr= frame->getTransmitterAddress();
        newNode.lastSeqNoProcessed = frame->getSequenceNumber();
        newNode.framesFromLastADRCommand = 0;
        newNode.numberOfSentADRPackets = 0;
        newNode.historyAllSNIR = new cOutVector;
        newNode.historyAllSNIR->setName("Vector of SNIR per node");
        //newNode.historyAllSNIR->record(pkt->getSNIR());
        newNode.historyAllSNIR->record(math::fraction2dB(frame->getSNIR()));
        newNode.historyAllRSSI = new cOutVector;
        newNode.historyAllRSSI->setName("Vector of RSSI per node");
        newNode.historyAllRSSI->record(frame->getRSSI());
        newNode.receivedSeqNumber = new cOutVector;
        newNode.receivedSeqNumber->setName("Received Sequence number");
        newNode.calculatedSNRmargin = new cOutVector;
        newNode.calculatedSNRmargin->setName("Calculated SNRmargin in ADR");
        knownNodes.push_back(newNode);
    }
}

void LoRaNetworkServerApp::addPktToProcessingTable(Packet* pkt)
{
    const auto & frame = pkt->peekAtFront<LoRaMacFrame>();
    bool packetExists = false;
    for (auto &elem : receivedPackets)
    {
        const auto &frameAux = elem.rcvdPacket->peekAtFront<LoRaMacFrame>();
        if(frameAux->getTransmitterAddress() == frame->getTransmitterAddress() && frameAux->getSequenceNumber() == frame->getSequenceNumber())
        {
            packetExists = true;
            //const auto& networkHeader = getNetworkProtocolHeader(pkt);
            //const L3Address& gwAddress = networkHeader->getSourceAddress();
            /*
               Recuperation de l'adresse de la passerelle.
                        *
                        * Ancienne version : on utilisait getNetworkProtocolHeader(pkt) pour extraire
                        * l'adresse source depuis l'en-tete reseau IP (car le paquet transitait par UDP/IP).
                        *   const auto& networkHeader = getNetworkProtocolHeader(pkt);
                        *   const L3Address& gwAddress = networkHeader->getSourceAddress();
                        *
                        * Nouvelle version : on lit directement le tag L3AddressInd attache au paquet.
                        * Ce tag est pose par la passerelle lors de l'envoi direct via gate. C'est plus
                        * simple et coherent avec l'architecture sans socket UDP.
                        */
            const L3Address& gwAddress = pkt->getTag<L3AddressInd>()->getSrcAddress();
            elem.possibleGateways.emplace_back(gwAddress, math::fraction2dB(frame->getSNIR()), frame->getRSSI());
            delete pkt;
            break;
        }
    }

    if(!packetExists)
    {
        receivedPacket rcvPkt;
        rcvPkt.rcvdPacket = pkt;
        rcvPkt.endOfWaiting = new cMessage("endOfWaitingWindow");
        //rcvPkt.endOfWaiting->setControlInfo(pkt);
        rcvPkt.endOfWaiting->setContextPointer(pkt);
        const L3Address& gwAddress = pkt->getTag<L3AddressInd>()->getSrcAddress();
        rcvPkt.possibleGateways.emplace_back(gwAddress, math::fraction2dB(frame->getSNIR()), frame->getRSSI());
        EV << "Added " << gwAddress << " " << math::fraction2dB(frame->getSNIR()) << " " << frame->getRSSI() << endl;
        //scheduleAt(simTime() + 1, rcvPkt.endOfWaiting);
        /*
                 * CHANGEMENT: Reduction du delai d'attente avant traitement.
                 *
                 * Ancienne version : scheduleAt(simTime() + 1, ...)  -> 1 seconde d'attente
                 * Nouvelle version : scheduleAt(simTime() + 0.1, ...) -> 0.1 seconde d'attente
                 *
                 * Ce delai represente la fenetre pendant laquelle le serveur attend d'eventuels
                 * duplicats du meme paquet envoyes par d'autres passerelles.

                 */
        scheduleAt(simTime()  + 0.1, rcvPkt.endOfWaiting);
        receivedPackets.push_back(rcvPkt);
    }
}

void LoRaNetworkServerApp::processScheduledPacket(cMessage* selfMsg)
{
    EV << "processScheduledPacket called at t=" << simTime() << endl;

        // Find the matching receivedPacket entry by context pointer
        Packet* pkt = check_and_cast<Packet *>((cObject*)selfMsg->getContextPointer());
        const auto & frame = pkt->peekAtFront<LoRaMacFrame>();

        EV << "Processing ACK for node: " << frame->getTransmitterAddress() << endl;
        EV << "DEBUG: SF=" << frame->getLoRaSF()
               << ", index=" << (frame->getLoRaSF()-7)
               << ", warmup=" << getSimulation()->getWarmupPeriod()
               << ", simTime=" << simTime() << endl;
    if (simTime() >= getSimulation()->getWarmupPeriod()){ // Le warm up est une phase initiale pendant laquelle le systeme se stabilise (les etats transitoires s estompent). Pendant cette periode, on evite generalement d enregistrer des statistiques ou de compter des evenements, car ils pourraient biaiser les resultats finaux.
        counterUniqueReceivedPacketsPerSF[frame->getLoRaSF()-7]++; }

    L3Address pickedGateway;
    double SNIRinGW = -99999999999;
    double RSSIinGW = -99999999999;
    // utilisees pour stocker respectivement le meilleur rapport signal-sur-bruit-plus-interference (SNIR) et la meilleure puissance recue (RSSI) parmi toutes les passerelles qui ont recu le meme paquet montant.
    int packetNumber;
    int nodeNumber;
    // Le serveur recoit effectivement un seul paquet d'un noeud donne, mais ce paquet peut avoir ete receptionne par plusieurs passerelles. La structure receivedPackets est une liste qui stocke temporairement tous les paquets montants en attente de traitement, chacun avec la liste des passerelles qui l ont recu (champ possibleGateways).
    for(uint i=0;i<receivedPackets.size();i++)
    {
        const auto &frameAux = receivedPackets[i].rcvdPacket->peekAtFront<LoRaMacFrame>();
        if(frameAux->getTransmitterAddress() == frame->getTransmitterAddress() && frameAux->getSequenceNumber() == frame->getSequenceNumber())
        {

            packetNumber = i;
            nodeNumber = frame->getTransmitterAddress().getInt();
            if (numReceivedPerNode.count(nodeNumber-1)>0)
                ++numReceivedPerNode[nodeNumber-1];
            else
                numReceivedPerNode[nodeNumber-1] = 1;

            for(uint j=0;j<receivedPackets[i].possibleGateways.size();j++)
            {
                //std::cout << "SNIRinGW: " << SNIRinGW << ", possibleGWSNIR: " << std::get<1>(receivedPackets[i].possibleGateways[j]) << endl;
                if(SNIRinGW < std::get<1>(receivedPackets[i].possibleGateways[j]))
                {
                    RSSIinGW = std::get<2>(receivedPackets[i].possibleGateways[j]);
                    SNIRinGW = std::get<1>(receivedPackets[i].possibleGateways[j]);
                    pickedGateway = std::get<0>(receivedPackets[i].possibleGateways[j]);
                }
            }
        }
    }


    emit(LoRa_ServerPacketReceived, true);


    int adrSF = -1;
    double adrTP = -1;
    bool hasADR = false;

    if(evaluateADRinServer)
        hasADR = evaluateADR(pkt, pickedGateway, SNIRinGW, RSSIinGW, adrSF, adrTP);

    if(WorkWithAck)
    {
        auto mgmtPacket = makeShared<LoRaAppPacket>();
        mgmtPacket->setMsgType(ACK);

        // Fusionner les options ADR dans l'ACK si disponibles
        if(hasADR && adrSF != -1)
        {
            LoRaOptions opts;
            opts.setLoRaSF(adrSF);
            opts.setLoRaTP(adrTP);
            mgmtPacket->setOptions(opts);
            EV << "Sending ACK with embedded ADR: SF=" << adrSF << " TP=" << adrTP << endl;
        }

        auto frameToSend = makeShared<LoRaMacFrame>();
        frameToSend->setChunkLength(B(par("headerLength").intValue()));
        frameToSend->setReceiverAddress(frame->getTransmitterAddress());
        frameToSend->setLoRaTP(math::dBmW2mW(14));
        frameToSend->setLoRaCF(frame->getLoRaCF());
        frameToSend->setLoRaSF(frame->getLoRaSF());
        frameToSend->setLoRaBW(frame->getLoRaBW());
        frameToSend->setLoRaCR(frame->getLoRaCR());
        frameToSend->setNumHop(frame->getNumHop());

        auto pktAux = new Packet("HI I AM AN ACK MESSAGE !");
        mgmtPacket->setChunkLength(B(par("headerLength").intValue()));
        pktAux->insertAtFront(mgmtPacket);
        pktAux->insertAtFront(frameToSend);
        send(pktAux, "out");
    }

    for (auto &node : knownNodes) {
        if (node.srcAddr == frame->getTransmitterAddress()) {
            int seqNo = frame->getSequenceNumber();
            if (node.processedSeqNumbers.find(seqNo)
                    == node.processedSeqNumbers.end()) {
                node.processedSeqNumbers.insert(seqNo);
                if (simTime() >= getSimulation()->getWarmupPeriod()){
                    counterUniqueReceivedPackets++;
                totalUniqueSentPackets++;}
            }
            break;
        }
    }
    receivedRSSI.collect(frame->getRSSI());
    //if(evaluateADRinServer)
      //  evaluateADR(pkt, pickedGateway, SNIRinGW, RSSIinGW);
    delete receivedPackets[packetNumber].rcvdPacket;
    delete selfMsg;
    receivedPackets.erase(receivedPackets.begin()+packetNumber);
}

bool LoRaNetworkServerApp::evaluateADR(Packet* pkt, L3Address pickedGateway,
                                        double SNIRinGW, double RSSIinGW,
                                        int &outSF, double &outTP)
{
    bool sendADR = false;
    bool sendADRAckRep = false;
    double SNRm;
    int nodeIndex;
    outSF = -1;
    outTP = -1;

    pkt->trimFront();
    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    const auto & rcvAppPacket = pkt->peekAtFront<LoRaAppPacket>();

    if(rcvAppPacket->getOptions().getADRACKReq())
        sendADRAckRep = true;

    for(uint i=0;i<knownNodes.size();i++)
    {
        if(knownNodes[i].srcAddr == frame->getTransmitterAddress())
        {
            knownNodes[i].adrListSNIR.push_back(SNIRinGW);
            knownNodes[i].historyAllSNIR->record(SNIRinGW);
            knownNodes[i].historyAllRSSI->record(RSSIinGW);
            knownNodes[i].receivedSeqNumber->record(frame->getSequenceNumber());
            if(knownNodes[i].adrListSNIR.size() == 20)
                knownNodes[i].adrListSNIR.pop_front();
            knownNodes[i].framesFromLastADRCommand++;

            if(knownNodes[i].framesFromLastADRCommand == 2 || sendADRAckRep)

            {
                nodeIndex = i;
                knownNodes[i].framesFromLastADRCommand = 0;
                sendADR = true;

                if(adrMethod == "max")
                    SNRm = *max_element(knownNodes[i].adrListSNIR.begin(), knownNodes[i].adrListSNIR.end());
                if(adrMethod == "avg")
                {
                    double totalSNR = 0; int n = 0;
                    for(auto it=knownNodes[i].adrListSNIR.begin(); it!=knownNodes[i].adrListSNIR.end(); ++it)
                    { totalSNR+=*it; n++; }
                    SNRm = totalSNR/n;
                }
            }
        }
    }

    if(sendADR)
    {
        double requiredSNR;
        if(frame->getLoRaSF()==7)  requiredSNR=-7.5;
        if(frame->getLoRaSF()==8)  requiredSNR=-10;
        if(frame->getLoRaSF()==9)  requiredSNR=-12.5;
        if(frame->getLoRaSF()==10) requiredSNR=-15;
        if(frame->getLoRaSF()==11) requiredSNR=-17.5;
        if(frame->getLoRaSF()==12) requiredSNR=-20;

        double SNRmargin = SNRm - requiredSNR - adrDeviceMargin;
        knownNodes[nodeIndex].calculatedSNRmargin->record(SNRmargin);
        int Nstep = round(SNRmargin/3);

        int calculatedSF = frame->getLoRaSF();
        while(Nstep > 0 && calculatedSF > 7) { calculatedSF--; Nstep--; }

        double calculatedPowerdBm = math::mW2dBmW(frame->getLoRaTP()) + 30;
        while(Nstep > 0 && calculatedPowerdBm > 2)  { calculatedPowerdBm-=3; Nstep--; }
        if(calculatedPowerdBm < 2) calculatedPowerdBm = 2;
        while(Nstep < 0 && calculatedPowerdBm < 14) { calculatedPowerdBm+=3; Nstep++; }
        if(calculatedPowerdBm > 14) calculatedPowerdBm = 14;

        outSF = calculatedSF;
        outTP = calculatedPowerdBm;

        if(simTime() >= getSimulation()->getWarmupPeriod())
            knownNodes[nodeIndex].numberOfSentADRPackets++;

        EV << "ADR calculated SF=" << calculatedSF << " TP=" << calculatedPowerdBm << endl;
    }

    return (sendADR || sendADRAckRep);
}

void LoRaNetworkServerApp::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    if (simTime() >= getSimulation()->getWarmupPeriod())
    {

            counterOfSentPacketsFromNodes++;
        counterOfSentPacketsFromNodesPerSF[value-7]++;
    }
}


}
