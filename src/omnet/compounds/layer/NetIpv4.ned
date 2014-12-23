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

package unibas.ccnlite.compounds.layer;


// INET includes
import inet.networklayer.ipv4.IPv4;
import inet.networklayer.ipv4.ICMP;
import inet.networklayer.ipv4.ErrorHandling;
import inet.networklayer.arp.ARP;



// IPv4 network layer module

module NetIpv4
{
    parameters:
        @display("i=block/fork");
        bool proxyARP = default(false);
        
    gates:
        input ifIn[] @labels(IPv4Datagram);
        input tcpIn @labels(TCPSegment,IPv4ControlInfo/down);
        input udpIn @labels(UDPPacket,IPv4ControlInfo/down);
        input pingIn[];
        
        output ifOut[] @labels(IPv4Datagram);
        output tcpOut @labels(TCPSegment,IPv4ControlInfo/up);
        output udpOut @labels(UDPPacket,IPv4ControlInfo/up);
        output pingOut[];

    submodules:
        ip: IPv4 {
            parameters:
                protocolMapping = "6:0,17:1,1:2"; // see: http://www.iana.org/assignments/protocol-numbers
                @display("p=85,95;q=queue");
            gates:
                transportIn[3];
                transportOut[3];
                queueIn[sizeof(ifIn)];
        }
        arp: ARP {
            parameters:
                proxyARP = proxyARP;
                @display("p=202,142;q=pendingQueue");
            gates:
                nicOut[sizeof(ifOut)];
        }
        icmp: ICMP {
            parameters:
                @display("p=160,63");
        }
        errorHandling: ErrorHandling {
            parameters:
                @display("p=239,63");
        }

    connections allowunconnected:
        // transport Layer
        ip.transportOut[0] --> { @display("m=n"); } --> tcpOut;
        ip.transportIn[0] <-- { @display("m=n"); } <-- tcpIn;

        ip.transportOut[1] --> { @display("m=n"); } --> udpOut;
        ip.transportIn[1] <-- { @display("m=n"); } <-- udpIn;

        ip.transportOut[2] --> icmp.localIn;
        ip.transportIn[2] <-- icmp.sendOut;

		// connect ping apps to icmp
        for i=0..sizeof(pingOut)-1 {
            icmp.pingOut++ --> { @display("m=n"); } --> pingOut[i];
        }
        for i=0..sizeof(pingIn)-1 {
            icmp.pingIn++ <-- { @display("m=n"); } <-- pingIn[i];
        }

        icmp.errorOut --> errorHandling.in;

		// connect to NICs
        for i=0..sizeof(ifIn)-1 {
            ifIn[i] --> { @display("m=s"); } --> ip.queueIn[i];
        }
        ip.queueOut --> arp.ipIn;
        for i=0..sizeof(ifOut)-1 {
            arp.nicOut[i] --> { @display("m=s"); } --> ifOut[i];
        }
}