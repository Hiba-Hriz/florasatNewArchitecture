//
// Copyright (C) 2016 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <cmath>
#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/common/UserPriority.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/csmaca/CsmaCaMac.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "LoRaApp/SimpleLoRaApp.h"
#include "LoRaMac.h"
#include "LoRaTagInfo_m.h"
#include "LoRaPhy/LoRaTransmission.h"
using namespace std;

namespace flora {

Define_Module(LoRaMac);

LoRaMac::~LoRaMac()
{
    cancelAndDelete(endTransmission);
    cancelAndDelete(endReception);
    cancelAndDelete(droppedPacket);
    cancelAndDelete(pingPeriod);
    cancelAndDelete(beaconPeriod);
    cancelAndDelete(beaconReservedEnd);
    cancelAndDelete(beaconGuardStart);
    cancelAndDelete(beaconGuardEnd);
    cancelAndDelete(endPingSlot);
    cancelAndDelete(endDelay_1);
    cancelAndDelete(endListening_1);
    cancelAndDelete(endDelay_2);
    cancelAndDelete(endListening_2);
    cancelAndDelete(mediumStateChange);
    cancelAndDelete(beginTXslot);
    cancelAndDelete(retransmissionTimer);
    cancelAndDelete(dutyCycleTimer);

}

/****************************************************************
 * Initialization functions.
 */
void LoRaMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        EV << "Initializing stage 0\n";

        payloadBytes = par("payloadSize");
        // ===== Energy parameters =====
        supplyVoltage = 3.3;

        // XML values converted from mA to A
        cadCurrent = 0.0097;      // 9.7 mA
        idleCurrent = 0.0097;     // backoff currently uses RECEIVER mode

        totalCadEnergy = 0.0;
        totalBackoffEnergy = 0.0;

        totalCadTime = 0.0;
        totalBackoffTime = 0.0;

        totalCadOperations = 0;
        lostDueToVisibility = 0;





        // Parametre de retransmission
        numRetransmissions = 0;
        retransmissionPending = false;
        retransmissionTimer = new cMessage("RetransmissionTimer");
        WATCH(numRetransmissions);
        withRetransmission = par("withRetransmission");
        usingAck = par("usingAck");
        maxRetransmissions = par("maxRetransmissions");
        retransmissionCount = 0;
        EV << "Retransmission enabled=" << withRetransmission
           << ", maxRetransmissions=" << maxRetransmissions << endl;

        //maxQueueSize = par("maxQueueSize");
        headerLength = par("headerLength");
        ackLength = par("ackLength");
        ackTimeout = par("ackTimeout");
        retryLimit = par("retryLimit");

        // RX1 RX2 receive windows parameters
        waitDelay1Time = par("waitDelay1Time");
        listening1Time = par("listening1Time");
        waitDelay2Time = par("waitDelay2Time");
        listening2Time = par("listening2Time");

        cModule *nic = getParentModule();
        cModule *node = nic->getParentModule();
        auto loRaApp = check_and_cast<SimpleLoRaApp *>(node->getSubmodule("SimpleLoRaApp"));
        loRaSF = loRaApp->loRaSF;
        loRaCF = inet::Hz(868.1e6);

        dutyCycleTimer = new cMessage("DutyCycleTimer");

        // beacon parameters
        beaconStart = par("beaconStart");
        beaconPeriodTime = par("beaconPeriodTime");
        beaconReservedTime = par("beaconReservedTime");
        beaconGuardTime = par("beaconGuardTime");

        // class B parameters
        classBslotTime = par("classBslotTime");
        timeToNextSlot = par("timeToNextSlot");
        pingOffset = par("pingOffset");

        // class S parameters
        maxToA = par("maxToA");
        clockThreshold = par("clockThreshold");
        classSslotTime = 2*clockThreshold + maxToA;
        maxClassSslots = floor((beaconPeriodTime - beaconGuardTime - beaconReservedTime) / classSslotTime);

        slotSelectionData.setName("ClassSTXSlotSelection");
        //slotBeginTimes.setName("slotBeginTimes");
        useSlottedAloha = par("useSlottedAloha");


        const char *addressString = par("address");
        if (!strcmp(addressString, "auto")) {
            // assign automatic address
            address = MacAddress::generateAutoAddress();
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(address.str().c_str());
        }
        else
            address.setAddress(addressString);

        // subscribe for the information of the carrier sense
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(LoRaRadio::droppedPacket, this);
        radio = check_and_cast<IRadio *>(radioModule);

        // initialize self messages
        mediumStateChange = new cMessage("MediumStateChange");
        endTransmission = new cMessage("Transmission");
        endReception = new cMessage("Reception");
        droppedPacket = new cMessage("Dropped Packet");

        endDelay_1 = new cMessage("Delay_1");
        endListening_1 = new cMessage("Listening_1");
        endDelay_2 = new cMessage("Delay_2");
        endListening_2 = new cMessage("Listening_2");

        beaconGuardStart = new cMessage("Beacon_Guard_Start");
        beaconGuardEnd = new cMessage("Beacon_Guard_End");
        beaconPeriod = new cMessage("Beacon_Period");
        beaconReservedEnd = new cMessage("Beacon_Close");

        pingPeriod = new cMessage("Ping_Period");
        endPingSlot = new cMessage("Ping_Slot_Close");

        beginTXslot = new cMessage("UplinkSlot_Start");


        // set up internal queue
        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));

        // schedule beacon when using class B or S
        const char *usedClass = par("classUsed");
        if (strcmp(usedClass,"A"))
        {
            scheduleAt(simTime() + beaconStart, beaconPeriod);
            isClassA = false;

            if (!strcmp(usedClass,"B"))
                isClassB = true;

            if (!strcmp(usedClass,"S"))
                isClassS = true;
        }



        // FSA Game parameters
        FSAGame = par("FSAGame");
        if (FSAGame)
        {
            realNodeNumber = par("realNodeNumber");
            int id = (realNodeNumber-5) / 10;
            double b = 1 - (1.0/maxClassSslots);
            std::list<double>::iterator it = estimations.begin();
            advance(it, id);
            double nodeEstimation = *it;

            if (nodeEstimation <= -1.0/log(b))
                a = 1;
            else
                a = -1 / (nodeEstimation*log(b));

            //std::cout << "a= " << a << "; realNodeNumber= " << realNodeNumber << "; nodeEstimation= " << nodeEstimation << endl;
        }



        // state variables
        fsm.setName("LoRaMac State Machine");
        backoffPeriod = -1;
        retryCounter = 0;

        // sequence number for messages
        sequenceNumber = 0;
        bdw = 0;

        // statistics
        numRetry = 0;
        numSentWithoutRetry = 0;
        numGivenUp = 0;
        numCollision = 0;
        numSent = 0;
        numReceived = 0;
        numSentBroadcast = 0;
        numReceivedBroadcast = 0;
        numReceivedBeacons = 0;
        DPAD = 0;


        // initialize watches
        WATCH(fsm);
        WATCH(backoffPeriod);
        WATCH(retryCounter);
        WATCH(numRetry);
        WATCH(DPAD);

        WATCH(numSentWithoutRetry);
        WATCH(numGivenUp);
        WATCH(numCollision);
        WATCH(numSent);
        WATCH(numReceived);
        WATCH(numSentBroadcast);
        WATCH(numReceivedBroadcast);

        // Parametres CSMA/CAD
                        useCSMA = par("useCSMA");
                        if (useCSMA) {
                            difsCount = par("difsCount");           // Defaut: 2 CADs
                            backoffMax = par("backoffMax");         // Defaut: 6
                            maxChanges = par("maxChanges");         // Defaut: 6

                            cadAttempts = 0;

                            numBackoff = 0;


                            cadTimer = new cMessage("CAD_Timer");
                            backoffSlotTimer = new cMessage("Backoff_Slot");
                            availableChannels = {868.1e6, 868.3e6, 868.5e6};
                            numAvailableChannels = 3;
                            // Configuration CAD par defaut
                            cadSymbolNum = 2;
                        }


    }

    else if (stage == INITSTAGE_LINK_LAYER)
    {
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        macDPAD = registerSignal("macDPAD");
        macDPADOwnTraffic = registerSignal("macDPADOwnTraffic");
    }

}



