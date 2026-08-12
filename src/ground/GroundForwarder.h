/*
 * GroundForwarder.h
 *
 *  Created on: Apr 22, 2022
 *      Author: diego
 */

#ifndef GROUND_GROUNDFORWARDER_H_
#define GROUND_GROUNDFORWARDER_H_

#include <omnetpp.h>
#include <vector>
#include "inet/common/INETDefs.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/RadioControlInfo_m.h"
#include "LoRa/LoRaMacControlInfo_m.h"
#include "LoRa/LoRaMacFrame_m.h"

namespace flora {

class GroundForwarder : public cSimpleModule, public cListener
{
protected:
  std::vector<L3Address> destAddresses;
  int localPort = -1;
  int destPort = -1;
  UdpSocket socket;
  cMessage *selfMsg = nullptr;

  virtual void initialize(int stage) override;
  virtual int numInitStages() const override { return NUM_INIT_STAGES; }
  virtual void handleMessage(cMessage *msg) override;

  void startUDP();
  void processLoraMACPacket(Packet *pk);

};


}

#endif /* GROUND_GROUNDFORWARDER_H_ */
