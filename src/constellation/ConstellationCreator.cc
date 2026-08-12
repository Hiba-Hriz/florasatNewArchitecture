/*
 * ConstellationCreator.cc
 *
 * Created on: Jan 21, 2023
 *     Author: Robin Ohs
 */

#include "ConstellationCreator.h"

namespace flora {
namespace constellation {

Define_Module(ConstellationCreator);

ConstellationCreator::ConstellationCreator() : planeCount(0),
                                               inclination(0.0),
                                               altitude(0),
                                               interPlaneSpacing(0),
                                               baseYear(0),
                                               baseDay(0.0),
                                               epochYear(0),
                                               epochDay(0.0),
                                               eccentricity(0.0),
                                               raanSpread(0),
                                               satsPerPlane(0),
                                               satCount(0) {
}

void ConstellationCreator::initialize() {
    planeCount = par("planeCount");
    inclination = par("inclination");
    altitude = par("altitude");
    interPlaneSpacing = par("interPlaneSpacing");
    baseYear = par("baseYear");
    baseDay = par("baseDay");
    epochYear = par("epochYear");
    epochDay = par("epochDay");
    eccentricity = par("eccentricity");
    raanSpread = par("raanSpread");

    // get satellite number and compute sats_per_plane
    satCount = getParentModule()->getSubmoduleVectorSize("loRaGW");
    satsPerPlane = satCount / planeCount;

    // Validate DATA
    VALIDATE(interPlaneSpacing <= planeCount - 1 && interPlaneSpacing >= 0);
    VALIDATE(altitude > 0);
    VALIDATE(raanSpread == 180 || raanSpread == 360);
    VALIDATE(satCount > 0);
    VALIDATE(planeCount > 0);
    VALIDATE(satsPerPlane > 0);
    VALIDATE(satsPerPlane * planeCount == satCount);
    
    createSatellites();
}

void ConstellationCreator::createSatellites() {
    double raanDelta = raanSpread / planeCount;                   // ΔΩ = 2𝜋/𝑃 in [0,2𝜋]
    double phaseDifference = 360.0 / satsPerPlane;                // ΔΦ = 2𝜋/Q in [0,2𝜋]
    double phaseOffset = (360.0 * interPlaneSpacing) / satCount;  // Δ𝑓 = 2𝜋𝐹/𝑃𝑄 in [0,2𝜋[

    // iterate over planes
    for (size_t plane = 0; plane < planeCount; plane++) {
        // create plane satellites
        double raan = raanDelta * plane;
        for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++) {
            int index = planeSat + plane * satsPerPlane;
            double meanAnomaly = std::fmod(plane * phaseOffset + planeSat * phaseDifference, 360.0);
            createSatellite(index, raan, meanAnomaly, plane);
        }
    }
}

// void ConstellationCreator::createSatellites() {
//     double meanAnomalyDelta = 360.0 / satsPerPlane;
//     // double angularMeasurePerSlot = (360.0 / satCount) - eccentricity * sin(angularMeasurePerSlot);
//     double angularMeasurePerSlot = (360.0 / satCount);
//     double raanDelta = raanSpread / planeCount ;

//     // iterate over planes
//     for (size_t plane = 0; plane < planeCount; plane++) {
//         // create plane satellites
//         double raan = raanDelta * plane;
//         for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++) {
//             int index = planeSat + plane * satsPerPlane;
//             double meanAnomaly = std::fmod(planeSat * meanAnomalyDelta, 360.0);
//             createSatellite(index, raan, meanAnomaly, plane);
//         }
//     }
// }

void ConstellationCreator::createSatellite(int index, double raan, double meanAnomaly, int plane) {
    cModule *parent = getParentModule();
    if (parent == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): parent is nullptr");
    }
    cModule *sat = parent->getSubmodule("loRaGW", index);
    if (sat == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): sat(%d) is nullptr", index);
    }
    NoradA *oldNoradModule = check_and_cast<NoradA *>(sat->getSubmodule("NoradModule"));
    if (oldNoradModule == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): noradModule of sat(%d) is nullptr", index);
    }

    const char *satName = oldNoradModule->par("satName").stringValue();

    oldNoradModule->deleteModule();

    cModule *noradModule = cModuleType::get("flora.mobility.NoradA")->create("NoradModule", sat);
    if (noradModule == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): Cannot create \"flora.mobility.NoradA\".");
    }
    noradModule->par("satIndex").setIntValue(index);
    noradModule->par("baseYear").setIntValue(baseYear);
    noradModule->par("baseDay").setDoubleValue(baseDay);
    noradModule->par("satName").setStringValue(satName);
    noradModule->par("planes").setIntValue(planeCount);
    noradModule->par("satPerPlane").setIntValue(satsPerPlane);
    noradModule->par("epochYear").setIntValue(epochYear);
    noradModule->par("epochDay").setDoubleValue(epochDay);
    noradModule->par("eccentricity").setDoubleValue(eccentricity);
    noradModule->par("inclination").setDoubleValue(inclination);
    noradModule->par("altitude").setDoubleValue(altitude);
    noradModule->par("raan").setDoubleValue(raan);
    noradModule->par("meanAnomaly").setDoubleValue(meanAnomaly);

    noradModule->finalizeParameters();
    noradModule->callInitialize();
    NoradA *norad = check_and_cast<NoradA *>(noradModule);
    if (norad == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): Cannot find casted created module NoradModule.");
    }
    norad->initializeMobility(simTime());
}

}  // namespace constellation
}  // namespace flora