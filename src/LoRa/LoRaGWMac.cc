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

#include "LoRaGWMac.h"

#include <cmath>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "LoRaGWRadio.h"
#include "LoRaPhy/LoRaPhyPreamble_m.h"
#include "LoRaApp/SimpleLoRaApp.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "mobility/UniformGroundMobility.h"
#include "inet/common/geometry/common/Coord.h"
#include <cmath>
#include "../mobility/SatelliteMobility.h"
using namespace std;

namespace flora {

Define_Module(LoRaGWMac);

void LoRaGWMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        outGate = findGate("upperMgmtOut");

        // subscribe for the information of the carrier sense
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);

        // subscribe at LoRaGWNIC module level, which contains this module and radio
        // module, the latter is where the signal origins within receiver submodule
        getParentModule()->subscribe("belowSensitivityReception", this);


        // subscribe at system module level, all signal received, i.e. all gateways
        // (precisely, all LoRaGWRadio modules, source of the signal)
        getSimulation()->getSystemModule()->subscribe("LoRaGWRadioReceptionStarted", this);
        getSimulation()->getSystemModule()->subscribe("LoRaGWRadioReceptionFinishedCorrect", this);


        radio = check_and_cast<IRadio *>(radioModule);


        beaconPeriod = new cMessage("Beacon Timer");
        endTXslot = new cMessage("UplinkSlot_End");
        beaconGuardStart = new cMessage("Beacon_Guard_Start");
        beaconReservedEnd = new cMessage("Beacon_Reserved_End");

        // beacon parameters
        beaconStart = par("beaconStart");
        beaconPeriodTime = par("beaconPeriodTime");
        beaconReservedTime = par("beaconReservedTime");
        beaconGuardTime = par("beaconGuardTime");

        pingNumber = par("pingNumber");
        satIndex = par("satIndex");

        // class S parameters
        maxToA = par("maxToA");
        clockThreshold = par("clockThreshold");
        classSslotTime = 2*clockThreshold + maxToA;
        //maxClassSslots = floor((beaconPeriodTime - beaconGuardTime - beaconReservedTime) / classSslotTime);

        // lora parameters for beacon
        beaconSF = par("beaconSF");
        beaconTP = par("beaconTP");
        beaconCF = par("beaconCF");
        beaconBW = par("beaconBW");
        beaconCR = par("beaconCR");

        classSslotStatus.setName("ClassS_Slot_Status");
        classSslotBeacon.setName("ClassS_Slot_Beacon_Number");
        classSslotReceptionAttempts.setName("ClassS_Reception_Attempts_Per_Slot");
        classSslotReceptionSuccess.setName("ClassS_Successful_Receptions_Per_Slot");
        classSslotReceptionBelowSensitivity.setName("ClassS_BelowSensitivity_Receptions_Per_Slot");

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

        const char *addressString = par("address");
        GW_forwardedDown = 0;
        GW_droppedDC = 0;
        if (!strcmp(addressString, "auto"))
        {
            // assign automatic address
            address = MacAddress::generateAutoAddress();
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(address.str().c_str());
        }
        else
            address.setAddress(addressString);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(IRadio::RADIO_MODE_TRANSCEIVER);
    }
}

void LoRaGWMac::finish()
{
    recordScalar("GW_forwardedDown", GW_forwardedDown);
    recordScalar("GW_droppedDC", GW_droppedDC);
    //cancelAndDelete(dutyCycleTimer);
    for (auto& pair : dutyCycleTimers) {
        cancelAndDelete(pair.second);
    }
    dutyCycleTimers.clear();
    cancelAndDelete(beaconPeriod);
    cancelAndDelete(endTXslot);
    cancelAndDelete(beaconGuardStart);
    cancelAndDelete(beaconReservedEnd);
    recordScalar("BSV", beaconsSkippedVisibility);
}


void LoRaGWMac::configureNetworkInterface()
{

    MacAddress address = parseMacAddressParameter(par("address"));

    // generate a link-layer address to be used as interface token for IPv6
    networkInterface->setMacAddress(address);

    // capabilities
    networkInterface->setMtu(par("mtu"));
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
    networkInterface->setPointToPoint(false);
}