void LoRaMac::finish()
{
    recordScalar("numRetry", numRetry);
    recordScalar("DPAD", DPAD);
    recordScalar("numSentWithoutRetry", numSentWithoutRetry);
    recordScalar("numGivenUp", numGivenUp);
    //recordScalar("numCollision", numCollision);
    recordScalar("numSent", numSent);
    recordScalar("numReceived", numReceived);
    recordScalar("numSentBroadcast", numSentBroadcast);
    recordScalar("numReceivedBroadcast", numReceivedBroadcast);
    recordScalar("numReceivedBeacons", numReceivedBeacons);
    recordScalar("numRetransmissions", numRetransmissions);

    recordScalar("Total CAD Energy (J)", totalCadEnergy);
    recordScalar("Total Backoff Energy (J)", totalBackoffEnergy);

    recordScalar("Total CAD Time (s)", totalCadTime);
    recordScalar("Total Backoff Time (s)", totalBackoffTime);

    recordScalar("Total CAD Operations", totalCadOperations);

    recordScalar("Total CSMA Energy (J)",
                 totalCadEnergy + totalBackoffEnergy);
    recordScalar("maxClassSslots", maxClassSslots);
        recordScalar("classSslotTime", classSslotTime);
        recordScalar("lostDueToVisibility", lostDueToVisibility);
        recordScalar("queueRemainingAtEnd", txQueue->getNumPackets());


}

void LoRaMac::configureNetworkInterface()
{
    //NetworkInterface *e = new NetworkInterface(this);

    // data rate
    networkInterface->setDatarate(bitrate);
    networkInterface->setMacAddress(address);

    // capabilities
    //interfaceEntry->setMtu(par("mtu"));
    networkInterface->setMtu(std::numeric_limits<int>::quiet_NaN());
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
    networkInterface->setPointToPoint(false);
}
/****************************************************************
 * Message handling functions.
 */
void LoRaMac::handleSelfMessage(cMessage *msg)
{


    if (msg == beaconPeriod)
    {
        beaconGuard = false;
        beaconScheduling();


        if (lastBeaconReceptionTime > 0 && !iGotBeacon)
        {

            double elapsed = (simTime() - lastBeaconReceptionTime).dbl();
            int periodsElapsed = (int)round(elapsed / beaconPeriodTime);
            if (periodsElapsed > 0)
                lastBeaconReceptionTime = lastBeaconReceptionTime
                                        + periodsElapsed * beaconPeriodTime;
            EV << "CLASS S: estimated lastBeaconReceptionTime updated to "
               << lastBeaconReceptionTime << endl;
        }
    }

    if (msg == beaconReservedEnd)
    {
        if (iGotBeacon)
        {
            numReceivedBeacons++;
            iGotBeacon = false;
            if (isClassS)
            {
                cancelEvent(beginTXslot);
                scheduleULslots();
            }
        }
        else if (isClassS && (retransmissionPending || !txQueue->isEmpty()))
        {
            cancelEvent(beginTXslot);
            if (!waitingForDC)
            {

                EV << "CLASS S: no beacon this period but retransmission pending, rescheduling" << endl;
                scheduleULslotsSlottedAloha();
            }

        }
    }

    if (msg == beaconGuardStart)
        beaconGuard = true;


    if (msg == endPingSlot)
    {
        simtime_t nextTime = (pingOffset*classBslotTime) + timeToNextSlot - classBslotTime;
        EV << "scheduling Next Ping Slot at " << simTime() + nextTime << endl;
        scheduleAt(simTime() + nextTime, pingPeriod);
        scheduleAt(simTime() + nextTime + classBslotTime, endPingSlot);
    }
    if (msg == dutyCycleTimer) {
        waitingForDC = false;
        EV << "DutyCycle 1%: TX allowed again" << endl;
        if (!isClassS) {
            if (!txQueue->isEmpty() && fsm.getState() == IDLE) {
                popTxQueue();
                handleWithFsm(currentTxFrame);
            }
        } else {

            if (retransmissionPending || !txQueue->isEmpty()) {
                EV << "CLASS S: DC expired, rescheduling transmission" << endl;
                scheduleULslotsSlottedAloha();
            }
        }
        return;
    }

    if (msg == retransmissionTimer) {
        retransmissionPending = false;
        if (currentTxFrame != nullptr) {
            if (useCSMA) {
                maxChanges = par("maxChanges");
                numBackoff = intuniform(1, backoffMax);
                availableChannels = {868.1e6, 868.3e6, 868.5e6};
                numAvailableChannels = 3;


            }
                handleWithFsm(currentTxFrame);

        }
        return;
    }
    if (useCSMA) {
                if (msg == cadTimer) {
                    handleDIFSExpiry();
                    return;
                }

                if (msg == backoffSlotTimer) {
                    handleBackoffSlotExpiry();
                    return;
                }
            }


    handleWithFsm(msg);
}
void LoRaMac::scheduleULslotsSlottedAloha()
{
    // Cancel any previously scheduled TX slot event before recomputing
    cancelEvent(beginTXslot);

    // Nothing to send, and no retransmission waiting -> nothing to schedule
    if (txQueue->isEmpty() && !retransmissionPending)
        return;

    // Slotted Aloha slots are referenced relative to the last received beacon
    simtime_t beaconPeriodStart = lastBeaconReceptionTime;

    // No beacon has been received yet -> can't compute slots, defer until later
    if (beaconPeriodStart <= 0)
    {
        retransmissionPending = true;
        return;
    }

    // If we're past the current beacon period, we missed our window for this
    // period -> mark pending and wait for the next beacon to restart scheduling
    if ((simTime() - beaconPeriodStart) > beaconPeriodTime)
    {
        retransmissionPending = true;
        EV << "CLASS S: outside beacon period, waiting for next beacon" << endl;
        return;
    }

    // Time elapsed since slots became available ( after the beacon
    // guard/reserved time), used to figure out which slot we're currently in
    simtime_t elapsed = simTime() - beaconPeriodStart - beaconReservedTime;
    int currentSlotIndex = (int)floor(elapsed.dbl() / classSslotTime);
    if (currentSlotIndex < 0)
        currentSlotIndex = 0;
    int nextSlotIndex = currentSlotIndex;

    simtime_t nextSlotTime = beaconPeriodStart
                           + beaconReservedTime
                           + nextSlotIndex * classSslotTime
                           + clockThreshold;

    while (nextSlotTime <= simTime())
    {
        nextSlotIndex++;

        if (nextSlotIndex >= maxClassSslots)
        {
            retransmissionPending = true;
            return;
        }

        nextSlotTime = beaconPeriodStart
                     + beaconReservedTime
                     + nextSlotIndex * classSslotTime
                     + clockThreshold;
    }

    if (nextSlotIndex >= maxClassSslots)
    {
        retransmissionPending = true;
        return;
    }



    // Duty cycle (DC) restriction check: if we're currently blocked by a duty
    // cycle timer, skip forward through slots until one starts after the
    // duty cycle timer expires
    if (waitingForDC && dutyCycleTimer->isScheduled())
    {
        simtime_t dcEnd = dutyCycleTimer->getArrivalTime();

        // Keep advancing to later slots as long as they still fall before
        // the duty cycle ends, and we haven't run out of slots
        while (nextSlotIndex < maxClassSslots - 1 && nextSlotTime < dcEnd)
        {
            nextSlotIndex++;
            nextSlotTime = beaconPeriodStart
                         + beaconReservedTime
                         + nextSlotIndex * classSslotTime
                         + clockThreshold;
        }

        // If even the last available slot still starts before the duty
        // cycle timer expires, no usable slot exists in this period
        if (nextSlotTime < dcEnd)
        {
            retransmissionPending = true;
            EV << "CLASS S SlottedAloha: DC blocks all slots, waiting for DC expiry" << endl;
            return;
        }
    }

    // A valid slot was found: record it and schedule the TX event for its start time
    targetClassSslot = nextSlotIndex;
    slotSelectionData.record(targetClassSslot);
    scheduleAt(nextSlotTime, beginTXslot);
}
void LoRaMac::startDIFS()
{

    EV << "CSMA: Starting DIFS (checking " << difsCount << " CADs)";

    EV << endl;

    cadAttempts = 0;
    // Allumer la radio pour ecouter le canal AVANT le timer
    LoRaRadio *loraRadio = check_and_cast<LoRaRadio *>(radio);
    loraRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    IRadio::RadioMode radioMode = radio->getRadioMode();
        IRadio::ReceptionState receptionState = radio->getReceptionState();
        EV << "dans startBackoff : RADIO MODE = " << radioMode
               << " RECEPTION STATE = " << receptionState << endl;
    // Calculer la duree d un CAD
    cModule *nic = getParentModule();
    cModule *node = nic->getParentModule();
    auto loRaApp = check_and_cast<SimpleLoRaApp *>(node->getSubmodule("SimpleLoRaApp"));
    double BW = loRaApp->loRaBW.get();
    double Tsym = pow(2, loRaApp->loRaSF) / BW;
    double cadDelay = Tsym * cadSymbolNum;
    // ===== CAD Energy =====
    double cadEnergy = supplyVoltage * cadCurrent * cadDelay;

    totalCadEnergy += cadEnergy;
    totalCadTime += cadDelay;
    totalCadOperations++;

    EV << "CAD Energy added = "
       << cadEnergy
       << " J, Total CAD Energy = "
       << totalCadEnergy << " J" << endl;

    if (cadTimer->isScheduled())
        cancelEvent(cadTimer);

    scheduleAt(simTime() + cadDelay , cadTimer);
}

