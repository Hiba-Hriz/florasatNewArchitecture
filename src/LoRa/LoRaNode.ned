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

package flora.LoRa;

import inet.networklayer.common.InterfaceTable;
import inet.mobility.contract.IMobility;
import flora.LoRa.LoRaNic;
import flora.LoRaApp.SimpleLoRaApp;
import flora.mobility.UniformGroundMobility;

module LoRaNode
{
    parameters:
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        @display("bgb=270,203;i=device/accesspoint,gold,90;is=s");

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=72,32");
        }
        mobility: <default("UniformGroundMobility")> like IMobility if typename != "" {
            @display("p=72,152");
        }
        LoRaNic: LoRaNic {
            @display("p=202,152");
        }
        SimpleLoRaApp: SimpleLoRaApp {
            @display("p=202,32");
        }

    connections allowunconnected:
        SimpleLoRaApp.appOut --> LoRaNic.upperLayerIn;
        LoRaNic.upperLayerOut --> SimpleLoRaApp.appIn;
}
