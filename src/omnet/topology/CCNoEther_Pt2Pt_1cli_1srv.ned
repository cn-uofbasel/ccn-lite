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

package unibas.ccnlite.topology;

import unibas.ccnlite.node.CcnMacNode;
import unibas.ccnlite.CcnAdmin;
import ned.DatarateChannel;


network Ether_Pt2Pt_1cli_1srv
{
    parameters:

        double linkDataRate @unit(Mbps) = default(100Mbps);
        double linkDelay @unit(us) = default(0.5us);

        string client1SendsTo = default("");  // destination MAC address, or module path name of destination station; empty means off
        string client2SendsTo = default("");  // destination MAC address, or module path name of destination station; empty means off

        string pinger = default("");	// who initiates test, if "" then both clients do

        double testStartTime @unit(s) = default(0);  // time of sending the first request
        double testStopTime @unit(s) = default(0);  // time of finish sending, 0 means forever

        double sendInterval @unit(s) = default(uniform(0s,1s));  // interval between sending requests
        double responseInterval @unit(s) = default(uniform(0s,1s));  // interval between sending requests

        int defaultDebugLevel = default(3);		// 0=none, 1=Info, 2=Warning, 3=Error, 4=Detail

    submodules:

        client1: CcnMacNode {
            parameters:
                @display("p=131,67");
            gates: ethg[1];									// number of ethernet interfaces active on node
        }

        client2: CcnMacNode {
            parameters:
                @display("p=131,195");
            gates: ethg[1];
        }

        admin: CcnAdmin {
            @display("p=38,28");
        }

    connections:
        //client1.ethg++ <--> {  datarate = linkDataRate; delay = linkDelay; } <--> client2.ethg++;
        client1.ethg[0] <--> {  datarate = linkDataRate; delay = linkDelay; } <--> client2.ethg[0];
}