void LoRaMac::handleDIFSExpiry()
{
    cadAttempts++;
    if (performCAD()) {
        // performCAD indique canal libre
        if (cadAttempts == difsCount) { // DIFS terminee
            EV << "CSMA: DIFS complete (" << difsCount << " clear CADs)" << endl;
            cadAttempts = 0;

            LoRaRadio *loraRadio = check_and_cast<LoRaRadio *>(radio);
            loraRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            if (numBackoff > 0) {
                EV << "CSMA: DIFS complete, entering backoff (NumBackoff=" << numBackoff << ")" << endl;
                startBackoff();
            }
            else {
                EV << "CSMA: No backoff (numBackoff=0), transmitting" << endl;
                //Le update AvailableCh / NumAvailableCh-- sont dans sendDataFrame
                handleWithFsm(currentTxFrame);
            }
        } else {    // DIFS en cours

            EV << "CSMA: DIFS in progress (" << cadAttempts
               << "/" << difsCount << " clear CADs)" << endl;

            cModule *nic = getParentModule();
            cModule *node = nic->getParentModule();
            auto loRaApp = check_and_cast<SimpleLoRaApp *>(node->getSubmodule("SimpleLoRaApp"));

            double BW = loRaApp->loRaBW.get();
            double Tsym = pow(2, loRaApp->loRaSF) / BW;
            double cadDelay = Tsym * cadSymbolNum;
            // ===== CAD energy during difs =====
            double cadEnergy = supplyVoltage * cadCurrent * cadDelay;

            totalCadEnergy += cadEnergy;
            totalCadTime += cadDelay;
            totalCadOperations++;

            scheduleAt(simTime() + cadDelay, cadTimer);
        }
    } else {
        LoRaRadio *loraRadio = check_and_cast<LoRaRadio *>(radio);
        loraRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        // VERIFIER SI ON A ATTEINT MaxChanges
        if (maxChanges > 0) {
            double newCF = changeChannel();
            cModule *nic = getParentModule();
            cModule *node = nic->getParentModule();
            auto loRaApp = check_and_cast<SimpleLoRaApp *>(node->getSubmodule("SimpleLoRaApp"));
            loRaApp->loRaCF = Hz(newCF);
            cadAttempts = 0;
            startDIFS();
        } else {
            EV << "CSMA: MaxChanges exhausted, ALOHA MODE" << endl;
            cadAttempts = 0;

            numBackoff = 0;
            loraRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            handleWithFsm(currentTxFrame);
        }

    }
}

void LoRaMac::startBackoff()
{

    EV << "CSMA: Starting backoff with " << numBackoff << " slots (max=" << backoffMax << ")" << endl;


    cModule *nic = getParentModule();
    cModule *node = nic->getParentModule();
    cModule *loRaAppModule = node->getSubmodule("SimpleLoRaApp");
    auto loRaApp = check_and_cast<SimpleLoRaApp *>(loRaAppModule);
    // Chaque slot correspond a cadSymbolNum symboles LoRa.
    double BW = loRaApp->loRaBW.get();
    double Tsym = pow(2, loRaApp->loRaSF) / BW; // Tsym est la duree d un symbole
    double slotDuration = Tsym * cadSymbolNum;  // temps d un slot de backoff.
    // ===== Backoff Energy =====
    double backoffEnergy =  supplyVoltage * idleCurrent * slotDuration;

    totalBackoffEnergy += backoffEnergy;
    totalBackoffTime += slotDuration;

    EV << "Backoff Energy added = "
       << backoffEnergy
       << " J, Total Backoff Energy = "
       << totalBackoffEnergy << " J" << endl;

    LoRaRadio *loraRadio = check_and_cast<LoRaRadio *>(radio);
    loraRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    IRadio::RadioMode radioMode = radio->getRadioMode();
    IRadio::ReceptionState receptionState = radio->getReceptionState();
    EV << "dans startBackoff : RADIO MODE = " << radioMode
           << " RECEPTION STATE = " << receptionState << endl;

    if (backoffSlotTimer->isScheduled())
        cancelEvent(backoffSlotTimer);
    // Planifie un evenement asynchrone (backoffSlotTimer) apres la duree du slot.
    scheduleAt(simTime() + slotDuration, backoffSlotTimer);
}

void LoRaMac::handleBackoffSlotExpiry()
{
    if (performCAD()) {    // canal libre
        numBackoff--;      // NumBackoff--
        EV << "CSMA: Channel clear, NumBackoff remaining=" << numBackoff << endl;

        if (numBackoff == 0) {
            // NumBackoff == 0 : transmettre
            EV << "CSMA: Backoff complete, transmitting" << endl;
            LoRaRadio *loraRadio = check_and_cast<LoRaRadio *>(radio);
            loraRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            handleWithFsm(currentTxFrame);
            return;
        }
        // NumBackoff > 0 : continuer le backoff
        scheduleNextBackoffSlot();
    }
    else {
        LoRaRadio *loraRadio = check_and_cast<LoRaRadio *>(radio);
        loraRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        if (maxChanges > 0) {
            EV << "CSMA: busy during backoff, changing channel (maxChanges=" << maxChanges << ")" << endl;
            cModule *nic = getParentModule();
            cModule *node = nic->getParentModule();
            auto loRaApp = check_and_cast<SimpleLoRaApp *>(node->getSubmodule("SimpleLoRaApp"));
            double newCF = changeChannel();   // maxChanges-- dedans
            loRaApp->loRaCF = Hz(newCF);
            cadAttempts = 0;
            startDIFS();
        } else {
            EV << "CSMA: MaxChanges exhausted, ALOHA MODE" << endl;
            cadAttempts = 0;
            numBackoff = 0;
            LoRaRadio *loraRadio2 = check_and_cast<LoRaRadio *>(radio);
            loraRadio2->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            handleWithFsm(currentTxFrame);
        }
    }
}
void LoRaMac::scheduleNextClassSslot()
{
    cancelEvent(beginTXslot);

    simtime_t beaconPeriodStart = lastBeaconReceptionTime;
    simtime_t elapsed = simTime() - beaconPeriodStart - beaconReservedTime;
    int currentSlotIndex = (int)floor(elapsed.dbl() / classSslotTime);
    if (currentSlotIndex < 0)
        currentSlotIndex = 0;
    int nextSlotIndex = currentSlotIndex;

    simtime_t nextSlotTime = beaconPeriodStart
                           + beaconReservedTime
                           + nextSlotIndex * classSslotTime
                           + clockThreshold;

    while (nextSlotTime <= simTime())
    {
        nextSlotIndex++;

        if (nextSlotIndex >= maxClassSslots)
        {
            retransmissionPending = true;
            return;
        }

        nextSlotTime = beaconPeriodStart
                     + beaconReservedTime
                     + nextSlotIndex * classSslotTime
                     + clockThreshold;
    }

    if (nextSlotIndex >= maxClassSslots)
    {
        retransmissionPending = true;
        return;
    }

    if (waitingForDC && dutyCycleTimer->isScheduled())
    {
        simtime_t dcEnd = dutyCycleTimer->getArrivalTime();
        while (nextSlotIndex < maxClassSslots - 1 && nextSlotTime < dcEnd)
        {
            nextSlotIndex++;
            nextSlotTime = beaconPeriodStart
                         + beaconReservedTime
                         + nextSlotIndex * classSslotTime
                         + clockThreshold;
        }
        if (nextSlotTime < dcEnd)
        {
            retransmissionPending = true;
            return;
        }
    }


    targetClassSslot = nextSlotIndex;
    scheduleAt(nextSlotTime, beginTXslot);

    EV << "CLASS S: retransmission scheduled at slot " << nextSlotIndex
       << " t=" << nextSlotTime
       << " (in " << (nextSlotTime - simTime()) << "s)" << endl;
}
void LoRaMac::scheduleNextBackoffSlot()
{
    cModule *nic = getParentModule();
    cModule *node = nic->getParentModule();
    auto loRaApp = check_and_cast<SimpleLoRaApp *>(node->getSubmodule("SimpleLoRaApp"));

    double BW = loRaApp->loRaBW.get();
    double Tsym = pow(2, loRaApp->loRaSF) / BW;
    double slotDuration = Tsym * cadSymbolNum;
    // ===== Backoff Energy =====
    double backoffEnergy =
        supplyVoltage * idleCurrent * slotDuration;

    totalBackoffEnergy += backoffEnergy;
    totalBackoffTime += slotDuration;

    EV << "Backoff Energy added = "
       << backoffEnergy
       << " J, Total Backoff Energy = "
       << totalBackoffEnergy << " J" << endl;


    if (backoffSlotTimer->isScheduled())
        cancelEvent(backoffSlotTimer);
    scheduleAt(simTime() + slotDuration, backoffSlotTimer);
}

