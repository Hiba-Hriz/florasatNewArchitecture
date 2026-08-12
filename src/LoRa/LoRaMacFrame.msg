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

//
// TODO generated message class
//

import inet.common.INETDefs;
import inet.common.Units;
import inet.linklayer.common.MacAddress;
import inet.common.packet.chunk.Chunk;

cplusplus {{
using namespace inet;
}}

namespace flora;

enum MacFrameType
{
    OTHER = 1;
    BEACON = 2;
    UPLINK = 3;
    DOWNLINK = 4;
}

class LoRaMacFrame extends inet::FieldsChunk {
    inet::MacAddress transmitterAddress;
    inet::MacAddress receiverAddress;

    int pktType @enum(MacFrameType);

    int sequenceNumber;
    double LoRaTP;
    inet::Hz LoRaCF;
    int LoRaSF;
    inet::Hz LoRaBW;
    int LoRaCR;
    bool LoRaUseHeader;
    double RSSI;
    double SNIR;
    
    int BeaconTimer = 128;
    int PingNb = 4;
    int PingPeriod = 32;
    int PingOffset;
    int BeaconTime;
    
    int numHop = 0;
    int satNumber;
    //int SlotLen;
    
    // message creation in node timestamp
    simtime_t originTime;
    
    // ground station message arrival timestamp
    simtime_t groundTime;
    simtime_t gwReceptionTime = 0;
    
    // route stores satellite indexes realying this message
    // timestamps stores the time when this message is relayed 
    int route[];
    simtime_t timestamps[];
    
}
