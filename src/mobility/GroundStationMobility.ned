
package flora.mobility;

import inet.mobility.static.StationaryMobility;
import inet.mobility.contract.IMobility;

//
// Initializes the initial positions of Base Stations
//
simple GroundStationMobility extends StationaryMobility
{
    parameters:
        double latitude;
        double longitude;
        initFromDisplayString = false;
        updateFromDisplayString = false;
        @class(GroundStationMobility);
}
