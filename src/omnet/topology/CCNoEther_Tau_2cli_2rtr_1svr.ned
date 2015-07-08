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


network CCNoEther_Tau_2cli_2rtr_1svr
{
    parameters:
        @display("bgb=912,359");

        int defaultDebugLevel = default(3);		// 0=none, 1=Info, 2=Warning, 3=Error, 4=Detail
        bool auxDebug = default(false);			// enable console output for debugging

    types:
        channel fastEthernet extends DatarateChannel
        {
            delay = 0.5us;
            datarate = 100Mbps;
        }

    submodules:

        admin: CcnAdmin {
            @display("p=38,28");
        }

        client1: CcnMacNode {
            parameters:
                @display("p=214,62");
                //eth[0].mac.address = "0A-00-00-00-00-0A";	// manually set mac address
            gates:
                ethg[1];									// number of ethernet interfaces active on node
        }


        client2: CcnMacNode {
            parameters:
                @display("p=214,252");
            gates:
                ethg[1];									// number of ethernet interfaces active on node
        }


        router1: CcnMacNode {
            parameters:
                @display("p=234,150");
            gates:
                ethg[3];									// number of ethernet interfaces active on node
        }

        router2: CcnMacNode {
            parameters:
                @display("p=373,150");
            gates:
                ethg[2];									// number of ethernet interfaces active on node
        }

        server1: CcnMacNode {
            parameters:
                @display("p=656,150");
            gates:
                ethg[1];									// number of ethernet interfaces active on node
        }

    connections:

        client1.ethg[0] <--> fastEthernet <--> router1.ethg[0];
        client2.ethg[0] <--> fastEthernet <--> router1.ethg[1];

        router1.ethg[2] <--> fastEthernet <--> router2.ethg[0];
        router2.ethg[1] <--> fastEthernet <--> server1.ethg[0];
}