double LoRaMac::changeChannel()
{
    maxChanges--;
    double currentCF = loRaCF.get();

    // Retirer le canal occupe
    availableChannels.erase(
        std::remove(availableChannels.begin(),
                    availableChannels.end(), currentCF),
        availableChannels.end()
    );
    numAvailableChannels = availableChannels.size();

    if (availableChannels.empty()) {
        EV << "CSMA: No channel available, ALOHA MODE" << endl;
        return currentCF;
    }

    int idx = intuniform(0, availableChannels.size() - 1);
    double newCF = availableChannels[idx];
    loRaCF = Hz(newCF);
    return newCF;
}

bool LoRaMac::performCAD()
{
    // Recupere la transmission en cours au niveau PHY
    auto transmission = radio->getReceptionInProgress();

    // Aucun signal detecte => canal libre
    if (!transmission)
        return true;
    auto loraSignal = dynamic_cast<const LoRaTransmission *>(transmission);

    // Un CAD LoRa detecte tout signal dont le SF correspond au notre.
    // Les SF sont quasi-orthogonaux : un SF different n'est pas detecte.
    if (loraSignal->getLoRaSF() != loRaSF)
        return true;  // SF different => canal considere libre

    // Canal occupe par un signal LoRa de meme SF
    return false;
}

void LoRaMac::handleUpperMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
    auto pktEncap = encapsulate(pkt);
    const auto &frame = pktEncap->peekAtFront<LoRaMacFrame>();
    txQueue->enqueuePacket(pktEncap);

    EV << "frame " << pktEncap << " received from higher layer, receiver = "
       << frame->getReceiverAddress() << endl;

    if (isClassS)
    {
        cancelEvent(beginTXslot);
        if (!retransmissionPending)
        {
            EV << "CLASS S: packet arrived mid-period, scheduling next available slot" << endl;
            scheduleULslotsSlottedAloha();
        }
        return;
    }

    if (fsm.getState() != IDLE)
        EV << "deferring upper message transmission in " << fsm.getStateName() << " state\n";
    else
    {
        if (waitingForDC) {
            EV << "DutyCycle 1%: TX blocked, packet queued" << endl;
            return;
        }
        popTxQueue();
        retransmissionCount = 0;
        if (useCSMA) {
            maxChanges = par("maxChanges");
            numBackoff = intuniform(1, backoffMax);
            availableChannels = {868.1e6, 868.3e6, 868.5e6};
            numAvailableChannels = 3;
        }
        handleWithFsm(currentTxFrame);
    }
}

void LoRaMac::handleLowerMessage(cMessage *msg)
{
    if( (fsm.getState() == RECEIVING_1) || (fsm.getState() == RECEIVING_2) ||
            (fsm.getState()== RECEIVING) || (fsm.getState()==RECEIVING_BEACON) )
    {
        auto pkt = check_and_cast<Packet *>(msg);
        const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

        if (isBeacon(frame))
        {
            int ping = pow(2,12)/frame->getPingNb();
            timeToNextSlot = ping*classBslotTime;
            EV << "time to next slot: " << timeToNextSlot<< endl;
        }

        if (isDownlink(frame))
        {
            EV << "RECEIVED A DOWNLINK MESSAGE WITH A DPAD OF : " << DPAD<<endl;
            emit(macDPAD, DPAD.dbl());
            if (isForUs(frame))
                emit(macDPADOwnTraffic, DPAD.dbl());
        }

        handleWithFsm(msg);
    }

    else
    {
        EV << "Received lower message but MAC FSM is not on a valid state for reception" << endl;
        EV << "Deleting message " << msg << endl;
        delete msg;
    }
}

