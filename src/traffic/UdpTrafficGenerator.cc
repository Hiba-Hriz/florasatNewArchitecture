//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 OpenSim Ltd.
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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "UdpTrafficGenerator.h"

namespace flora {
namespace traffic {

Define_Module(UdpTrafficGenerator);

UdpTrafficGenerator::~UdpTrafficGenerator() {
    cancelAndDelete(selfMsg);
}

void UdpTrafficGenerator::initialize(int stage) {
    ClockUserModuleMixin::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);

        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");

        if (stopTime >= CLOCKTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new ClockEvent("sendTimer");
    }
}

void UdpTrafficGenerator::finish() {
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
}

// void UdpTrafficGenerator::setSocketOptions() {
//     socket.setCallback(this);
// }

L3Address UdpTrafficGenerator::chooseDestAddr() {
    int k = intrand(destAddresses.size());
    return destAddresses[k];
}

void UdpTrafficGenerator::sendPacket() {
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    const auto &payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(par("messageLength")));
    payload->setSequenceNumber(numSent);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);

    const auto transpHeader = makeShared<TransportHeader>();
    L3Address destAddr = chooseDestAddr();
    transpHeader->setChunkLength(B(1));
    transpHeader->setDstIpAddress(destAddr.str().c_str());
    transpHeader->setSrcIpAddress(appLocalAddress.str().c_str());
    packet->insertAtFront(transpHeader);

    emit(packetSentSignal, packet);
    // socket.sendTo(packet, destAddr, 5000);
    send(packet, "socketOut");
    numSent++;
}

void UdpTrafficGenerator::processStart() {
    // socket.setOutputGate(gate("socketOut"));
    const char *localAddress = getParentModule()->par("localAddress");
    appLocalAddress = L3Address(localAddress);
    // socket.bind(L3Address(localAddress), 5000);
    // setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != nullptr) {
        destAddressStr.push_back(token);
        L3Address result = L3Address(token);
        destAddresses.push_back(result);
    }

    if (!destAddresses.empty()) {
        selfMsg->setKind(SEND);
        processSend();
    } else {
        if (stopTime >= CLOCKTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleClockEventAt(stopTime, selfMsg);
        }
    }
}

void UdpTrafficGenerator::processSend() {
    sendPacket();
    clocktime_t d = par("sendInterval");
    if (stopTime < CLOCKTIME_ZERO || getClockTime() + d < stopTime) {
        selfMsg->setKind(SEND);
        scheduleClockEventAfter(d, selfMsg);
    } else {
        selfMsg->setKind(STOP);
        scheduleClockEventAt(stopTime, selfMsg);
    }
}

void UdpTrafficGenerator::processStop() {
    // socket.close();
}

void UdpTrafficGenerator::handleMessageWhenUp(cMessage *msg) {
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
#ifndef NDEBUG
                EV << "START" << endl;
#endif
                processStart();
                break;

            case SEND:
#ifndef NDEBUG
                EV << "SEND" << endl;
#endif
                processSend();
                break;

            case STOP:
#ifndef NDEBUG
                EV << "STOP" << endl;
#endif
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    } else {
        // socket.processMessage(msg);
        auto pkt = check_and_cast<inet::Packet *>(msg);
        processPacket(pkt);
    }
}

// void UdpTrafficGenerator::socketDataArrived(UdpSocket *socket, Packet *packet) {
//     // process incoming packet
//     processPacket(packet);
// }

// void UdpTrafficGenerator::socketErrorArrived(UdpSocket *socket, Indication *indication) {
//     EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
//     delete indication;
// }

// void UdpTrafficGenerator::socketClosed(UdpSocket *socket) {
//     if (operationalState == State::STOPPING_OPERATION)
//         startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
// }

void UdpTrafficGenerator::refreshDisplay() const {
    ApplicationBase::refreshDisplay();

    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void UdpTrafficGenerator::processPacket(Packet *pk) {
    emit(packetReceivedSignal, pk);
    EV_INFO << "Received packet: " << pk << endl;
    delete pk;
    numReceived++;
}

void UdpTrafficGenerator::handleStartOperation(LifecycleOperation *operation) {
    clocktime_t start = std::max(startTime, getClockTime());
    if ((stopTime < CLOCKTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleClockEventAt(start, selfMsg);
    }
}

void UdpTrafficGenerator::handleStopOperation(LifecycleOperation *operation) {
    cancelEvent(selfMsg);
    // socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void UdpTrafficGenerator::handleCrashOperation(LifecycleOperation *operation) {
    cancelClockEvent(selfMsg);
    // socket.destroy();  // TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
}

}  // namespace traffic
}  // namespace flora
