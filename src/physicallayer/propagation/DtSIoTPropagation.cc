/*
 * DtSIoTPropagation.cc
 *
 *  Created on: May 15, 2022
 *      Author: diego
 */

#include "DtSIoTPropagation.h"

namespace flora {

Define_Module(DtSIoTPropagation);

/**
 * Contructor for the SatellitePropagation model. It must also call the constructor for the
 * PropagationBase to be compatible with the radio medium models of INET.
 */
DtSIoTPropagation::DtSIoTPropagation() : PropagationBase(),
                                         ignoreMovementDuringTransmission(false),
                                         ignoreMovementDuringPropagation(false),
                                         ignoreMovementDuringReception(false) {
}

/**
 *Intialise method of SatellitePropagation model.This method simply initialises specific parameters
 *that enable further customisation of the model. The same implementation is used within the
 *INET ConstantSpeedPropagation, which this model is based off.
 */
void DtSIoTPropagation::initialize(int stage) {
    PropagationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        ignoreMovementDuringTransmission = par("ignoreMovementDuringTransmission");
        ignoreMovementDuringPropagation = par("ignoreMovementDuringPropagation");
        ignoreMovementDuringReception = par("ignoreMovementDuringReception");
        // TODO:
        if (!ignoreMovementDuringTransmission)
            throw cRuntimeError("ignoreMovementDuringTransmission is yet not implemented");
    }
}

/**
 * This method has no functionality but is only used due to its being a part of the INET ConstantSpeedPropagation model.
 * INET has not implemented this method yet, and as a result, it does not affect the model. The method was implemnted so
 * that when INET does adapt the propagation models of INET, the SatellitePropagation model should be robust enough to
 * easily adapt.
 */
const Coord DtSIoTPropagation::computeArrivalPosition(const simtime_t time, const Coord position, IMobility *mobility) const {
    // TODO: return mobility->getPosition(time);
    throw cRuntimeError("Movement approximation is not implemented");
}

std::ostream &DtSIoTPropagation::printToStream(std::ostream &stream, int level, int evFlags) const {
    stream << "SatellitePropagation";
    if (level <= PRINT_LEVEL_TRACE)
        stream << ", ignoreMovementDuringTransmission = " << ignoreMovementDuringTransmission
               << ", ignoreMovementDuringPropagation = " << ignoreMovementDuringPropagation
               << ", ignoreMovementDuringReception = " << ignoreMovementDuringReception;
    return PropagationBase::printToStream(stream, level);
}

/**
 * computeArrival is the primary method of the SatellitePropagationModel. Whenever a transmission has been made in INET,
 * the propagation model is used to compute the arrival, which returns an Arrival object with the start and end propagation
 * times. The ConstantSpeedPropagation model returns a propagation delay equal to the distance between two nodes on the OMNeT++
 * 2D canvas. This method adapts this by firstly checking if the transmission model is the custom SatelliteUnitDiskTransmission.
 * This transmission model encapsulates the coordinates so that the corresponding receiver nodes getDistance method can use the
 * coordinates and get the actual distance.
 */
