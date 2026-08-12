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

//
// This module will handle packet coming from 3 interfaces:
// LoRaGWNic for communication with LoRa Nodes on ground
// ISLNic for communication with adjacent Satellites
// UHFGWNic for communication with the GorundStations
//
// Packet handling includes forwarding, queueing and conversion
// of packets, translating messages between different protocols
// 
// This PacketHandler version is for a wired ISL configuration
simple PacketHandlerWired
{
    parameters:
        @display("i=block/app");
        @signal[LoRa_GWPacketReceived](type=long); // optional
        @statistic[LoRa_GWPacketReceived](source=LoRa_GWPacketReceived; record=count);
    
    	int destPort;
        int localPort = default(-1);  // local port (-1: use ephemeral port)
        string destAddresses = default(""); // list of IP addresses, separated by spaces ("": don't send)
        string localAddress = default("");
        bool globalGrid = default(false);
       
       
     
		double rx1Delay @unit(s) = default(1.0s);  // Receive window 1 delay (standard LoRaWAN)
		double rx2Delay @unit(s) = default(2.0s);  // Receive window 2 delay (standard LoRaWAN)
    gates:        
        // ISL gates
        inout left1;
        inout right1;
        inout up1;
        inout down1;
        // LoRa gates
        input lowerLayerLoRaIn @labels(LoRaGate/up);
        output lowerLayerLoRaOut @labels(LoRaGate/down);
        
        // GroundStation gates
        inout lowerLayerGS @labels(GroundStationGate);
    

    	output toNS;       // vers NetworkServer
    	input fromNS;      // depuis NetworkServer
}
