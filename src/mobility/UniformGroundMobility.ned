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
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package flora.mobility;

import inet.mobility.static.StationaryMobility;
import inet.mobility.contract.IMobility;

//
// Initializes the initial positions of LoRa nodes based on a uniform
// distribution given a center point in latitude, longitude coordinates
// and a radius in degrees
//
simple UniformGroundMobility extends StationaryMobility
{
    parameters:
        @class(UniformGroundMobility);
        double centerLatitude = default(-15.0);
        double centerLongitude = default(-56.0);
        double deploymentRadius = default(200.0);
        initFromDisplayString = false;
        updateFromDisplayString = false;
}
