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

#include "LoRaTransmitter.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarTransmission.h"
#include "LoRaModulation.h"
#include "LoRaPhyPreamble_m.h"
#include <algorithm>
#include "mobility/SatelliteMobility.h"
#include "mobility/GroundStationMobility.h"
#include "mobility/UniformGroundMobility.h"
#include "LoRaApp/LoRaAppPacket_m.h"
namespace flora {

Define_Module(LoRaTransmitter);

LoRaTransmitter::LoRaTransmitter() :
    FlatTransmitterBase()
{
}

void LoRaTransmitter::initialize(int stage)
{
    TransmitterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        preambleDuration = 0.001; //par("preambleDuration");
        headerLength = b(par("headerLength"));
        bitrate = bps(par("bitrate"));
        power = W(par("power"));
        centerFrequency = Hz(par("centerFrequency"));
        bandwidth = Hz(par("bandwidth"));
        LoRaTransmissionCreated = registerSignal("LoRaTransmissionCreated");
        iAmGateway = false;
        if(strcmp(getParentModule()->getClassName(), "flora::LoRaGWRadio") == 0)
            iAmGateway = true;
        payloadBytes = par("payloadSize");
        // Enregistrement des vecteurs
                toaVector.setName("ToA");
                toaVector.setUnit("s");
                payloadSizeVector.setName("payloadSize");
                payloadSizeVector.setUnit("B");
    }
}

std::ostream& LoRaTransmitter::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaTransmitter";
    return FlatTransmitterBase::printToStream(stream, level, evFlags);
}

const ITransmission *LoRaTransmitter::createTransmission(const IRadio *transmitter, const Packet *macFrame, const simtime_t startTime) const
{
    // TransmissionBase *controlInfo = dynamic_cast<TransmissionBase *>(macFrame->getControlInfo());
    // W transmissionPower = controlInfo && !std::isnan(controlInfo->getPower().get()) ? controlInfo->getPower() : power;
    const_cast<LoRaTransmitter* >(this)->emit(LoRaTransmissionCreated, true);
    // const LoRaMacFrame *frame = check_and_cast<const LoRaMacFrame *>(macFrame);
    EV << macFrame->getDetailStringRepresentation(evFlags) << endl;
    const auto &frame = macFrame->peekAtFront<LoRaPhyPreamble>();

    int nPreamble = 8;
    int effectivePayloadBytes = iAmGateway ? 15 : payloadBytes;

    int payloadSymbNb = 8;
    payloadSymbNb += std::ceil((8*effectivePayloadBytes - 4*frame->getSpreadFactor() + 28 + 16 - 20*0) / (4*(frame->getSpreadFactor()-2*0))) * (frame->getCodeRendundance() + 4);
    if(payloadSymbNb < 8) payloadSymbNb = 8;

    simtime_t Tsym = pow(2, frame->getSpreadFactor()) / frame->getBandwidth().get();
    simtime_t Tpreamble = (nPreamble + 4.25) * Tsym;
    simtime_t Theader = 0.5 * (8+payloadSymbNb) * Tsym;
    simtime_t Tpayload = 0.5 * (8+payloadSymbNb) * Tsym;

    const simtime_t duration = Tpreamble + Theader + Tpayload;
    const simtime_t endTime = startTime + duration;
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const Quaternion startOrientation = mobility->getCurrentAngularPosition();
    const Quaternion endOrientation = mobility->getCurrentAngularPosition();
    W transmissionPower = computeTransmissionPower(macFrame);

    auto longLatStartPosition = cCoordGeo();
    auto longLatEndPosition = cCoordGeo();

    // transmitter is a satellite
    if (SatelliteMobility *sgp4Mobility = dynamic_cast<SatelliteMobility *>(mobility))
    {
        longLatStartPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
        longLatEndPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
    }
    // transmitter is a node with random uniform disk mobility initialization
    else if (UniformGroundMobility *diskMobility = dynamic_cast<UniformGroundMobility *>(mobility))
    {
        longLatStartPosition = cCoordGeo(diskMobility->getLatitude(), diskMobility->getLongitude(), 0);
        longLatEndPosition = cCoordGeo(diskMobility->getLatitude(), diskMobility->getLongitude(), 0);
    }
    // transmitter is a node or a ground station with a defined position
    else if (GroundStationMobility *lutMobility = dynamic_cast<GroundStationMobility *>(mobility))
    {
        longLatStartPosition = cCoordGeo(lutMobility->getLatitude(), lutMobility->getLongitude(), 0);
        longLatEndPosition = cCoordGeo(lutMobility->getLatitude(), lutMobility->getLongitude(), 0);
    }
    // other, should never reach this point
    else
    {
        longLatStartPosition = cCoordGeo(0, 0, 0);
        longLatEndPosition = cCoordGeo(0, 0, 0);
    }


    if(!iAmGateway)
    {
        LoRaRadio *radio = check_and_cast<LoRaRadio *>(getParentModule());
        radio->setCurrentTxPower(transmissionPower.get());
    }
    EV << "=== ToA Debug ===" << endl;
    EV << "SF=" << frame->getSpreadFactor() << endl;
    EV << "BW=" << frame->getBandwidth().get() << endl;
    EV << "CR=" << frame->getCodeRendundance() << endl;
    EV << "payloadBytes=" << payloadBytes << endl;
    EV << "payloadSymbNb=" << payloadSymbNb << endl;
    EV << "Tsym=" << Tsym << endl;
    EV << "Tpreamble=" << Tpreamble << endl;
    EV << "Tpayload=" << Tpayload << endl;
    EV << "ToA=" << duration << endl;
    EV << "numerateur=" << (8*payloadBytes - 4*frame->getSpreadFactor() + 28 + 16) << endl;
    EV << "denominateur=" << (4*(frame->getSpreadFactor() - 2*0)) << endl;
    EV << "ceil result=" << (int)ceil((8*payloadBytes - 4*frame->getSpreadFactor() + 28 + 16.0) / (4*(frame->getSpreadFactor() - 2*0))) << endl;
    EV << "CR+4=" << (frame->getCodeRendundance() + 4) << endl;
    const_cast<LoRaTransmitter*>(this)->toaVector.record(duration.dbl());
    if(!iAmGateway) {
        b phyHeaderLen = macFrame->peekAtFront<LoRaPhyPreamble>()->getChunkLength();
        const auto &macChunk = macFrame->peekAt<LoRaMacFrame>(phyHeaderLen);
        b macHeaderLen = macChunk->getChunkLength();
        int actualPayloadBytes = B(macFrame->getTotalLength() - phyHeaderLen - macHeaderLen).get();
        const_cast<LoRaTransmitter*>(this)->payloadSizeVector.record(actualPayloadBytes);
    }
    return new LoRaTransmission(transmitter,
            macFrame,
            startTime,
            endTime,
            Tpreamble,
            Theader,
            Tpayload,
            startPosition,
            endPosition,
            startOrientation,
            endOrientation,
            transmissionPower,
            frame->getCenterFrequency(),
            frame->getSpreadFactor(),
            frame->getBandwidth(),
            frame->getCodeRendundance(),
            longLatStartPosition,
            longLatEndPosition);

}

}
