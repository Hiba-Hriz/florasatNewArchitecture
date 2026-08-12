/*
 * LoRaNetworkServerApp.h
 *
 *  Created on: May 2, 2022
 *      Author: diego
 */

#ifndef GROUND_LORANETWORKSERVERAPP_H_
#define GROUND_LORANETWORKSERVERAPP_H_

#include <omnetpp.h>
#include "inet/physicallayer/wireless/common/contract/packetlevel/RadioControlInfo_m.h"
#include <vector>
#include <tuple>
#include <algorithm>
#include "inet/common/INETDefs.h"

#include "LoRa/LoRaMacControlInfo_m.h"
#include "LoRa/LoRaMacFrame_m.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "../LoRaApp/LoRaAppPacket_m.h"
#include <list>

namespace flora {

class knownNode
{
public:
    //cOutVector *historyAllMsg;
    MacAddress srcAddr;
    int framesFromLastADRCommand;
    int lastSeqNoProcessed;
    int numberOfSentADRPackets;

    std::list<double> adrListSNIR;
    cOutVector *historyAllSNIR;
    cOutVector *historyAllRSSI;
    cOutVector *receivedSeqNumber;
    cOutVector *calculatedSNRmargin;
    std::set<int> processedSeqNos;
    std::set<int> processedSeqNumbers;
};

class knownGW
{
public:
    L3Address ipAddr;
};

class receivedPacket
{
public:
    Packet* rcvdPacket = nullptr;
    bool isRetransmission = false;
    cMessage* endOfWaiting = nullptr;
    std::vector<std::tuple<L3Address, double, double>> possibleGateways; // <address, sinr, rssi>
};

class LoRaNetworkServerApp : public cSimpleModule, cListener
{
protected:
    std::vector<knownNode> knownNodes;
    std::vector<knownGW> knownGateways;
    std::vector<receivedPacket> receivedPackets;
    int localPort = -1, destPort = -1;
    std::vector<std::tuple<MacAddress, int>> recvdPackets;
    // state
    UdpSocket socket;
    std::map<int, std::set<int>> sentSeqPerNode;

    int totalUniqueSentPackets = 0;
    void sendAckForRetransmission(Packet *pk);

    cMessage *selfMsg = nullptr;
    int totalReceivedPackets;
    void ensureNodeKnown(Packet* pkt);

    std::string adrMethod;
    double adrDeviceMargin;
    std::map<int, int> numReceivedPerNode;

    //cMessage *BeaconTimer;
    double netServerDPAD;
    cOutVector dpadVector;
    cMessage *calculationYay;
    int calculationDelay = 1;

    int totalCase1;
    int totalCase2;
    int totalCase3;
    int totalCase4;
    double pathToRightGW[25];
    double followMe[25];
    int k=1201;

    bool WorkWithAck;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void processLoraMACPacket(Packet *pk);
    void startUDP();
    void setSocketOptions();
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    bool isPacketProcessed(const Ptr<const LoRaMacFrame> &);
    void updateKnownNodes(Packet* pkt);
    void addPktToProcessingTable(Packet* pkt);
    void processScheduledPacket(cMessage* selfMsg);
    bool evaluateADR(Packet* pkt, L3Address pickedGateway, double SNIRinGW, double RSSIinGW, int &outSF, double &outTP);
    void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;
    bool evaluateADRinServer;

    //void sendBeacon();
    void calculateDelayTime();
    double satToNodeDelayTime(int satNumber, int nodeNumber);

    cHistogram receivedRSSI;

public:
    simsignal_t LoRa_ServerPacketReceived;
    int counterOfSentPacketsFromNodes = 0;
    int counterOfSentPacketsFromNodesPerSF[6];
    int counterUniqueReceivedPackets = 0;
    int rec = 0;
    int counterUniqueReceivedPacketsPerSF[6];
};

} //namespace flora

#endif /* GROUND_LORANETWORKSERVERAPP_H_ */
