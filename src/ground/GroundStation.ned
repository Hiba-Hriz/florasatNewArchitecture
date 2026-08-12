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

import inet.node.inet.INetworkNode;
import inet.common.MessageDispatcher;
import inet.applications.contract.IApp;
import inet.transportlayer.contract.IUdp;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.linklayer.loopback.LoopbackInterface;
import inet.linklayer.contract.IWiredInterface;
import inet.linklayer.acking.AckingWirelessInterface;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import flora.mobility.GroundStationMobility;
import flora.ground.DummyNic;

import flora.ground.GroundForwarder;


// The GroundStation module realys the communications between the LoRaGW (satellites)
// and the LoRa Network Server. The communication link GroundStation and satellite
// follows a satellite protocol while the communication link GroundStation and Network
// Server follows UDP protocol, thus assuming the GroundStation has a stable and
// reliable connection with the Internet
//
// IMPORTANT this module is not yet integrated into the DtS-IoT network due to the lack
// of an implemented satellite protocol
module GroundStation like INetworkNode //WirelessHost
{
    parameters:
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");

        int numEthInterfaces = default(0);  // minimum number of ethernet interfaces
        int numWlanInterfaces = 1;

        bool hasIpv4 = default(true);
        int numUdpApps = default(0);
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        string udpType = default(firstAvailableOrEmpty("UDP"));
        string networkLayerType = default("Ipv4NetworkLayer");
        string routingTableType = default("Ipv4RoutingTable");

    gates:
        inout ethg @labels(EtherFrame-conn) @allowUnconnected;
        inout satelliteLink;

    submodules:

	    // to be added the UHF submodule for communication with satellites
	    // the UHF will likely implement a satellite protocol rather than a simeple one
	    // Cubesat Satellite Protocol CSP is a tentative option but it requires
	    // integration into the omnet++ environment

        mobility: GroundStationMobility {
            @display("p=71,158");
        }

        interfaceTable: InterfaceTable {
            @display("p=71,77");
        }

        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=71,235;is=s");
        }

        groundForwarder: GroundForwarder {
            @display("p=200,185");
        }

        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=321,185");
        }
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=403,185;q=queue");
        }
        md: MessageDispatcher {
            @display("p=403,259");
        }
        eth: <default("EthernetInterface")> like IWiredInterface {
            parameters:
                @display("p=403,345,row,60;q=txQueue");
        }

  


    connections allowunconnected:

        satelliteLink <--> groundForwarder.satLink;

        // Connexions UDP conditionnelles
    	groundForwarder.socketOut --> udp.appIn if hasUdp;
    	groundForwarder.socketIn <-- udp.appOut if hasUdp;

    	udp.ipOut --> ipv4.transportIn if hasUdp;
	    udp.ipIn <-- ipv4.transportOut if hasUdp;

    	ipv4.ifOut --> md.in++ if hasIpv4;
    	ipv4.ifIn <-- md.out++ if hasIpv4;

    	md.out++ --> eth.upperLayerIn;
	    md.in++ <-- eth.upperLayerOut;

    	ethg <--> { @display("m=s"); } <--> eth.phys;

}
