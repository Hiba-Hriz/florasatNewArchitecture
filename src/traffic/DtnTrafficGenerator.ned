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

package flora.traffic;

import inet.applications.contract.IApp;

simple DtnTrafficGenerator like IApp
{
    parameters:
        @class(traffic::DtnTrafficGenerator);
        @display("i=block/app");
        bool enable = default(false); // Traffic Generator Enabled
        string destinationEid = default("1"); // Generated Bundles destination EiD
        string bundlesNumber = default("1"); // Number of Bundles to Generate
        string size = default("1024"); // Generated Bundles Size in Bytes
        string start = default("0"); // tiempo de generación de mensajes
        double interval = default(0); // Bundles Generation Interval in Secs
        double ttl = default(9000000); // Generated Bundles Time to Live in Secs
        bool returnToSender = default(true);
        bool critical = default(false);
        bool custodyTransfer = default(false);
        string externalTrafficEvents = default("");
        string packetName = default("DtnUdpBasicAppData");
        volatile int messageLength @unit(B); // length of messages to generate, in bytes
        
        // Signals and Statistics
        @signal[appBundleSent];
        @signal[appBundleReceived];
        @signal[appBundleReceivedHops];
        @signal[appBundleReceivedDelay];
        
        @statistic[appBundleSent](title="appBundleSent"; source="appBundleSent"; record=count; interpolationmode=none);
        @statistic[appBundleReceived](title="appBundleReceived"; source="appBundleReceived"; record=count; interpolationmode=none);
        @statistic[appBundleReceivedHops](title="appBundleReceivedHops"; source="appBundleReceivedHops"; record=stats,histogram; interpolationmode=none);
        @statistic[appBundleReceivedDelay](title="appBundleReceivedDelay"; source="appBundleReceivedDelay"; record=stats,histogram; interpolationmode=none);
        
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}