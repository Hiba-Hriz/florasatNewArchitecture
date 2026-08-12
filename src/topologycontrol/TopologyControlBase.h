/*
 * TopologyControlBase.h
 *
 * Created on: May 05, 2023
 *     Author: Robin Ohs
 */
#ifndef __FLORA_TOPOLOGYCONTROL_TOPOLOGYCONTROLBASE_H_
#define __FLORA_TOPOLOGYCONTROL_TOPOLOGYCONTROLBASE_H_

#include <omnetpp.h>

#include <unordered_map>

#include "core/Utils.h"
#include "core/WalkerType.h"
#include "ground/GroundStationRoutingBase.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "satellite/SatelliteRoutingBase.h"
#include "topologycontrol/data/GsSatConnection.h"
#include "topologycontrol/utilities/ChannelState.h"

using namespace omnetpp;
using namespace flora::satellite;
using namespace flora::ground;
using namespace flora::core;

namespace flora {
namespace topologycontrol {

class TopologyControlBase : public ClockUserModuleMixin<cSimpleModule> {
   public:
    TopologyControlBase();

    virtual void updateTopology() = 0;

    // Sat API
    SatelliteRoutingBase *const getSatellite(int satId) const;
    SatelliteRoutingBase *const findSatByPlaneAndNumberInPlane(int plane, int numberInPlane) const;
    std::unordered_map<int, SatelliteRoutingBase *> const &getSatellites() const;

    int getNumberOfSatellites() const {
        return numSatellites;
    };

    // GS API
    GroundStationRoutingBase *const getGroundstationInfo(int gsId) const;

    // Connections API
    GsSatConnection const &getGroundstationSatConnection(int gsId, int satId) const;

   protected:
    virtual ~TopologyControlBase();
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    virtual void handleMessage(cMessage *msg) override;

    /** This method is called if the topology has changed.
     * It is only called if there is a pre-plannable topology change.
     * E.g., ISL gates that get disabled do not lead to a call.
     */
    virtual void trackTopologyChange() {}

    /** Used to connect a satellite pair with the given direction as the output direction of the first satellite.
     * The input gate is the output gate of the second satellite on the counter direction.
     */
    void connectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction direction);

    /** Used to connect a satellite pair with the given direction as the output direction of the first satellite.
     * The input gate is the output gate of the second satellite on the counter direction.
     */
    void disconnectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction direction);

    /** @brief Creates a one-way connection between the outGate and the inGate.
     * Considers the ISLStates of the gates and only connects if they are both working.
     */
    void createSatelliteConnection(cGate *outGate, cGate *inGate, double delay, double datarate, ISLState outState, ISLState inState);

    /** @brief Creates/Updates the channel from outGate to inGate. If channel exists updates channel params, otherwise creates the channel.*/
    ChannelState updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate);
    /** @brief Deletes the channel of outGate. If channel does not exist, nothing happens, otherwise deletes the channel.*/
    ChannelState deleteChannel(cGate *outGate);

   private:
    /** @brief Schedules the update timer that will update the topology state.*/
    void scheduleUpdate();
    void loadSatellites();
    void loadGroundstations();

   protected:
    /** @brief Map of satellite ids and their correspinding SatelliteInfo data struct. */
    std::unordered_map<int, SatelliteRoutingBase *> satellites;
    int numSatellites;

    /** @brief Structs that represent groundstations and all satellites in range. */
    std::unordered_map<int, GroundStationRoutingBase *> groundStations;
    int numGroundStations;
    int numGroundLinks = 40;

    /** @brief Structs that represent connections between satellites and groundstations. */
    std::map<std::pair<int, int>, GsSatConnection> gsSatConnections;

    /** @brief Can be set to true for constellations that do not feature inter-plane ISL*/
    bool interPlaneIslDisabled;
    /** @brief Delay of the isl channel, in microseconds/km. */
    double islDelay;
    /** @brief Datarate of the isl channel, in bit/second. */
    double islDatarate;

    /** @brief Delay of the groundlink channel, in microseconds/km. */
    double groundlinkDelay;
    /** @brief Datarate of the groundlink channel, in bit/second. */
    double groundlinkDatarate;
    /** @brief The minimum elevation between a satellite and a groundstation.*/
    double minimumElevation;

    WalkerType::WalkerType walkerType;
    int interPlaneSpacing;
    int planeCount;
    int satsPerPlane;

    /** @brief Used to indicate if there was a change to the topology. */
    bool topologyChanged = false;

   private:
    /**
     * @brief The simulation time interval used to regularly signal mobility state changes.
     *
     * The 0 value turns off the signal.
     */
    cPar *updateIntervalParameter = nullptr;
    ClockEvent *updateTimer = nullptr;
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_TOPOLOGYCONTROLBASE_H_