const IArrival *DtSIoTPropagation::computeArrival(const ITransmission *transmission, IMobility *mobility) const {
    arrivalComputationCount++;
    const simtime_t startTime = transmission->getStartTime();
    const simtime_t endTime = transmission->getEndTime();

    const Coord startPosition = transmission->getStartPosition();  // antenna position when the transmitter has started the transmission
    const Coord endPosition = transmission->getEndPosition();

    double distance = 0;  // m

    // for ISL transmissions
    if (const SatelliteUnitDiskTransmission *satUnitTransmission = dynamic_cast<const SatelliteUnitDiskTransmission *>(transmission)) {
        if (const SatelliteMobility *receiverSatMobility = dynamic_cast<const SatelliteMobility *>(mobility))
            distance = receiverSatMobility->getDistance(satUnitTransmission->getStartLongLatPosition().m_Lat,
                                                        satUnitTransmission->getStartLongLatPosition().m_Lon, satUnitTransmission->getStartLongLatPosition().m_Alt) *
                       1000;  // OS3 uses KM for all measurements
    }

    // for LoRa transmissions
    else if (const LoRaTransmission *loraTransmission = dynamic_cast<const LoRaTransmission *>(transmission)) {
        if (const SatelliteMobility *receiverSatMobility = dynamic_cast<const SatelliteMobility *>(mobility)) {
            distance = receiverSatMobility->getDistance(loraTransmission->getStartLongLatPosition().m_Lat,
                                                        loraTransmission->getStartLongLatPosition().m_Lon, loraTransmission->getStartLongLatPosition().m_Alt) *
                       1000;  // OS3 uses KM for all measurements
            EV << "\nDISTANCE FROM NODE IN POSITION (" << loraTransmission->getStartLongLatPosition().m_Lat << ", " << loraTransmission->getStartLongLatPosition().m_Lon
               << ") TO SATELLITE IN POSITION (" << receiverSatMobility->getLatitude() << ", " << receiverSatMobility->getLongitude()
               << ") IS: " << distance / 1000 << " km" << endl;
        } else if (const UniformGroundMobility *receiverGroundMobility = dynamic_cast<const UniformGroundMobility *>(mobility)) {
            distance = receiverGroundMobility->getDistance(loraTransmission->getStartLongLatPosition().m_Lat,
                                                           loraTransmission->getStartLongLatPosition().m_Lon, loraTransmission->getStartLongLatPosition().m_Alt);
            // EV << "HERE" << endl;
        } else if (const GroundStationMobility *receiverGroundMobility = dynamic_cast<const GroundStationMobility *>(mobility))
            distance = receiverGroundMobility->getDistance(loraTransmission->getStartLongLatPosition().m_Lat,
                                                           loraTransmission->getStartLongLatPosition().m_Lon, loraTransmission->getStartLongLatPosition().m_Alt);
    } else
        EV_ERROR << "OTHER TRANSMITTER DETECTED";

    const Coord startArrivalPosition = ignoreMovementDuringPropagation ? mobility->getCurrentPosition() : computeArrivalPosition(startTime, startPosition, mobility);
    const simtime_t startPropagationTime = distance / propagationSpeed.get();
    const simtime_t startArrivalTime = startTime + startPropagationTime;
    const Quaternion startArrivalOrientation = mobility->getCurrentAngularPosition();

    if (ignoreMovementDuringReception) {
        const Coord endArrivalPosition = startArrivalPosition;
        const simtime_t endPropagationTime = startPropagationTime;
        const simtime_t endArrivalTime = endTime + startPropagationTime;
        const simtime_t preambleDuration = transmission->getPreambleDuration();
        const simtime_t headerDuration = transmission->getHeaderDuration();
        const simtime_t dataDuration = transmission->getDataDuration();
        const Quaternion endArrivalOrientation = mobility->getCurrentAngularPosition();
        return new Arrival(startPropagationTime, endPropagationTime, startArrivalTime, endArrivalTime, preambleDuration,
                           headerDuration, dataDuration, startArrivalPosition, endArrivalPosition, startArrivalOrientation, endArrivalOrientation);
    }

    else {
        const Coord endArrivalPosition = computeArrivalPosition(endTime, endPosition, mobility);
        const simtime_t endPropagationTime = endPosition.distance(endArrivalPosition) / propagationSpeed.get();
        const simtime_t endArrivalTime = endTime + endPropagationTime;
        const simtime_t preambleDuration = transmission->getPreambleDuration();
        const simtime_t headerDuration = transmission->getHeaderDuration();
        const simtime_t dataDuration = transmission->getDataDuration();
        const Quaternion endArrivalOrientation = mobility->getCurrentAngularPosition();
        return new Arrival(startPropagationTime, endPropagationTime, startArrivalTime, endArrivalTime, preambleDuration,
                           headerDuration, dataDuration, startArrivalPosition, endArrivalPosition, startArrivalOrientation, endArrivalOrientation);
    }
}

}  // namespace flora
