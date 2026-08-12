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

package flora.LoRaApp;

//
// TODO auto-generated module
//
simple SimpleLoRaApp
{
    parameters:
        @signal[LoRa_AppPacketSent](type=long); // optional
        @statistic[LoRa_AppPacketSent](source=LoRa_AppPacketSent; record=count);
        int numberOfPacketsToSend = default(1);
        volatile double timeToFirstPacket @unit(s) = default(10s);
        volatile double timeToNextPacket @unit(s) = default(10s);
        volatile double ackTimeout @unit(s) = default(7s);
         int maxRetransmissions = default(2); 
        volatile double timer @unit(s) = default(1s);
        double initialLoRaTP @unit(dBm) = default(14dBm);
        double initialLoRaCF @unit(Hz) = default(868MHz);
        int initialLoRaSF = default(12);
        int ADR_ACK_LIMIT = default(64);   
        int ADR_ACK_DELAY = default(32); 
        double initialLoRaBW @unit(Hz) = default(125kHz);
        int initialLoRaCR = default(4);
        bool initialUseHeader = default(true);
        int initialPingSlot = default(1);
        bool InitClassA = default(true);
        bool InitClassB = default(false);
        bool evaluateADRinNode = default(false);
        int payloadSize @unit(B) = default(10B);
        bool usingAck = default(true);
        double minElevationAngle @unit(deg) = default(5deg);
        double maxDistance @unit(m) = default(2000km);
    gates:
        output appOut @labels(LoRaAppPacket/down);
        input appIn @labels(LoRaAppPacket/up);
}
