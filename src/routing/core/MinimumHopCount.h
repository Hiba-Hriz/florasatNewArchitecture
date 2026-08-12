/*
 * RoutingBase.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_CORE_MINIMUMHOPCOUNT_H_
#define __FLORA_ROUTING_CORE_MINIMUMHOPCOUNT_H_

namespace flora {
namespace routing {
namespace core {

// (o, i) => ith satellite in orbital plane o
// Q satellites per plane, inclination 𝛼, altitude h, F relative spacing between sats in adjacent planes
// F usually given as phasing factor F € {0, ..., P-1}
// Walker delta: PQ/P/F
// RAAN difference:     ΔΩ = 2𝜋/𝑃 € [0,2𝜋]      -> how far neighbouring planes are from each other
// Phase difference:    ΔΦ = 2𝜋/Q € [0,2𝜋]      -> difference in arg of latitude
// Phase offset:        Δf = 2𝜋F/PQ € [0,2𝜋[    -> difference in arg of latitude between horizontal neighbours


}  // namespace core
}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_CORE_MINIMUMHOPCOUNT_H_