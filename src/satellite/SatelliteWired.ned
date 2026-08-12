package flora.satellite;

import inet.node.inet.INetworkNode;
import inet.networklayer.common.InterfaceTable;
import inet.node.inet.StandardHost;

import flora.mobility.NoradA;
import flora.mobility.SatelliteMobility;
import flora.LoRa.LoRaGWNic;
import flora.satellite.PacketHandlerWired;

module SatelliteWired like INetworkNode
{
    parameters:
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");
        @display("bgb=633,339");

    gates:
        inout groundLink;
        inout left;
        inout right;
        inout up;
        inout down;
     	
    submodules:
        mobility: SatelliteMobility {
            @display("p=54,56");
        }

        NoradModule: NoradA {
            @display("p=153,56");
        }

        LoRaGWNic: LoRaGWNic {
            @display("p=54,206");
        }

        interfaceTable: InterfaceTable {
            @display("p=270,56");
        }
        packetHandler: PacketHandlerWired {
            @display("p=150,200");
        }

        networkServer: flora.ground.LoRaNetworkServerApp {
            @display("p=350,200");
        }



    connections allowunconnected:
        packetHandler.down1 <--> down;
        packetHandler.right1 <--> right;
        packetHandler.left1 <--> left;
        packetHandler.up1 <--> up;

        LoRaGWNic.upperLayerIn <-- packetHandler.lowerLayerLoRaOut;
        LoRaGWNic.upperLayerOut --> packetHandler.lowerLayerLoRaIn;
        
        // packet to and from the GroundStation and the PacketHandler
        // uhf radio module to be implemented
        groundLink <--> packetHandler.lowerLayerGS;

        // GW -> NS
        packetHandler.toNS --> networkServer.in;

        // NS -> GW
        networkServer.out --> packetHandler.fromNS;
        

}
