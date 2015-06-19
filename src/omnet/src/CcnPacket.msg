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

cplusplus {{
#include "INETDefs.h"
#include "ByteArrayMessage.h"
}}

enum CcnPayloadType {
    MSG_TYPE_INTEREST = 0;
    MSG_TYPE_CONTENT = 1;
    MSG_TYPE_DATA = 1;
};

enum CcnSuiteType {
    SUITE_CCNx0 = 1;
    SUITE_CCNx1 = 2;
    SUITE_NDNx0 = 3;
};

// ByteArrayMessage is an INET defined type for dynamically varying packet
// sizes, which allows us to store in it the CCN packet
//
packet ByteArrayMessage;


//      The following is a container for the CCN packet handed by the CCNlite core.
//      One can place here additional extension header fields for different 
//      experiments (e.g. we use the CS-id of the cache that consumes an 
//      Interest in our experiments with transport strategies). 
//
packet CcnPacket extends ByteArrayMessage {
    byteLength = 6;		// Set initial size of packet upon construction to 
    					// automatically account for the extra fields added below.
    					// Thereafter when creating a new CcnPacket be carefull to 
    					// use the addByteLength() method instead of setByteLength()
    					// to correctly set the size of the packet!  
    
    long cacheId;
    char suiteType @enum(CcnSuiteType);
    char msgPayloadType @enum(CcnPayloadType);
}


