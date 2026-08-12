/*
 * DtSIoTPropagation.h
 *
 *  Created on: May 15, 2022
 *      Author: diego
 */

#ifndef LORAPHY_PROPAGATION_DTSIOTPROPAGATION_H_
#define LORAPHY_PROPAGATION_DTSIOTPROPAGATION_H_

#include <math.h>

#include "../unitdisk/SatelliteUnitDiskTransmission.h"
#include "LoRa/LoRaRadio.h"
#include "LoRaPhy/LoRaTransmission.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/PropagationBase.h"
#include "inet/physicallayer/wireless/common/signal/Arrival.h"
#include "libnorad/cEcef.h"
#include "libnorad/cEci.h"
#include "libnorad/ccoord.h"
#include "mobility/GroundStationMobility.h"
#include "mobility/SatelliteMobility.h"
#include "mobility/UniformGroundMobility.h"

namespace flora {

using namespace physicallayer;

/**Class: DtSIoTPropagation
 * Within this model the distance between two positions are calculated using the coordinates of the source and destination.
 * This distanced is used to calculate the propagation delay for nodes within a satellite constellation.
 * Written by Aiden Valentine
 * Modified by Diego Maldonado
 */
class DtSIoTPropagation : public PropagationBase {
   protected:
    bool ignoreMovementDuringTransmission;
    bool ignoreMovementDuringPropagation;
    bool ignoreMovementDuringReception;

   protected:
    virtual void initialize(int stage) override;
    virtual const Coord computeArrivalPosition(const simtime_t startTime, const Coord startPosition, IMobility *mobility) const;

   public:
    DtSIoTPropagation();

    virtual std::ostream &printToStream(std::ostream &stream, int level, int evFlags = 0) const override;
    virtual const IArrival *computeArrival(const ITransmission *transmission, IMobility *mobility) const override;
};

}  // namespace flora

#endif /* LORAPHY_PROPAGATION_DTSIOTPROPAGATION_H_ */
