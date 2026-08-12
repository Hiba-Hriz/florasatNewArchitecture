/*
 * DijkstraShortestPath.cc
 *
 * Created on: Apr 21, 2023
 *     Author: Robin Ohs
 */

#include "DijkstraShortestPath.h"

namespace flora {
namespace routing {
namespace core {

DijkstraResult runDijkstra(int src, const std::unordered_map<int, SatelliteRoutingBase*>& constellation, int dst, bool earlyAbort) {
    int size = constellation.size();
    ASSERT(src >= 0 && src < size);
    ASSERT(!earlyAbort || dst >= 0 && dst < size);

    std::vector<std::vector<double>> cost;
    std::vector<double> distance;
    std::vector<int> prev;
    std::vector<bool> visited;

    for (size_t i = 0; i < size; i++) {
        distance.push_back(INF);
        prev.push_back(-1);
        visited.push_back(false);
        std::vector<double> v;
        for (size_t j = 0; j < size; j++) {
            v.push_back(INF);
        }
        cost.emplace_back(v);

        const SatelliteRoutingBase* sat = constellation.at(i);
        ASSERT(sat != nullptr);
        if (sat->hasLeftSat()) {
            cost[i][sat->getLeftSatId()] = sat->getLeftSatDistance();
        }
        if (sat->hasUpSat()) {
            cost[i][sat->getUpSatId()] = sat->getUpSatDistance();
        }
        if (sat->hasRightSat()) {
            cost[i][sat->getRightSatId()] = sat->getRightSatDistance();
        }
        if (sat->hasDownSat()) {
            cost[i][sat->getDownSatId()] = sat->getDownSatDistance();
        }
    }

    distance[src] = 0;

    for (size_t i = 0; i < size; i++) {
        int minValue = INF;
        int nearest = -1;
        for (size_t j = 0; j < size; j++) {
            if (!visited[j] && distance[j] < minValue) {
                minValue = distance[j];
                nearest = j;
            }
        }
        ASSERT(nearest != -1);
        visited[nearest] = true;

        // EARLY ABORT
        if (earlyAbort && nearest == dst) {
            break;
        }

        for (size_t j = 0; j < size; j++) {
            if (cost[nearest][j] != INF && distance[j] > distance[nearest] + cost[nearest][j]) {
                distance[j] = distance[nearest] + cost[nearest][j];
                prev[j] = nearest;
            }
        }
    }

    std::vector<double> vDistance;
    for (size_t i = 0; i < size; i++) {
        vDistance.push_back(distance[i]);
    }

    std::vector<int> vPrev;
    for (size_t i = 0; i < size; i++) {
        vPrev.push_back(prev[i]);
    }

    return DijkstraResult{vDistance, vPrev};
}

std::vector<int> reconstructPath(int src, int dest, std::vector<int> prev) {
    std::deque<int> path;
    path.push_back(dest);
    int it = dest;
    while (it != src) {
        it = prev[it];
        path.push_front(it);
    }
    std::vector<int> v(std::make_move_iterator(path.begin()),
                       std::make_move_iterator(path.end()));
    return v;
}

}  // namespace core
}  // namespace routing
}  // namespace flora