void LoRaGWMac::handleSelfMessage(cMessage *msg)
{
    if (msg == beaconPeriod)
    {
        beaconNumber++;
        beaconGuard = false;
        beaconScheduling();
        sendBeacon();
    }

    if (msg == beaconGuardStart)
    {
        beaconGuard = true;
        if (isClassS)
            cancelEvent(endTXslot);
    }

    if (msg == beaconReservedEnd && isClassS)
        scheduleAt(simTime() + classSslotTime, endTXslot);

    if (msg == endTXslot)
    {
        int detectableReceptions = attemptedReceptionsPerSlot - belowSensitivityReceptions;

        if (successfulReceptionsPerSlot == 0)    // no successful reception during slot time
        {
            if (attemptedReceptionsPerSlot == 0) // no reception attempts, slot is IDLE
                classSslotStatus.record(0);
            else if (detectableReceptions == 0)  // all reception attempts with low power, slot is IDLE
                classSslotStatus.record(1);
            else if (detectableReceptions >= 1)  // at least two receptions collided, slot is COLLIDED
                classSslotStatus.record(4);
            else                                 // single reception over sensitivity but collided with another, slot is COLLIDED
                std::cout << "SUCCESSFUL IS 0, BUT THIS SHOULD BE AN IMPOSSIBLE CASE" << endl;
        }

        else if (successfulReceptionsPerSlot == 1) // one successful reception during slot time
        {
            if (detectableReceptions == 1)         // single reception over sensitivity, slot is SUCCESSFUL
                classSslotStatus.record(2);
            else if (detectableReceptions >= 2)    // at least two receptions collided, slot is COLLIDED
                classSslotStatus.record(3);
            else                                   // impossible
                //std::cout << "SUCCESSFUL IS 1, BUT THIS SHOULD BE AN IMPOSSIBLE CASE" << endl;
                EV << "Multi-channel: " << successfulReceptionsPerSlot << " simultaneous receptions" << endl;
        }

        else // two or more successful reception during slot time
            //std::cout << "SUCCESSFUL IS " << successfulReceptionsPerSlot << " , BUT THIS SHOULD BE AN IMPOSSIBLE CASE" << endl;
            EV << "Multi-channel: " << successfulReceptionsPerSlot << " simultaneous receptions" << endl;
        classSslotReceptionAttempts.record(attemptedReceptionsPerSlot);
        classSslotReceptionSuccess.record(successfulReceptionsPerSlot);
        classSslotReceptionBelowSensitivity.record(belowSensitivityReceptions);
        classSslotBeacon.record(beaconNumber);
        successfulReceptionsPerSlot = 0;
        attemptedReceptionsPerSlot = 0;
        belowSensitivityReceptions = 0;

        scheduleAt(simTime() + classSslotTime, endTXslot);
    }

    //Quand un timer que TU as planifie avec scheduleAt(simTime() + delta, dutyCycleTimers[freqHz]) arrive a echeance.
    for (auto& pair : dutyCycleTimers) {
        if (msg == pair.second) {
            // msg est le timer qui vient d'expirer
            // pair.second est le timer stocke pour cette frequence
            // s'ils pointent vers le meme objet => c'est cette frequence qui a expire
            waitingForDC[pair.first] = false;
            break;
        }
    }
}

void LoRaGWMac::handleUpperMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

    // Get frequency of this specific downlink packet
    double freqHz = frame->getLoRaCF().get();

    //Verification du duty cycle actif
    //waitingForDC est une map<double, bool> : une entree par frequence :  cle = frequence (freqHz), valeur = timer (cMessage*)
    //Si true => la frequence est encore en cooldown, le paquet est jete.
    //Si false ou absent => on continue.
    if (waitingForDC.count(freqHz) && waitingForDC[freqHz]) //checking both existence and value Since waitingForDC is a std::map<double, bool>
    {
        EV << "LoRaGWMac: duty cycle active for " << freqHz/1e6
           << " MHz, dropping downlink to " << frame->getReceiverAddress() << endl;
        GW_droppedDC++;
        delete msg;
        return;
    }

    // Proceed with transmission
    if (pkt->getControlInfo())
        delete pkt->removeControlInfo();

    auto tag = pkt->addTagIfAbsent<MacAddressReq>();
    tag->setDestAddress(frame->getReceiverAddress());

    // Compute duty cycle duration for this SF
    double delta;
    if(frame->getLoRaSF() == 7)  delta = 0.61696;
    if(frame->getLoRaSF() == 8)  delta = 1.23392;
    if(frame->getLoRaSF() == 9)  delta = 2.14016;
    if(frame->getLoRaSF() == 10) delta = 4.28032;
    if(frame->getLoRaSF() == 11) delta = 7.24992;
    if(frame->getLoRaSF() == 12) delta = 14.49984;

    //creation/reutilisation du timer
    waitingForDC[freqHz] = true;

    if (dutyCycleTimers.count(freqHz) == 0) {
        // Premiere fois : on cree un nouveau cMessage
        dutyCycleTimers[freqHz] = new cMessage("Duty Cycle Timer");
    }
    //Timer planifie a (maintenant + delta)
    scheduleAt(simTime() + delta, dutyCycleTimers[freqHz]); //Planifie ce message (cMessage) pour qu'il soit declenche a l'instant actuel + delta

    GW_forwardedDown++;
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
    sendDown(pkt); //Le paquet part vers la couche radio pendant que le timer tourne.
}

void LoRaGWMac::handleLowerMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    auto header = pkt->popAtFront<LoRaPhyPreamble>();
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

    if(frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
        sendUp(pkt);
    else
        delete pkt;
}