void LoRaMac::handleWithFsm(cMessage *msg)
{
    Ptr<LoRaMacFrame>frame = nullptr;

    auto pkt = dynamic_cast<Packet *>(msg);
    if (pkt)
    {
        const auto &chunk = pkt->peekAtFront<Chunk>();
        frame = dynamicPtrCast<LoRaMacFrame>(constPtrCast<Chunk>(chunk));
    }

    if (isClassA)
    {
        FSMA_Switch(fsm)
        {
            // Verifica que el estado sea IDLE
            FSMA_State(IDLE)
            {
                // Realiza la accion turnOffReceiver()
                FSMA_Enter(turnOffReceiver());
                // FSMA_Event_Transition(transition, condition, target, action)
                FSMA_Event_Transition(Idle-Transmit,
                    isUpperMessage(msg),
                    useCSMA ? WAIT_DIFS : TRANSMIT,

                            if (useCSMA) {

                                  EV << "CLASS A: Starting CSMA with DIFS" << endl;



                                  startDIFS();
                                   } else {

                                      EV << "CLASS A: Starting transmission (pure aloha)" << endl;
                             }

                );
                FSMA_Event_Transition(Idle-Retransmit,
                    msg == currentTxFrame && retransmissionPending == false,
                    useCSMA ? WAIT_DIFS : TRANSMIT,
                    if (useCSMA) {
                        EV << "CLASS A: retransmitting with CSMA" << endl;
                        startDIFS();

                    }
                );
                  }




            // Etat WAIT_DIFS pour CSMA
                                FSMA_State(WAIT_DIFS)
                                {
                                    // Cet etat attend l appel de handleWithFsm(currentTxFrame)
                                    // Cette transition permet de passer a TRANSMIT quand le canal est libre
                                    FSMA_Event_Transition(WaitDIFS-Transmit,
                                                          msg == currentTxFrame,  // Message du paquet a transmettre
                                                          TRANSMIT,
                                                          EV << "CLASS A: DIFS complete, starting transmission" << endl;
                                                          );
                                }
            // Verifica que el estado sea TRANSMIT
            FSMA_State(TRANSMIT)
            {
                EV << "Previa de Transmit ACHF " << endl;
                FSMA_Enter(sendDataFrame(getCurrentTransmission()));


                FSMA_Event_Transition(Transmit-Wait_Delay_1,
                                      msg == endTransmission,
                                      WAIT_DELAY_1,
                                      EV << "CLASS A: transmission concluded" << endl;
                                      finishCurrentTransmission();
                                      numSent++;

                                      );
                EV << "Acabo de Transmit ACHF " << endl;
            }

            // Verifica que el estado sea WAIT DELAY 1
            FSMA_State(WAIT_DELAY_1)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_1-Listening_1,
                    msg == endDelay_1 || endDelay_1->isScheduled() == false,
                    LISTENING_1,
                    EV << "CLASS A: opening receive window 1" << endl;
                    retransmissionPending = false;
                    loRaCF = lastUplinkCF;
                    auto loraRadio = check_and_cast<LoRaRadio *>(radio);
                    loraRadio->setCenterFrequency(lastUplinkCF);
                    EV << "RX1 listening on CF = " << lastUplinkCF << endl;
                );
            }

            // Verifica que el estado sea LISTENING 1
            FSMA_State(LISTENING_1)
            {
                EV << "Previa de LISTENING 1 ACHF " << endl;

                FSMA_Enter(
                    if (usingAck) turnOnReceiver();

                );
                FSMA_Event_Transition(Listening_1-Wait_Delay_2,
                    msg == endListening_1 || endListening_1->isScheduled() == false,
                    WAIT_DELAY_2,
                    DPAD = simTime() - bdw;
                    EV << "CLASS A: didn't receive on RX1" << endl;
                );
                FSMA_Event_Transition(Listening_1-Receiving1,
                    msg == mediumStateChange && isReceiving(),
                    RECEIVING_1,
                    EV << "CLASS A: receiving on RX1" << endl;
                    DPAD = simTime() - bdw;
                );
                EV << "FIN de LISTENING 1 ACHF " << endl;
            }

            // Verifica que el estado sea RECEIVING 1
            FSMA_State(RECEIVING_1)
            {
                EV << "Previa de RECEIVING 1 ACHF " << endl;
                FSMA_Event_Transition(Receive-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_1,
                                      EV << "CLASS A: wrong address downlink received, back to listening on window 1" << endl;
                                      );
                FSMA_Event_Transition(Receive-Unicast,
                    isLowerMessage(msg) && isForUs(frame),
                    IDLE,
                    EV << "CLASS A: received downlink successfully on window 1, back to IDLE" << endl;
                    sendUp(decapsulate(pkt));
                    numReceived++;


                    cancelEvent(endListening_1);
                    cancelEvent(endDelay_2);
                    cancelEvent(endListening_2);

                    // Toujours nettoyer currentTxFrame apres reception ACK
                    if (currentTxFrame != nullptr) {
                        deleteCurrentTxFrame();
                        retransmissionCount = 0;
                        retransmissionPending = false;

                    }
                );
                FSMA_Event_Transition(Receive-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_1,
                                      EV << "CLASS A: low power downlink, back to listening on window 1" << endl;
                                      );
                EV << "FIN de RECEIVING 1 ACHF " << endl;
            }

            // Verifica que el estado sea WAIT DELAY 2
            FSMA_State(WAIT_DELAY_2)
            {
                EV << "Previa de WAIT DELAY 2 ACHF " << endl;
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_2-Listening_2,
                                      msg == endDelay_2 || endDelay_2->isScheduled() == false,
                                      LISTENING_2,
                                      EV << "CLASS A: opening receive window 2" << endl;
                                      auto loraRadio2 = check_and_cast<LoRaRadio *>(radio);
                                      loraRadio2->setCenterFrequency(inet::Hz(869525000));
                                      );
                EV << "FIN de WAIT DELAY 2 ACHF " << endl;
            }

            // Verifica que el estado sea LISTENING 2
            FSMA_State(LISTENING_2)
            {
                EV << "Previa de LISTENING 2 ACHF " << endl;
                // UN SEUL FSMA_Enter conditionnel
                FSMA_Enter(
                    if (usingAck) turnOnReceiver();
                    // Unconfirmed : radio reste en SLEEP
                );
                FSMA_Event_Transition(Listening_2-idle,
                    msg == endListening_2 || endListening_2->isScheduled() == false,
                    IDLE,
                    EV << "CLASS A: didn't receive on RX2" << endl;
                    if (withRetransmission && currentTxFrame != nullptr
                            && retransmissionCount < maxRetransmissions) {
                        retransmissionCount++;
                        numRetransmissions++;
                        retransmissionPending = true;
                        double retxDelay = uniform(1.0, 3.0);
                        if (retransmissionTimer->isScheduled())
                            cancelEvent(retransmissionTimer);
                        scheduleAt(simTime() + retxDelay, retransmissionTimer);
                    }
                    else if (withRetransmission && currentTxFrame != nullptr
                            && retransmissionCount >= maxRetransmissions) {

                        deleteCurrentTxFrame();
                        retransmissionCount = 0;
                        numGivenUp++;
                    }

                    else if (!usingAck && currentTxFrame != nullptr) {

                        deleteCurrentTxFrame();

                    }
                    else {
                        // Pure ALOHA sans retransmission avec ACK
                        if (currentTxFrame != nullptr) {
                            deleteCurrentTxFrame();
                            numGivenUp++;
                        }
                    }
                );
                FSMA_Event_Transition(Listening_2-Receiving2,
                    msg == mediumStateChange && isReceiving(),
                    RECEIVING_2,
                    EV << "CLASS A: receiving on RX2" << endl;
                    DPAD = simTime() - bdw;
                );
                EV << "FIN de LISTENING 2 ACHF " << endl;
            }

            // Verifica que el estado sea RECEIVING 2
            FSMA_State(RECEIVING_2)
            {
                EV << "Previa de RECEIVING 2 ACHF " << endl;
                FSMA_Event_Transition(Receive2-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_2,
                                      EV << "CLASS A: wrong address downlink received, back to listening on window 2" << endl;
                                      );
                FSMA_Event_Transition(Receive2-Unicast,
                    isLowerMessage(msg) && isForUs(frame),
                    IDLE,
                    EV << "CLASS A: received downlink successfully on window 2, back to IDLE" << endl;
                    sendUp(pkt);
                    numReceived++;


                    cancelEvent(endListening_2);

                    // Toujours nettoyer currentTxFrame apres reception ACK
                    if (currentTxFrame != nullptr) {
                        deleteCurrentTxFrame();
                        retransmissionCount = 0;
                        retransmissionPending = false;

                    }
                );
                FSMA_Event_Transition(Receive2-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_2,
                                      EV << "CLASS A: low power downlink, back to listening on window 2" << endl;
                                      );
                EV << "FIN de RECEIVING 2 ACHF " << endl;
            }
        }
    }

    // THE FSM FOR CLASS B
    if (isClassB)
    {
        FSMA_Switch(fsm)
        {
            FSMA_State(IDLE)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Idle-BeaconReception,
                                      msg == beaconPeriod,
                                      BEACON_RECEPTION,
                                      EV << "CLASS B: Going to Beacon Reception" << endl;
                                      );
                FSMA_Event_Transition(Idle-Transmit,
                                      isUpperMessage(msg),
                                      TRANSMIT,
                                      EV << "CLASS B: starting transmission" << endl;
                                      );
                FSMA_Event_Transition(Idle-ListeningOnPingSlot,
                                      msg == pingPeriod && !beaconGuard,
                                      PING_SLOT,
                                      EV << "CLASS B: starting Ping Slot" << endl;
                                      );
            }

            FSMA_State(BEACON_RECEPTION)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(BeaconReception-Idle,
                                      msg == beaconReservedEnd,
                                      IDLE,
                                      EV << "CLASS B: no beacon detected, increasing beacon time" << endl;
                                      increaseBeaconTime();
                                      );
                FSMA_Event_Transition(BeaconReception-ReceivingBeacon,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_BEACON,
                                      EV << "CLASS B: Going to Receiving Beacon" << endl;
                                      );
            }

            FSMA_State(RECEIVING_BEACON)
            {
                FSMA_Event_Transition(ReceivingBeacon-Unicast-Not-For,
                                      isLowerMessage(msg) && isBeacon(frame),  //  && !isForUs(frame)
                                      IDLE,
                                      EV << "CLASS B: beacon received" << endl;
                                      calculatePingPeriod(frame);
                                      );
                FSMA_Event_Transition(ReceivingBeacon-BelowSensitivity,
                                      msg == droppedPacket,
                                      IDLE,
                                      EV << "CLASS B: beacon below sensitivity" << endl;
                                      increaseBeaconTime();
                                      );
            }

            FSMA_State(PING_SLOT)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(ListeningOnPingSlot-Idle,
                                      msg == endPingSlot && !isReceiving(),
                                      IDLE,
                                      EV << "CLASS B: no downlink detected, back to IDLE" << endl;
                                      );
                FSMA_Event_Transition(ListeningOnPingSlot-ReceivingOnPingSlot,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING,
                                      EV << "CLASS B: going to receive downlink on ping slot" << endl;
                                      );
            }

            FSMA_State(RECEIVING)
            {
                FSMA_Event_Transition(ReceivingOnPingSlot-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: wrong address downlink on ping slot, back to IDLE" << endl;
                                      );
                FSMA_Event_Transition(ReceivingOnPingSlot-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: received downlink on ping slot, back to IDLE" << endl;
                                      sendUp(decapsulate(pkt));
                                      numReceived++;
                                      );
                FSMA_Event_Transition(ReceivingOnPingSlot-BelowSensitivity,
                                      msg == droppedPacket,
                                      IDLE,
                                      EV << "CLASS B: downlink below sensitivity, back to IDLE" << endl;
                                      );
            }

            FSMA_State(TRANSMIT)
            {

                FSMA_Enter(sendDataFrame(getCurrentTransmission()));
                FSMA_Event_Transition(Transmit-Wait_Delay_1,
                                      msg == endTransmission,
                                      WAIT_DELAY_1,
                                      EV << "CLASS B: transmission concluded" << endl;
                                      finishCurrentTransmission();
                                      numSent++;
                                      );
            }

            FSMA_State(WAIT_DELAY_1)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_1-Listening_1,
                                      msg == endDelay_1 || endDelay_1->isScheduled() == false,
                                      LISTENING_1,
                                      EV << "CLASS B: opening receive window 1" << endl;
                                      );
            }

            FSMA_State(LISTENING_1)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(Listening_1-Wait_Delay_2,
                                      msg == endListening_1 || endListening_1->isScheduled() == false,
                                      WAIT_DELAY_2,
                                      EV << "CLASS B: didn t receive downlink on receive window 1" << endl;
                                      );
                FSMA_Event_Transition(Listening_1-Receiving1,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_1,
                                      EV << "CLASS B: receiving a message on receive window 1, analyzing packet..." << endl;
                                      );
            }

            FSMA_State(RECEIVING_1)
            {
                FSMA_Event_Transition(Receive-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_1,
                                      EV << "CLASS B: wrong address downlink received, back to listening on window 1" << endl;
                                      );
                FSMA_Event_Transition(Receive-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: received downlink successfully on window 1, back to IDLE" << endl;
                                      sendUp(decapsulate(pkt));
                                      numReceived++;

                                      cancelEvent(endListening_1);
                                      cancelEvent(endDelay_2);
                                      cancelEvent(endListening_2);
                                      );
                FSMA_Event_Transition(Receive-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_1,
                                      EV << "CLASS B: low power downlink, back to listening on window 2" << endl;
                                      );
            }

            FSMA_State(WAIT_DELAY_2)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_2-Listening_2,
                                      msg == endDelay_2 || endDelay_2->isScheduled() == false,
                                      LISTENING_2,
                                      EV << "CLASS B: opening receive window 2" << endl;
                                      );
            }

            FSMA_State(LISTENING_2)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(Listening_2-idle,
                                      msg == endListening_2 || endListening_2->isScheduled() == false,
                                      IDLE,
                                      EV << "CLASS B: didn t receive downlink on receive window 2" << endl;
                                      );
                FSMA_Event_Transition(Listening_2-Receiving2,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_2,
                                      EV << "CLASS B: receiving a message on receive window 2, analyzing packet..." << endl;
                                      );
            }

            FSMA_State(RECEIVING_2)
            {
                FSMA_Event_Transition(Receive2-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_2,
                                      EV << "CLASS B: wrong address downlink received, back to listening on window 2" << endl;
                );
                FSMA_Event_Transition(Receive2-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: received downlink successfully on window 2, back to IDLE" << endl;
                                      sendUp(pkt);
                                      numReceived++;
                                      cancelEvent(endListening_2);
                                      );
                FSMA_Event_Transition(Receive2-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_2,
                                      EV << "CLASS B: low power downlink, back to listening on window 2" << endl;
                                      );
            }
        }
    }

    // THE FSM FOR CLASS S
    // THE FSM FOR CLASS S
    if (isClassS)
    {
        FSMA_Switch(fsm)
        {
            FSMA_State(IDLE)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Idle-BeaconReception,
                                      msg == beaconPeriod,
                                      BEACON_RECEPTION,
                                      EV << "CLASS S: Going to Beacon Reception" << endl;
                                      );
                FSMA_Event_Transition(Idle-UplinkSlot,
                                      msg == beginTXslot && timeToTrasmit(),
                                      TRANSMIT,
                                      EV << "CLASS S: entering uplink slot" << endl;
                                      );
                FSMA_Event_Transition(Idle-UplinkSlot-BeaconGuard,
                    msg == beginTXslot && !timeToTrasmit() && beaconGuard,
                    IDLE,
                    EV << "CLASS S: slot skipped (beacon guard), rescheduling after guard" << endl;
                    // Reschedule after beacon guard ends
                    scheduleULslotsSlottedAloha();
                );
            }

            FSMA_State(BEACON_RECEPTION)
            {
                FSMA_Enter(
                        auto loraRadio = check_and_cast<LoRaRadio *>(radio);

                        loraRadio->setCenterFrequency(inet::Hz(869525000));
                        turnOnReceiver());
                FSMA_Event_Transition(BeaconReception-Idle,
                                      msg == beaconReservedEnd,
                                      IDLE,
                                      EV << "CLASS S: no beacon detected, increasing beacon time" << endl;
                                      increaseBeaconTime();
                                      // Restaurer la frequence uplink
                                      auto loraRadio = check_and_cast<LoRaRadio *>(radio);
                                      loraRadio->setCenterFrequency(loRaCF);
                                      );
                FSMA_Event_Transition(BeaconReception-ReceivingBeacon,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_BEACON,
                                      EV << "CLASS S: Going to Receiving Beacon" << endl;

                                      );
            }

            FSMA_State(RECEIVING_BEACON)
            {
                FSMA_Event_Transition(ReceivingBeacon-Beacon,
                    isLowerMessage(msg) && isBeacon(frame),
                    IDLE,
                    EV << "CLASS S: beacon received" << endl;
                    iGotBeacon = true;
                    lastBeaconReceptionTime = simTime();

                    auto loraRadio = check_and_cast<LoRaRadio *>(radio);
                    loraRadio->setCenterFrequency(loRaCF);
                );
                FSMA_Event_Transition(ReceivingBeacon-BelowSensitivity,
                                      msg == droppedPacket,
                                      IDLE,
                                      EV << "CLASS S: beacon below sensitivity" << endl;
                                      increaseBeaconTime();
                                      );
            }

            FSMA_State(TRANSMIT)
            {
                FSMA_Enter(sendDataFrame(getCurrentTransmission()));
                FSMA_Event_Transition(Transmit-WaitDelay1,
                                      msg == endTransmission,
                                      WAIT_DELAY_1,
                                      EV << "CLASS S: transmission concluded" << endl;
                                      finishCurrentTransmission();
                                      numSent++;
                                      );
            }

            FSMA_State(WAIT_DELAY_1)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(WaitDelay1-Listening1,
                                      msg == endDelay_1 || endDelay_1->isScheduled() == false,
                                      LISTENING_1,
                                      EV << "CLASS S: opening RX1 window" << endl;
                                      auto loraRadio = check_and_cast<LoRaRadio *>(radio);
                                      loraRadio->setCenterFrequency(lastUplinkCF);
                                      );
            }

            FSMA_State(LISTENING_1)
            {
                FSMA_Enter(
                    if (usingAck) turnOnReceiver();
                );
                FSMA_Event_Transition(Listening1-WaitDelay2,
                                      msg == endListening_1 || endListening_1->isScheduled() == false,
                                      WAIT_DELAY_2,
                                      DPAD = simTime() - bdw;
                                      EV << "CLASS S: no ACK on RX1" << endl;
                                      );
                FSMA_Event_Transition(Listening1-Receiving1,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_1,
                                      DPAD = simTime() - bdw;
                                      EV << "CLASS S: receiving on RX1" << endl;
                                      );
            }

            FSMA_State(RECEIVING_1)
            {
                FSMA_Event_Transition(Receiving1-WrongAddr,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_1,
                                      EV << "CLASS S: wrong address on RX1" << endl;
                                      );
                FSMA_Event_Transition(Receiving1-GotAck,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS S: ACK received on RX1, back to IDLE" << endl;
                                      sendUp(decapsulate(pkt));
                                      numReceived++;
                                      cancelEvent(endListening_1);
                                      cancelEvent(endDelay_2);
                                      cancelEvent(endListening_2);
                                      if (currentTxFrame != nullptr) {
                                          deleteCurrentTxFrame();
                                          retransmissionCount = 0;
                                          retransmissionPending = false;
                                      }
                                      // relancer la planification s il reste des paquets ----
                                      if (!txQueue->isEmpty() && !waitingForDC) {
                                          if (useSlottedAloha)
                                              scheduleULslotsSlottedAloha();
                                          else
                                              scheduleNextClassSslot();
                                      }
                                      );
                FSMA_Event_Transition(Receiving1-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_1,
                                      EV << "CLASS S: weak signal on RX1" << endl;
                                      );
            }

            FSMA_State(WAIT_DELAY_2)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(WaitDelay2-Listening2,
                                      msg == endDelay_2 || endDelay_2->isScheduled() == false,
                                      LISTENING_2,
                                      EV << "CLASS S: opening RX2 window" << endl;
                                      auto loraRadio2 = check_and_cast<LoRaRadio *>(radio);
                                      loraRadio2->setCenterFrequency(inet::Hz(869525000));
                                      );
            }

            FSMA_State(LISTENING_2)
            {
                FSMA_Enter(
                    if (usingAck) turnOnReceiver();
                );
                FSMA_Event_Transition(Listening2-Idle,
                    msg == endListening_2 || endListening_2->isScheduled() == false,
                    IDLE,
                    EV << "CLASS S: no ACK on RX2" << endl;
                    if (withRetransmission && currentTxFrame != nullptr
                            && retransmissionCount < maxRetransmissions) {
                        retransmissionCount++;
                        numRetransmissions++;
                        retransmissionPending = true;
                        if (useSlottedAloha)
                            scheduleULslotsSlottedAloha();
                        else
                            scheduleNextClassSslot();

                    }
                    else if (withRetransmission && currentTxFrame != nullptr
                            && retransmissionCount >= maxRetransmissions) {
                        deleteCurrentTxFrame();
                        retransmissionCount = 0;
                        retransmissionPending = false;
                        numGivenUp++;

                        if (!txQueue->isEmpty() && !waitingForDC) {
                            if (useSlottedAloha)
                                scheduleULslotsSlottedAloha();
                            else
                                scheduleNextClassSslot();
                        }
                    }
                    else {
                        if (currentTxFrame != nullptr) {
                            deleteCurrentTxFrame();
                            retransmissionPending = false;
                        }

                        if (!txQueue->isEmpty() && !waitingForDC) {
                            if (useSlottedAloha)
                                scheduleULslotsSlottedAloha();
                            else
                                scheduleNextClassSslot();
                        }
                    }
                    );
                FSMA_Event_Transition(Listening2-Receiving2,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_2,
                                      DPAD = simTime() - bdw;
                                      EV << "CLASS S: receiving on RX2" << endl;
                                      );
            }

            FSMA_State(RECEIVING_2)
            {
                FSMA_Event_Transition(Receiving2-WrongAddr,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_2,
                                      EV << "CLASS S: wrong address on RX2" << endl;
                                      );
                FSMA_Event_Transition(Receiving2-GotAck,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS S: ACK received on RX2, back to IDLE" << endl;
                                      sendUp(pkt);
                                      numReceived++;
                                      cancelEvent(endListening_2);
                                      if (currentTxFrame != nullptr) {
                                          deleteCurrentTxFrame();
                                          retransmissionCount = 0;
                                          retransmissionPending = false;
                                      }

                                      if (!txQueue->isEmpty() && !waitingForDC) {
                                          if (useSlottedAloha)
                                              scheduleULslotsSlottedAloha();
                                          else
                                              scheduleNextClassSslot();
                                      }
                                      );
                FSMA_Event_Transition(Receiving2-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_2,
                                      EV << "CLASS S: weak signal on RX2" << endl;
                                      );
            }
        }
    }


    if (fsm.getState() == IDLE)
    {
        if (isReceiving())
            handleWithFsm(mediumStateChange);


        else if (currentTxFrame != nullptr && !isClassS) {


             if (withRetransmission) {
                if (retransmissionPending)
                    EV << "Retransmission pending, waiting for timer" << endl;
                else
                    handleWithFsm(currentTxFrame);
            }
        }


        else if (!txQueue->isEmpty() && !isClassS)
        {
            popTxQueue();


                    handleWithFsm(currentTxFrame);

        }
    }

    if (endSifs)
    {
        if (isLowerMessage(msg) && pkt->getOwner() == this && (endSifs->getContextPointer() != pkt))
            delete pkt;
    }

    else
    {
        if (isLowerMessage(msg) && pkt->getOwner() == this)
            delete pkt;
    }

    getDisplayString().setTagArg("t", 0, fsm.getStateName());
}

void LoRaMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::receptionStateChangedSignal)
        {
            IRadio::ReceptionState newRadioReceptionState = (IRadio::ReceptionState)value;

            if (receptionState == IRadio::RECEPTION_STATE_RECEIVING &&
                newRadioReceptionState != IRadio::RECEPTION_STATE_RECEIVING)
            {
                // Radio was receiving, now it's not   put it to sleep
                // Only if not in a state that needs the receiver on
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            }

            receptionState = newRadioReceptionState;
            handleWithFsm(mediumStateChange);
        }
    else if (signalID == LoRaRadio::droppedPacket)
    {
        //radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        handleWithFsm(droppedPacket);
    }
    else if (signalID == IRadio::transmissionStateChangedSignal)
    {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
        {
            handleWithFsm(endTransmission);
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        }
        transmissionState = newRadioTransmissionState;
    }
}

Packet *LoRaMac::encapsulate(Packet *msg)
{
    auto frame = makeShared<LoRaMacFrame>();
    frame->setChunkLength(B(headerLength));
    msg->setArrival(msg->getArrivalModuleId(), msg->getArrivalGateId());
    auto tag = msg->getTag<LoRaTag>();

    frame->setTransmitterAddress(address);
    frame->setLoRaTP(tag->getPower().get());
    frame->setLoRaCF(tag->getCenterFrequency());
    frame->setLoRaSF(tag->getSpreadFactor());
    frame->setLoRaBW(tag->getBandwidth());
    frame->setLoRaCR(tag->getCodeRendundance());
    frame->setOriginTime(simTime());
    frame->setSequenceNumber(sequenceNumber);
    frame->setReceiverAddress(MacAddress::BROADCAST_ADDRESS);

    ++sequenceNumber;
    frame->setLoRaUseHeader(tag->getUseHeader());

    msg->insertAtFront(frame);

    return msg;
}

