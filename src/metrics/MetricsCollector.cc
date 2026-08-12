/*
 * MetricsCollector.cc
 *
 *  Created on: Mar 13, 2023
 *      Author: Robin Ohs
 */

#include "MetricsCollector.h"

namespace flora {
namespace metrics {

Define_Module(MetricsCollector);

MetricsCollector::MetricsCollector() {
}

void MetricsCollector::finish() {
    auto delivered_packets = &recordedPackets.at(PacketState::DELIVERED);
    if (delivered_packets != nullptr && delivered_packets->size() != 0) {
        double average_delivery_latency = calculate_average_latency(delivered_packets);
        EV << "Mean latency: " << average_delivery_latency << "ms." << endl;
        recordScalar("#mean-latency", average_delivery_latency);
    }
}

void MetricsCollector::initialize(int stage) {
    // init recordedPackets map
    for (const auto e : PacketState::All) {
        std::vector<RoutingHeader> vector;
        recordedPackets.emplace(e, vector);
    }
}

void MetricsCollector::record_packet(PacketState::Type state, RoutingHeader frame) {
    recordedPackets.at(state).emplace_back(frame);
    print_packet(&frame);
}

void MetricsCollector::print_packet(RoutingHeader *frame) {
    int source = frame->getSourceGroundstation();
    int destination = frame->getDestinationGroundstation();
    int hops = frame->getNumHop();
    double latency = calculate_latency(frame);

    std::stringstream ss;
    ss << "GS[" << destination << "]: Received msg from " << source << endl
       << " -> Hops:" << hops << " | Latency: " << latency << "ms | Route: {";
    for (size_t i = 0; i < frame->getRouteArraySize(); i++) {
        int pathSat = frame->getRoute(i);
        ss << pathSat << ",";
    }
    ss << "}.";
    EV << ss.str() << endl;
}

double MetricsCollector::calculate_latency(RoutingHeader *frame) {
    simtime_t latency = frame->getReceptionTime() - frame->getOriginTime();
    // EV << "{Sent: " << frame->getOriginTime() << "| Received: " << frame->getReceptionTime() << "} => " << latency << "," << latency.dbl() * 1000 << endl;
    return latency.dbl() * 1000;
}

double MetricsCollector::calculate_average_latency(std::vector<RoutingHeader> *frames) {
    if (frames->size() == 0)
        throw cRuntimeError("Error in MetricsCollector::calculate_average_latency(): Was called with empty vector.");
    double sum = 0.0;
    for (auto &element : *frames) {
        sum += calculate_latency(&element);
    }
    return sum / frames->size();
}

}  // namespace metrics
}  // namespace flora
