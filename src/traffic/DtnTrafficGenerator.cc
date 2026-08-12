/*
 * DtnDtnTrafficGenerator.cc
 *
 *  Created on: May 30, 2023
 *      Author: Sebastian Montoya
 */

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

#include "DtnTrafficGenerator.h"

namespace flora {
namespace traffic {

Define_Module(DtnTrafficGenerator);

void DtnTrafficGenerator::initialize(int stage) {
    enable = par("enable");
    if (stage == INITSTAGE_LOCAL && enable) {
        this->eid_ = this->getParentModule()->getIndex() + 1;
        parseBundlesNumber();
        parseDestinationsEid();
        parseSizes();
        parseStarts();
        packetName = par("packetName");
        EV << "Bundles Number: " << bundlesNumber.size()
           << " Destination Eid: " << destinationsEid.size()
           << " Sizes: " << sizes.size()
           << " Starts: " << starts.size() << endl;
        for (unsigned int i = 0; i < bundlesNumber.size(); i++)
        {
            TrafficGeneratorMsg * trafficGenMsg = new TrafficGeneratorMsg("trafGenMsg");
            trafficGenMsg->setSchedulingPriority(TRAFFIC_TIMER);
            trafficGenMsg->setKind(TRAFFIC_TIMER);
            trafficGenMsg->setBundlesNumber(bundlesNumber.at(i));
            trafficGenMsg->setDestinationEid(destinationsEid.at(i));
            trafficGenMsg->setSize(sizes.at(i));
            trafficGenMsg->setInterval(par("interval"));
            trafficGenMsg->setTtl(par("ttl"));
            scheduleAt(starts.at(i), trafficGenMsg);
        }
    }
    WATCH(appBundleSent);
    WATCH(appBundleReceived);
    WATCH(appBundleReceivedHops);
    WATCH(appBundleReceivedDelay);
}

void DtnTrafficGenerator::parseBundlesNumber() {
    const char* bundlesNumberChar = par("bundlesNumber");
    cStringTokenizer bundlesNumberTokenizer(bundlesNumberChar, ",");
    while (bundlesNumberTokenizer.hasMoreTokens()) {
        int bundleNumber = atoi(bundlesNumberTokenizer.nextToken());
        EV << "Bundle Number: " << bundleNumber << endl;
        bundlesNumber.push_back(bundleNumber);
    }
}

void DtnTrafficGenerator::parseDestinationsEid(){
    const char *destinationEidChar = par("destinationEid");
    cStringTokenizer destinationEidTokenizer(destinationEidChar, ",");
    while (destinationEidTokenizer.hasMoreTokens())
    {
        std::string destinationEidStr = destinationEidTokenizer.nextToken();
        int destinationEid = stoi(destinationEidStr);
        destinationsEid.push_back(destinationEid);
    }
}

void DtnTrafficGenerator::parseSizes(){
    const char *sizeChar = par("size");
    cStringTokenizer sizeTokenizer(sizeChar, ",");
    while (sizeTokenizer.hasMoreTokens())
        sizes.push_back(atoi(sizeTokenizer.nextToken()));
}

void DtnTrafficGenerator::parseStarts(){
    const char *startChar = par("start");
    cStringTokenizer startTokenizer(startChar, ",");
    while (startTokenizer.hasMoreTokens())
        starts.push_back(atof(startTokenizer.nextToken()));
}


void DtnTrafficGenerator::finish() {
    recordScalar("bundle sent", appBundleSent);
    recordScalar("bundle received", appBundleReceived);
    recordScalar("bundle received hops", appBundleReceivedHops);
    recordScalar("bundle received delay", appBundleReceivedDelay);
}


void DtnTrafficGenerator::handleMessage(cMessage *msg) {
    if (msg->getKind() == TRAFFIC_TIMER){
        TrafficGeneratorMsg* trafficGenMsg = check_and_cast<TrafficGeneratorMsg *>(msg);
        std::ostringstream str;
        str << packetName << "-" << appBundleSent;
        Packet *packet = new Packet(str.str().c_str());
        const auto &payload = makeShared<ApplicationPacket>();
        payload->setChunkLength(B(par("messageLength")));
        payload->setSequenceNumber(appBundleSent);
        payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtBack(payload);

        const auto transportHeader = makeShared<DtnTransportHeader>();
        transportHeader->setChunkLength(B(1));
        transportHeader->setBundlesNumber(trafficGenMsg->getBundlesNumber());
        transportHeader->setDestinationEid(trafficGenMsg->getDestinationEid());
        transportHeader->setSize(trafficGenMsg->getSize());
        transportHeader->setInterval(trafficGenMsg->getInterval());
        transportHeader->setTtl(trafficGenMsg->getTtl());
        packet->insertAtFront(transportHeader);
        packet->setKind(TRANSPORT_HEADER);

        // Keep generating traffic
        trafficGenMsg->setBundlesNumber((trafficGenMsg->getBundlesNumber() - 1));
        if (trafficGenMsg->getBundlesNumber() == 0)
            delete msg;
        else
            scheduleAt(simTime() + trafficGenMsg->getInterval(), msg);
        send(packet, "socketOut");
        appBundleSent++;
    }
}

int DtnTrafficGenerator::getEid() const
{
    return eid_;
}

vector<int> DtnTrafficGenerator::getBundlesNumber()
{
    return this->bundlesNumber;
}

vector<int> DtnTrafficGenerator::getDestinationsEid()
{
    return this->destinationsEid;
}

vector<int> DtnTrafficGenerator::getSizes()
{
    return this->sizes;
}

vector<double> DtnTrafficGenerator::getStarts()
{
    return this->starts;
}

}  // namespace traffic
}  // namespace flora