void LoRaGWMac::sendPacketBack(Packet *receivedFrame)
{
    const auto &frame = receivedFrame->peekAtFront<LoRaMacFrame>();
    EV << "sending Data frame back" << endl;
    auto pktBack = new Packet("LoraPacket");
    auto frameToSend = makeShared<LoRaMacFrame>();
    frameToSend->setChunkLength(B(par("headerLength").intValue()));

    frameToSend->setReceiverAddress(frame->getTransmitterAddress());
    pktBack->insertAtFront(frameToSend);
    sendDown(pktBack);
}

// schedule beacon signals
void LoRaGWMac::beaconScheduling()
{
    scheduleAt(simTime() + beaconPeriodTime, beaconPeriod);
    scheduleAt(simTime() + beaconReservedTime, beaconReservedEnd);
    scheduleAt(simTime() + beaconPeriodTime - beaconGuardTime, beaconGuardStart);
}

void LoRaGWMac::computeNetworkCentroid()
{
    cModule *network = getSimulation()->getSystemModule();
    double sumLat = 0.0, sumLon = 0.0;
    int count = 0;

    for (cModule::SubmoduleIterator it(network); !it.end(); ++it)
    {
        cModule *submod = *it;
        // Only consider modules that are actual LoRa nodes
        cModule *appModule = submod->getSubmodule("SimpleLoRaApp");
        if (appModule == nullptr)
            continue;

        cModule *mobMod = submod->getSubmodule("mobility");
        auto uniformMob = dynamic_cast<UniformGroundMobility *>(mobMod);
        if (uniformMob == nullptr)
            continue;

        sumLat += uniformMob->getLatitude();
        sumLon += uniformMob->getLongitude();
        count++;
    }

    if (count > 0)
    {
        centroidLat = sumLat / count;
        centroidLon = sumLon / count;
        EV << "GW: computed node centroid from " << count
           << " nodes -> (" << centroidLat << ", " << centroidLon << ")" << endl;
    }
    else
    {
        // Fallback if no nodes found (shouldn't normally happen)
        centroidLat = -16.0;
        centroidLon = -56.5;
        EV << "GW: WARNING - no LoRa nodes found for centroid, using fallback point" << endl;
    }

    centroidComputed = true;
}
//this function send beacon message when the class is set to B or S
void LoRaGWMac::sendBeacon()
{

        cModule *network = getSimulation()->getSystemModule();
        cModule *gwNode  = getParentModule()->getParentModule();


        cModule *gwMobility = gwNode->getSubmodule("mobility");
        auto satMob = dynamic_cast<SatelliteMobility *>(gwMobility);

        if (satMob != nullptr)
        {
            if (!centroidComputed)
                computeNetworkCentroid();

            double nodeAlt = 0.0;
            double elevation = satMob->getElevation(centroidLat, centroidLon, nodeAlt);
            double minElevation = par("minElevationAngle").doubleValue();

            if (elevation < minElevation)
            {
                beaconsSkippedVisibility++;
                EV << "GW: satellite below horizon over node centroid (elev=" << elevation
                   << "), skipping beacon" << endl;
                return;
            }
        }
    auto beacon = new Packet("Beacon");
    auto frame = makeShared<LoRaMacFrame>();
    frame->setPktType(BEACON);
    frame->setChunkLength(B(par("headerLength").intValue()));
    auto tag = beacon->addTagIfAbsent<MacAddressReq>();
    tag->setDestAddress(MacAddress::BROADCAST_ADDRESS);
    beacon->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);

    units::values::Hz loRaBW = inet::units::values::Hz(beaconBW);
    units::values::Hz loRaCF = inet::units::values::Hz(beaconCF);

    frame->setLoRaSF(beaconSF);
    frame->setLoRaTP(beaconTP);
    frame->setLoRaBW(loRaBW);
    frame->setLoRaCF(loRaCF);
    frame->setLoRaCR(beaconCR);

    frame->setBeaconTimer(beaconPeriodTime);
    frame->setPingNb(pingNumber);

    beacon->insertAtFront(frame);
    sendDown(beacon);
    if (hasGUI())
        getParentModule()->getParentModule()->bubble(beaconSentText);
}


void LoRaGWMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal)
    {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;

        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

        transmissionState = newRadioTransmissionState;
    }

    if (simTime() >= getSimulation()->getWarmupPeriod())
    {
        signalName = getSignalName(signalID);
        if (strcmp(signalName, "LoRaGWRadioReceptionStarted") == 0 && (int)value == satIndex)
            attemptedReceptionsPerSlot++;

        if (strcmp(signalName, "LoRaGWRadioReceptionFinishedCorrect") == 0 && (int)value == satIndex)
            successfulReceptionsPerSlot++;

        if (strcmp(signalName, "belowSensitivityReception") == 0 && (int)value == satIndex)
            belowSensitivityReceptions++;
    }
}

MacAddress LoRaGWMac::getAddress()
{
    return address;
}

}
