/*
 * PacketHandler.h
 *
 *  Created on: Mar 30, 2022
 *      Author: diego
 */

#ifndef SATELLITE_PACKETHANDLER_H_
#define SATELLITE_PACKETHANDLER_H_

#include <vector>
#include "inet/common/INETDefs.h"
#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/RadioControlInfo_m.h"

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

#include "mobility/INorad.h"

#include "LoRaPhy/LoRaRadioControlInfo_m.h"
#include "LoRa/LoRaMacControlInfo_m.h"
#include "LoRa/LoRaMacFrame_m.h"

namespace flora {

class PacketHandler : public cSimpleModule, public cListener
{

protected:
    int localPort = -1;
    int destPort = -1;
    int satIndex = 0;
    int maxHops = 1;

    int planes = 1;
    int satPlane = 0;

    int satRightIndex = -1;
    int satLeftIndex = -1;
    int satDownIndex = -1;
    int satUpIndex = -1;

    bool globalGrid;
    int numOfSatellites = 1;
    int numOfGroundStations = 1;

    INorad* noradModule = nullptr;

    cMessage *selfMsg = nullptr;

protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void sendPacket();
    void processLoraMACPacket(Packet *pk);
    void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;

    // initialize packet macFramType as UPINK or DOWNLINK and set route
    // tracing array length given constellation configuration
    void SetupRoute(Packet *pkt, int macFrameType);

    // checks if there exists at least one ground station link at the moment
    // current version only supports a ground link on sat 15 and only one GS
    bool groundStationAvailable();

    // checks if there lora sink node link is available at the moment
    bool loraNodeAvailable();

    // insert sat index and simulation time stamp in frame route
    void insertSatinRoute(Packet *pkt);

    // forwards message to the closest ground station
    void forwardToGround(Packet *pkt);

    // forwards message to lora node on ground
    void forwardToNode(Packet *pkt);

    // forwards message to another adjacent satellite
    void forwardToSatellite(Packet *pkt);


public:
    simsignal_t LoRa_GWPacketReceived;
    int counterOfSentPacketsFromNodes = 0;
    int counterOfReceivedPackets = 0;
    int sentToGround = 0;

    int rcvdFromLeftSat = 0;
    int rcvdFromDownSat = 0;
    int rcvdFromRightSat = 0;
    int rcvdFromUpSat = 0;
};

}

#endif /* SATELLITE_PACKETHANDLER_H_ */
