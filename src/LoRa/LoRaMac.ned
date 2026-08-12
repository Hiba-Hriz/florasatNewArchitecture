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

//based on Ieee802154NarrowbandMac

package flora.LoRa;

import inet.linklayer.csmaca.CsmaCaMac;

module LoRaMac extends CsmaCaMac
{
    parameters:
        string classUsed  = default("A");
        bool withRetransmission = default(true);
        bool usingAck = default(true);
        
        int maxRetransmissions = default(7); 
        // RX1 RX2 receive windows parameters
        int waitDelay1Time = default(1);
        int listening1Time = default(1);
        int waitDelay2Time = default(1);
        int listening2Time = default(1);
         int payloadSize @unit(B) = default(10B);
        
        // beacon parameters
        double beaconStart = default(1);
        double beaconPeriodTime = default(128);
        double beaconReservedTime = default(2.120);
        double beaconGuardTime = default(3);
        bool useCSMA = default(false);
         bool useSlottedAloha = default(false);
        
        int difsCount = default(2);        // DIFS: 2 CADs
        int backoffMax = default(6);       // BACKOFF_MAX
        int maxChanges = default(6);       // MaxChanges
        
        // class B parameters
        double classBslotTime = default(0.030);
        int timeToNextSlot = default(5);
        int pingOffset = default(5);
        
    
        // class S parameters
        double maxToA = default(0.96);
        double clockThreshold = default(0.010);
       
        // FSA Game parameters
        bool FSAGame = default(false);
        int realNodeNumber = default(0);
        
        bitrate = 250bps;
        @class(LoRaMac);
        
        // Signals and Statistics
        @signal[macDPAD];
        @signal[macDPADOwnTraffic];
        
        @statistic[macDPAD](title="macDPAD"; source="macDPAD"; record=mean,max,min,vector);
        @statistic[macDPADOwnTraffic](title="macDPADOwnTraffic"; source="macDPADOwnTraffic"; record=mean,max,min,vector);
        
    gates:
        input upperMgmtIn;
        output upperMgmtOut;
}