Packet *LoRaMac::decapsulate(Packet *frame)
{
    auto loraHeader = frame->popAtFront<LoRaMacFrame>();
    frame->addTagIfAbsent<MacAddressInd>()->setSrcAddress(loraHeader->getTransmitterAddress());
    frame->addTagIfAbsent<MacAddressInd>()->setDestAddress(loraHeader->getReceiverAddress());
    frame->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface->getInterfaceId());
    return frame;
}

/****************************************************************
 * Frame sender functions.
 */
void LoRaMac::sendDataFrame(Packet *frameToSend)
{
    EV << "sending Data frame ACHF\n";

    const auto &frame = currentTxFrame->peekAtFront<LoRaMacFrame>();
        lastUplinkCF = frame->getLoRaCF();
        loRaCF = lastUplinkCF;          // sync MAC CF
        loRaSF = frame->getLoRaSF();    // sync MAC SF
        EV << "Saved lastUplinkCF = " << lastUplinkCF
           << " loRaSF = " << loRaSF << endl;
    if (useCSMA) {
            double usedCF = loRaCF.get();
            availableChannels.erase(
                std::remove(availableChannels.begin(),
                            availableChannels.end(), usedCF),
                availableChannels.end()
            );
            numAvailableChannels = availableChannels.size();
        }
    radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

    auto frameCopy = frameToSend->dup();

    //LoRaMacControlInfo *ctrl = new LoRaMacControlInfo();
    //ctrl->setSrc(frameCopy->getTransmitterAddress());
    //ctrl->setDest(frameCopy->getReceiverAddress());
    //frameCopy->setControlInfo(ctrl);

    auto macHeader = frameCopy->peekAtFront<LoRaMacFrame>();
    auto macAddressInd = frameCopy->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(macHeader->getTransmitterAddress());
    macAddressInd->setDestAddress(macHeader->getReceiverAddress());

    //frameCopy->addTag<PacketProtocolTag>()->setProtocol(&Protocol::lora);

    sendDown(frameCopy);
}

void LoRaMac::sendAckFrame()
{
    auto frameToAck = static_cast<Packet *>(endSifs->getContextPointer());
    endSifs->setContextPointer(nullptr);
    auto macHeader = makeShared<CsmaCaMacAckHeader>();
    macHeader->setReceiverAddress(MacAddress(frameToAck->peekAtFront<LoRaMacFrame>()->getTransmitterAddress().getInt()));

    EV << "sending Ack frame\n";
    //auto macHeader = makeShared<CsmaCaMacAckHeader>();
    macHeader->setChunkLength(B(ackLength));
    auto frame = new Packet("CsmaAck");
    frame->insertAtFront(macHeader);
    //frame->addTag<PacketProtocolTag>()->setProtocol(&Protocol::lora);
    radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

    auto macAddressInd = frame->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(macHeader->getTransmitterAddress());
    macAddressInd->setDestAddress(macHeader->getReceiverAddress());

    sendDown(frame);
}

/****************************************************************
 * Helper functions.
 */

// schedule beacon signals
void LoRaMac::beaconScheduling()
{
    scheduleAt(simTime() + beaconPeriodTime, beaconPeriod);
    scheduleAt(simTime() + beaconReservedTime, beaconReservedEnd);
    scheduleAt(simTime() + beaconPeriodTime - beaconGuardTime, beaconGuardStart);
}

void LoRaMac::increaseBeaconTime()
{
    beaconReservedTime = beaconReservedTime + 1;
}

void LoRaMac::schedulePingPeriod()
{
    cancelEvent(pingPeriod);
    cancelEvent(endPingSlot);
    scheduleAt(simTime() + (pingOffset*classBslotTime), pingPeriod);
    scheduleAt(simTime() + (pingOffset*classBslotTime) + classBslotTime, endPingSlot);
}

void LoRaMac::scheduleULslots()
{
    cancelEvent(beginTXslot);
    EV << "Queue size = " << txQueue->getNumPackets() << endl;

        // Slotted ALOHA (prochain slot)
        if (useSlottedAloha)
        {
            scheduleULslotsSlottedAloha();
            return;
        }
    // when beacon is received begin scheduling of uplink slots
    // and randomly determine the slot to be used during this beacon period
    targetClassSslot = cComponent::intuniform(0, maxClassSslots-1);
    slotSelectionData.record(targetClassSslot);

    cancelEvent(beginTXslot);
    scheduleAt(simTime() + clockThreshold + targetClassSslot*classSslotTime, beginTXslot);
}

