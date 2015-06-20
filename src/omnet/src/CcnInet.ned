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

package unibas.ccnlite;


simple CcnInet
{
    parameters:
        
        @display("i=block/fork");
        
        //string protocolMapping;
        int numUdpFaces = default(0);
        int basePortUdpFaces = default(0);
        
        double procDelay @unit("s") = default(0s);
        int debugLevel = default(1);      
        
        @class(CcnInet);
        
    gates:
        input ifIn[] @labels(Ieee802Ctrl/up, CcnPacket);
        output ifOut[] @labels(Ieee802Ctrl/down, CcnPacket);
      
        input udpIn @labels(UDPCommand/up);
        output udpOut @labels(UDPCommand/down);

        input upperIn @labels(CcnAppMessage);
        output upperOut @labels(CcnAppMessage);        
}
