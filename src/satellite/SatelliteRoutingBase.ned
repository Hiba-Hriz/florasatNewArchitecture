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

package flora.satellite;

import flora.mobility.NoradA;
import flora.mobility.SatMobility;

module SatelliteRoutingBase
{
    parameters:
        @class(satellite::SatelliteRoutingBase);
        @display("p=695.185,549.115;i=satellit_blue");
        int numGroundLinks = default(40);
        double upperLatitudeBound = default(70.0);
        double lowerLatitudeBound = default(-70.0);

    gates:
        inout left;
        inout right;
        inout up;
        inout down;
        inout groundLink[numGroundLinks];

    submodules:
        satMobility: SatMobility {
            @display("p=100,100");
        }

        NoradModule: NoradA {
            @display("p=300,100");
        }
}