/*
 * MetricsCollector.h
 *
 *  Created on: Mar 13, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_METRICS_METRICSCOLLECTOR_H_
#define __FLORA_METRICS_METRICSCOLLECTOR_H_

#include <omnetpp.h>

#include <vector>

#include "PacketState.h"
#include "routing/RoutingHeader_m.h"

using namespace omnetpp;
using namespace inet;

namespace flora {
namespace metrics {

class MetricsCollector : public cSimpleModule {
   public:
    MetricsCollector();
    /** @brief Records the given packet with a given packet state. */
    void record_packet(PacketState::Type state, RoutingHeader packet);

   protected:
    virtual void finish() override;
    virtual void initialize(int stage) override;

   private:
    void print_packet(RoutingHeader *frame);

    /** @brief Calculates the latency (time from source to destination) of a frame (in ms). */
    double calculate_latency(RoutingHeader *frame);
    /** @brief Calculates the average latency (time from source to destination) of vector of frames (in ms). */
    double calculate_average_latency(std::vector<RoutingHeader> *frames);

   private:
    std::map<PacketState::Type, std::vector<RoutingHeader>> recordedPackets;
};

}  // namespace metrics
}  // namespace flora

#endif  // __FLORA_METRICS_METRICSCOLLECTOR_H_