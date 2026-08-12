/*
 * GroundForwarder.cc
 *
 *  Created on: Apr 22, 2022
 *      Author: diego
 */



#include "GroundForwarder.h"

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "../LoRaPhy/LoRaRadioControlInfo_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"



namespace flora {

Define_Module(GroundForwarder);

void GroundForwarder::initialize(int stage)
{
    if (stage == 0)
    {
        localPort = par("localPort");
        destPort = par("destPort");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
        startUDP();
}

void GroundForwarder::startUDP()
{
    if (!gate("socketOut")->isConnected())
        {
            EV << "GroundForwarder: socketOut not connected, skipping UDP init\n";
            return;
        }
    const char *token;
    const char *localAddress = par("localAddress");
    const char *destAddrs = par("destAddresses");

    socket.setOutputGate(gate("socketOut"));
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);

    // Create UDP sockets to multiple destination addresses (network servers)
    cStringTokenizer tokenizer(destAddrs);
    while ((token = tokenizer.nextToken()) != nullptr) {
        L3Address result;
        L3AddressResolver().tryResolve(token, result);

        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        else
            EV << "Got destination address: " << token << endl;

        destAddresses.push_back(result);
    }
}

void GroundForwarder::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet*>(msg);
    if (msg->arrivedOn("satLink$i")){
        EV << "ACHF - Initializing stage 0\n";
        processLoraMACPacket(pkt);
    }else if (msg->arrivedOn("socketIn"))
        send(pkt, "satLink$o");
}

void GroundForwarder::processLoraMACPacket(Packet *pk)
{
    if (destAddresses.empty() || !gate("socketOut")->isConnected())
        {
            EV << "GroundForwarder: no destination or socket not connected, dropping packet\n";
            delete pk;
            return;
        }
    auto frame = pk->removeAtFront<LoRaMacFrame>();
    frame->setGroundTime(simTime());
    pk->insertAtFront(frame);

    // FIXME : Identify network server message is destined for.
    L3Address destAddr = destAddresses[0];
    if (pk->getControlInfo())
        delete pk->removeControlInfo();

    socket.sendTo(pk, destAddr, destPort);
}

}
