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

package flora.LoRaPhy;

import inet.physicallayer.wireless.common.medium.RadioMedium;
import flora.physicallayer.propagation.DtSIoTPropagation;

module LoRaMedium extends RadioMedium
{
    parameters:
        propagation.typename = default("DtSIoTPropagation");
        analogModel.typename = default("LoRaAnalogModel");

        // 802.15.4-2006, page 266
        //pathLoss.typename = default("LoRaLogNormalShadowing");
        pathLoss.typename = default("FreeSpacePathLoss");
        backgroundNoise.typename = "DimensionalBackgroundNoise";
        
        rangeFilter = "communicationRange";
        radioModeFilter = true;

        // Reflects the thermal noise for the receiver sensitivity
        // TODO couple with sensitivity
        backgroundNoise.power = default(-96.616dBm);
        backgroundNoise.dimensions = default("time");
        @class(LoRaMedium);
}
