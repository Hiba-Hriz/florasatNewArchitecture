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

package flora.ground;

import flora.mobility.GroundStationMobility;
import flora.traffic.UdpTrafficGenerator;

module GroundStationRouting extends GroundStationRoutingBase
{
    parameters:
        @class(ground::GroundStationRouting);

    submodules:
        gsMobility: GroundStationMobility {
            @display("p=100,200");
        };
        packetGenerator: PacketGenerator {
            @display("p=100,500");
        };

        udpTrafficGenerator: UdpTrafficGenerator {
            @display("p=500,500");
        };

    connections allowunconnected:
        
        //GROUNDLINK LINKS (GS TO SATELLITE)
        for i=0..numGroundLinks-1 {
            satelliteLink[i] <--> packetGenerator.satelliteLink[i];
        }

        packetGenerator.transportOut --> udpTrafficGenerator.socketIn;
        packetGenerator.transportIn <-- udpTrafficGenerator.socketOut;
}