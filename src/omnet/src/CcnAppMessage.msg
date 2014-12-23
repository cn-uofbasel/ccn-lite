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

packet ByteArrayMessage;


// 42 is the Content of the answer to the ultimate question of life.
// The ultimate question of life for those Interested is encoded as the reverse of its answer
//
enum CcnAppMessageType {
    CCN_APP_INTEREST=24;
    CCN_APP_CONTENT=42;
};


// Different possible control/data actions for the relay in regard to named content
// May need/want to exend that according to the control interface between app and relay
//
enum CcnRelayAction {
    CONTENT_REGISTER=421;
    CONTENT_UNREGISTER=422;
    CONTENT_DELIVER=423;
};


// This is a message exchanged between the CCN layer and the Layer above 
// (strategy or application). Can be extended to enable various signalling/data 
// operations between the two (see some options in the commented code)
//
packet CcnAppMessage extends ByteArrayMessage {
    int		type @enum(CcnAppMessageType);
    int		action @enum(CcnRelayAction);
    string 	contentName;
    int 	seqNr;		// for CCN_APP_INTEREST & CCN_APP_CONTENT, the chunk seq num;
    					// for CCN_APP_CONTENT registration, the start chunk of a range of chunks to be registered so that the cache is the content source 
    int		numChunks;	// for CCN_APP_CONTENT registration requests, the num of chunks to be registered so that the cache is the content source
    int 	chunkSize;	// for CCN_APP_CONTENT registration requests, the size of each chunk, serialised in the ByteArray

    // string dest;		//e.g. for installing redirection rules or FIB rules   
    // int timeOut; 	//e.g. for testing a timeout strategy
    // int expire;		//e.g. for setting a content to timeout
    // bool register;	//e.g. for register/unregister content 
};