//calculate the pingSlotPeriod using Aes128 encryption for randomization
void LoRaMac::calculatePingPeriod(const Ptr<const LoRaMacFrame> &frame)
{
    iGotBeacon = true;
    beaconReservedTime = 2.120;
    unsigned char cipher[7];

    cipher[0]=(unsigned char)(frame->getBeaconTimer()
            + getAddress().getAddressByte(0)
            + getAddress().getAddressByte(1)
            + getAddress().getAddressByte(2)
            + getAddress().getAddressByte(3)
            + getAddress().getAddressByte(4)
            + getAddress().getAddressByte(5)
            );
    cipher[1]=(unsigned char)(getAddress().getAddressByte(0));
    cipher[2]=(unsigned char)(getAddress().getAddressByte(1));
    cipher[3]=(unsigned char)(getAddress().getAddressByte(2));
    cipher[4]=(unsigned char)(getAddress().getAddressByte(3));
    cipher[5]=(unsigned char)(getAddress().getAddressByte(4));
    cipher[6]=(unsigned char)(getAddress().getAddressByte(5));

    int message_len = strlen((const char*)cipher);
    unsigned char cipher2[64];
    unsigned char* key = (unsigned char*)"00000000000000000000000000000000";

    int cipher_len = aesEncrypt(cipher,message_len,key,cipher2);
    int period = pow(2,12)/(frame->getPingNb());

    pingOffset = (cipher2[0]+(cipher2[1]*256))% period;
}

int LoRaMac::aesEncrypt(unsigned char *message, int message_len, unsigned char *key, unsigned char *cipher)
{
    int cipher_len = 0;
    int len = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if(!ctx){
        perror("EVP_SIPHER_CTX_new() failed");
        exit(-1);
    }
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL)){
        perror("EVP_EncryptInit_ex() failed");
        exit(-1);
    }
    if (!EVP_EncryptUpdate(ctx, cipher, &len, message, message_len)){
        perror("EVP_EncryptUpdate() failed");
        exit(-1);
    }
    cipher_len += len;
    if (!EVP_EncryptFinal_ex(ctx, cipher + len, &len)){
        perror("EVP_EnryptFinal_ex() failed");
        exit(-1);
    }
    cipher_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return cipher_len;
}

void LoRaMac::finishCurrentTransmission()  //Radio termine la transmission physique
{
    if (isClassS) {
            const auto &frame = currentTxFrame->peekAtFront<LoRaMacFrame>();
            int nPreamble = 8;
            int payloadSymbNb = 8;
            payloadSymbNb += (int)std::ceil(
                (8.0*payloadBytes - 4.0*frame->getLoRaSF() + 28.0 + 16.0)
                / (4.0*(frame->getLoRaSF() - 2*0))
            ) * (frame->getLoRaCR() + 4);
            if (payloadSymbNb < 8) payloadSymbNb = 8;
            double Tsym      = pow(2.0, frame->getLoRaSF()) / frame->getLoRaBW().get();
            double Tpreamble = (nPreamble + 4.25) * Tsym;
            double Theader   = 0.5 * (8 + payloadSymbNb) * Tsym;
            double Tpayload  = 0.5 * (8 + payloadSymbNb) * Tsym;
            double ToA       = Tpreamble + Theader + Tpayload;
            double dcWaitS   = ToA * 99.0;

            // Duty cycle uniquement sur la premiere transmission (pas les retransmissions)
            if (retransmissionCount == 0) {
                waitingForDC = true;
                if (dutyCycleTimer->isScheduled())
                    cancelEvent(dutyCycleTimer);
                scheduleAt(simTime() + dcWaitS, dutyCycleTimer);
                EV << "CLASS S DutyCycle 1%: blocking TX for " << dcWaitS
                   << "s (ToA=" << ToA << "s)" << endl;
            } else {
                EV << "CLASS S: retransmission " << retransmissionCount
                   << ", skipping duty cycle" << endl;
            }

            if (!usingAck) {
                scheduleAt(simTime(), endDelay_1);
                scheduleAt(simTime(), endListening_1);
                scheduleAt(simTime(), endDelay_2);
                scheduleAt(simTime(), endListening_2);
            } else {
                bdw = simTime() + waitDelay1Time;
                scheduleAt(simTime() + waitDelay1Time, endDelay_1);
                scheduleAt(simTime() + waitDelay1Time + listening1Time, endListening_1);
                scheduleAt(simTime() + waitDelay1Time + listening1Time + waitDelay2Time, endDelay_2);
                scheduleAt(simTime() + waitDelay1Time + listening1Time
                           + waitDelay2Time + listening2Time, endListening_2);
            }
            return;
        }
    if (useCSMA) {
            // Rajouter le canal TX une fois la transmission terminee
            double usedCF = lastUplinkCF.get();
            if (std::find(availableChannels.begin(),
                          availableChannels.end(), usedCF)
                          == availableChannels.end()) {
                availableChannels.push_back(usedCF);
                numAvailableChannels = availableChannels.size();
                EV << "CSMA: Canal " << usedCF/1e6
                   << " MHz libere et rajoute" << endl;
            }
        }
    // ---- Duty Cycle 1% ----
    const auto &frame = currentTxFrame->peekAtFront<LoRaMacFrame>();
    int nPreamble = 8;
    int payloadSymbNb = 8;
    payloadSymbNb += (int)std::ceil(
        (8.0*payloadBytes - 4.0*frame->getLoRaSF() + 28.0 + 16.0)
        / (4.0*(frame->getLoRaSF() - 2*0))
    ) * (frame->getLoRaCR() + 4);
    if (payloadSymbNb < 8) payloadSymbNb = 8;
    double Tsym     = pow(2.0, frame->getLoRaSF()) / frame->getLoRaBW().get();
    double Tpreamble = (nPreamble + 4.25) * Tsym;
    double Theader  = 0.5 * (8 + payloadSymbNb) * Tsym;
    double Tpayload = 0.5 * (8 + payloadSymbNb) * Tsym;
    double ToA      = Tpreamble + Theader + Tpayload;
    double dcWait   = ToA * 99.0;
    waitingForDC = true;
    if (dutyCycleTimer->isScheduled())
        cancelEvent(dutyCycleTimer);
    scheduleAt(simTime() + dcWait, dutyCycleTimer);
    EV << "DutyCycle 1%: blocking TX for " << dcWait
       << "s (ToA=" << ToA << "s)" << endl;
    // ---- Fin Duty Cycle ----

    if (!usingAck) {

            scheduleAt(simTime(), endDelay_1);
            scheduleAt(simTime(), endListening_1);
            scheduleAt(simTime(), endDelay_2);
            scheduleAt(simTime(), endListening_2);
            return;
        }

    bdw = simTime() + waitDelay1Time;
    scheduleAt(simTime() + waitDelay1Time, endDelay_1);
    scheduleAt(simTime() + waitDelay1Time + listening1Time, endListening_1);
    scheduleAt(simTime() + waitDelay1Time + listening1Time + waitDelay2Time, endDelay_2);
    scheduleAt(simTime() + waitDelay1Time + listening1Time
               + waitDelay2Time + listening2Time, endListening_2);
}

Packet *LoRaMac::getCurrentTransmission()
{
    ASSERT(currentTxFrame != nullptr);
    return currentTxFrame;
}
/*Packet *LoRaMac::getCurrentReception()
{
    ASSERT(currentRxFrame != nullptr);
    return currentRxFrame;
}*/

bool LoRaMac::isReceiving()
{
    return radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING;
}

bool LoRaMac::isAck(const Ptr<const LoRaMacFrame> &frame)
{
    return false;//dynamic_cast<LoRaMacFrame *>(frame);
}

bool LoRaMac::isBeacon(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getPktType() == BEACON;
}

bool LoRaMac::isDownlink(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getPktType() == DOWNLINK;
}

bool LoRaMac::isBroadcast(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getReceiverAddress().isBroadcast();
}

bool LoRaMac::isForUs(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getReceiverAddress() == address;
}

bool LoRaMac::timeToTrasmit()
{
    if (waitingForDC) return false;
    if (FSAGame && dblrand() > a) return false;
    if (beaconGuard) return false;

    // Retransmission: currentTxFrame must exist
    if (retransmissionPending) {
        if (currentTxFrame != nullptr) {
            retransmissionPending = false;
            return true;
        } else {
            // Frame was lost; abandon retransmission
            retransmissionPending = false;
            retransmissionCount = 0;
            return false;
        }
    }

    if (!txQueue->isEmpty()) {
            if (currentTxFrame != nullptr) {
                // Une retransmission est encore en attente, ne pas ecraser
                EV << "CLASS S: fresh packet queued but retransmission still pending" << endl;
                return false;
            }
            popTxQueue();
            retransmissionCount = 0;

            return true;
        }

    return false;
}

void LoRaMac::turnOnReceiver()
{
    LoRaRadio *loraRadio;
    loraRadio = check_and_cast<LoRaRadio *>(radio);
    loraRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
}

void LoRaMac::turnOffReceiver()
{
    LoRaRadio *loraRadio;
    loraRadio = check_and_cast<LoRaRadio *>(radio);
    loraRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
}

MacAddress LoRaMac::getAddress()
{
    return address;
}

} // namespace inet
