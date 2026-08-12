package flora.mobility;

import inet.mobility.base.MovingMobilityBase;

//
// Manages the SatelliteMobility
//
simple SatelliteMobility extends MovingMobilityBase
{
    parameters:
        @class(SatelliteMobility);
        @display("i=block/cogwheel_s");
        bool displaySpanArea = default(false);
        int effectiveSlant = default(0);
}
