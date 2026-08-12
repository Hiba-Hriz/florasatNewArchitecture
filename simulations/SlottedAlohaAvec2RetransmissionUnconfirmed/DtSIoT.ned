package flora.simulations.SlottedAlohaAvec2RetransmissionUnconfirmed;

import inet.applications.udpapp.UdpBasicApp;
import inet.node.inet.StandardHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;
import inet.visualizer.common.IntegratedVisualizer;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import flora.ground.GroundStation;
import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.satellite.SatelliteWired;

@license(LGPL);
network DtSIot
{
    parameters:
        int nrOfNodes = default(1);
        int nrOfGW = default(1);
        int nrOfGS = default(0);
        @display("bgb=2160,1080;bgi=background_earth,s");

    types:
        channel DatarateChannel2 extends ned.DatarateChannel {
            datarate = 100Mbps;
            delay = 0.002s;
            @display("ls=black,1");
        }
        channel DatarateChannel1 extends ned.DatarateChannel {
            datarate = 20kbps;
            delay = 0s;
            @display("ls=red,2");
        }

    submodules:
        loRaNodes[nrOfNodes]: LoRaNode {
            @display("p=695.185,733.055;is=vs");
        }
        loRaGW[nrOfGW]: SatelliteWired {
            @display("p=695.185,549.115;i=satellit_blue");
        }
    
        LoRaMedium: LoRaMedium {
            @display("p=124.43,297.55");
        }
        dummyMedium: UnitDiskRadioMedium {
            @display("p=121.725,927.815;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                assignDisjunctSubnetAddresses = false;
            @display("p=124.43,102.79");
        }
        visualizer: IntegratedVisualizer {
            @display("p=124.43,675.0");
        }

    connections allowunconnected:
   // Aucune connexion externe necessaire : networkServer est interne a SatelliteWired
}