/*
 * ISLAckingMac.h
 *
 *  Created on: Apr 19, 2022
 *      Author: diego
 */

#ifndef SATELLITE_ISLACKINGMAC_H_
#define SATELLITE_ISLACKINGMAC_H_

#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/queueing/contract/IPacketQueue.h"

#include "inet/linklayer/acking/AckingMac.h"

#include "LoRa/LoRaMacFrame_m.h"

namespace flora {

using namespace inet;
using namespace inet::physicallayer;

class NetworkInterface;

/**
 * Implements a simplified ideal MAC.
 *
 * See the NED file for details.
 */
class ISLAckingMac : public AckingMac
{
  protected:
    /** implements MacProtocolBase functions */
    //@{
    virtual void encapsulate(Packet *msg) override;
    virtual void decapsulate(Packet *frame) override;
    virtual void startTransmitting() override;

    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void handleSelfMessage(cMessage *message) override;
    //@}

  public:
    ISLAckingMac();
    virtual ~ISLAckingMac();
    void finish() override;

    cOutVector SNIRDataVector;
    cHistogram SNIRDataHist;
    cOutVector RSSIDataVector;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
};

} // namespace flora

#endif /* SATELLITE_ISLACKINGMAC_H_ */
