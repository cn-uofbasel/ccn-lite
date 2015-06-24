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

package unibas.ccnlite.node;

import unibas.ccnlite.Ccn;
import inet.networklayer.common.InterfaceTable;
import inet.linklayer.IWiredNic;
//import inet.mobility.IMobility;
import inet.base.NotificationBoard;
//import inet.mobility.models.StationaryMobility;


module CcnMacNode
{
    parameters:
        @display("i=device/pc");
        @node;
        @labels(node,ethernet-node);        

    gates:
        inout ethg[] @labels(EtherFrame-conn);

    submodules:

        // events pub-sub in a cross-layer fashion
        notificationBoard: NotificationBoard {
            parameters:
                @display("p=82,112");
        }

        // --- Network Layer
        net: Ccn {
            @display("p=176,167");
        }

        interfaceTable: InterfaceTable {
            parameters:
                @display("p=82,41");
        }

        // --- Link layer Wired Ethernet NICs
        eth[sizeof(ethg)]: <default("EthernetInterface")> like IWiredNic {
            parameters:
                @display("p=116,238,row,90;q=txQueue");
        }

    connections allowunconnected:

        // --- connect NICs to outside world and the network layer

        for i=0..sizeof(ethg)-1 {
            ethg[i] <--> eth[i].phys;
            eth[i].upperLayerOut --> net.ifIn++;	// NOTE: in older versions of INET the gates upperLayerIn/Out at IWiredNic were named netwIn/Out
            eth[i].upperLayerIn <-- net.ifOut++;
        }
}
