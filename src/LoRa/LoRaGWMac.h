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

#ifndef LORA_LORAGWMAC_H_
#define LORA_LORAGWMAC_H_

#include "inet/common/INETDefs.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/ModuleAccess.h"
//#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "LoRaMacControlInfo_m.h"
#include "LoRaMacFrame_m.h"
//#include "LoRaApp/LoRaAppPacket_m.h"

#if INET_VERSION < 0x0403 || ( INET_VERSION == 0x0403 && INET_PATCH_LEVEL == 0x00 )
#  error At least INET 4.3.1 is required. Please update your INET dependency and fully rebuild the project.
#endif
namespace flora {

using namespace inet;
using namespace inet::physicallayer;

class LoRaGWMac: public MacProtocolBase {
public:
    int totalBelowSensitivityReceptions = 0;

    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void configureNetworkInterface() override;
    long GW_forwardedDown;
    long GW_droppedDC;

    virtual void handleUpperMessage(cMessage *msg) override;
    virtual void handleLowerMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *message) override;

    void sendPacketBack(Packet *receivedFrame);
    void beaconScheduling();
    void sendBeacon();
    virtual MacAddress getAddress();

protected:
    simtime_t rx1Delay;        // delai avant RX1 (ex: 1s)
    simtime_t rx2Delay;        // delai avant RX2 (ex: 2s)
    std::map<MacAddress, simtime_t> lastUplinkTime;   // dernier uplink par noeud
    std::map<MacAddress, cMessage*> pendingTimers;    // timers d envoi differe
    std::map<MacAddress, Packet*> pendingPackets;     // paquets en attente

        void scheduleDownlink(Packet* pkt, MacAddress dest, simtime_t sendTime);
        void sendDownlinkNow(Packet* pkt);

    const char beaconSentText[13] = "Beacon sent!";

    bool isClassA = true;
    bool isClassB = false;
    bool isClassS = false;
    bool beaconGuard = false;

    int outGate = -1;
    MacAddress address;

    //bool waitingForDC;
    int satIndex;

    int lastSentMeasurement;
    int pingNumber;

    int beaconSF;
    int beaconCR = -1;
    double beaconTP;
    double beaconCF;
    double beaconBW;

    int beaconNumber = -1;
    int attemptedReceptionsPerSlot = 0;
    int successfulReceptionsPerSlot = 0;
    int belowSensitivityReceptions = 0;

    simtime_t beaconStart = -1;
    simtime_t beaconGuardTime = -1;
    simtime_t beaconReservedTime = -1;
    double beaconPeriodTime = -1;

    simtime_t maxToA = -1;
    simtime_t clockThreshold = -1;
    simtime_t classSslotTime = -1;

    //cMessage *dutyCycleTimer = nullptr;
    std::map<double, bool> waitingForDC;           // per-frequency duty cycle flag
    std::map<double, cMessage*> dutyCycleTimers;   // per-frequency duty cycle timers
    /** End of the beacon period */
    cMessage *beaconPeriod = nullptr;

    /** End of the beacon reserved period */
    cMessage *beaconReservedEnd = nullptr;

    /** Start of the beacon guard period */
    cMessage *beaconGuardStart = nullptr;

    /** End of uplink transmission slot */
    cMessage *endTXslot = nullptr;

    // status for each slot during simulation
    // 0 no reception attempts, slot is IDLE
    // 1 all reception attempts with low power, slot is IDLE
    // 2 single reception over sensitivity, slot is SUCCESSFUL
    // 3 at least two receptions collided, slot is COLLIDED
    // 4 at least two receptions collided, slot is COLLIDED
    cOutVector classSslotStatus;

    // beacon number of the corresponding slot
    cOutVector classSslotBeacon;

    // reception attempts per slot
    cOutVector classSslotReceptionAttempts;

    // successful receptions per slot, should be 0 or 1
    cOutVector classSslotReceptionSuccess;

    // reception below sensitivity per slot
    cOutVector classSslotReceptionBelowSensitivity;

    IRadio *radio = nullptr;
    IRadio::TransmissionState transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;

    const char *signalName;

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;
    bool centroidComputed = false;
        double centroidLat = 0.0;
        double centroidLon = 0.0;
        long beaconsSkippedVisibility = 0;   // diagnostic counter

        void computeNetworkCentroid();
};

}

#endif /* LORA_LORAGWMAC_H_ */
