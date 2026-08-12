/*
 * DijkstraShortestPath.h
 *
 * Created on: Apr 21, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_CORE_DIJKSTRASHORTESTPATH_H_
#define __FLORA_ROUTING_CORE_DIJKSTRASHORTESTPATH_H_

#include <omnetpp.h>

#include <array>
#include <deque>
#include <unordered_map>

#include "satellite/SatelliteRoutingBase.h"

using namespace omnetpp;
using namespace flora::satellite;

#define INF 999999999

namespace flora {
namespace routing {
namespace core {

struct DijkstraResult {
    std::vector<double> distances;
    std::vector<int> prev;
};

DijkstraResult runDijkstra(int src, const std::unordered_map<int, SatelliteRoutingBase*>& constellation, int dst = -1, bool earlyAbort = false);

std::vector<int> reconstructPath(int src, int dest, std::vector<int> prev);

}  // namespace core
}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_CORE_DIJKSTRASHORTESTPATH_H_