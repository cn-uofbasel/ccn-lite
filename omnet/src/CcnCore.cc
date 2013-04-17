/*
 * CcnCore.cc
 *
 * C++ encapsulation of the C codebase of CCN Lite
 *
 *  Created on: Aug 11, 2012
 *      Author: manolis sifalakis
 */



#include "CcnCore.h"
#include "Ccn.h"
#include "CcnPacket_m.h"
#include "CcnAppMessage_m.h"



/*****************************************************************************
 * The CCN Lite code base is writen in C. The following platform functions are
 * needed to provide access for that code to the C++ based omnet environment
 *****************************************************************************/

extern "C"
double opp_time () { return simTime().dbl(); }


extern "C"
void opp_log (
    //IN int node_id,
    IN char *node,
    IN char *log )
{
    EV << "--- ccn-lite kernel log [" << node << "]: " << log; // << std::endl;
};


extern "C"
void opp_relay2app(
        void *module,
        char *name,
        int seqn,
        void *data,
        int len
        )
{
    CcnCore *mdl = static_cast <CcnCore *>(module);
    mdl->deliverContent(name, seqn, data, len);
    return;
}


extern "C"
void opp_relay2lnk(
        void *module,
        char *dst,
        char *src,
        void *data,
        int len
        )
{
    CcnCore *mdl = static_cast <CcnCore *>(module);
    mdl->toMACFace(dst, src, data, len);
    return;
}


extern "C"
long opp_timer (
        void *module,
        char set_reset,
        long usec_or_hdl,
        void (*callback)(void *, void*),
        void * param1,
        void * param2
        )
{
    CcnCore *mdl = static_cast <CcnCore *>(module);
    return mdl->manageTimerEvent (set_reset, usec_or_hdl, callback, param1, param2);
}



/*****************************************************************************
 * Encapsulate the various versions of the CCN Lite C code base in different
 * namespaces. Multi-version support could be used for compatibility testing
 * across CCN Lite versions and regression tests. The macro
 * #define MULTIPLE_CCNL_VERSIONS is in shared.h
 *****************************************************************************/


#ifdef MULTIPLE_CCNL_VERSIONS
# include "./ccn-lite-v1/ccnl-includes.h"  // platform includes common for all vers of ccn lite

  /* codebase of CCN Lite v0 (this is the initial ver we used with omnet 4.1)
   */
  namespace CcnLite_v0 {
        #include "./ccn-lite-v0/omnetglue.c"
  };

  /* codebase of CCN Lite v1 (this is the latest version by Christian)
   */
  namespace CcnLite_v1 {
        #include "./ccn-lite-v1/ccn-lite-omnet.c"
  };
#else // !MULTIPLE_CCNL_VERSIONS
# include "./ccn-lite/ccnl-includes.h"  // platform includes common for all vers of ccn lite

  namespace CcnLiteRelay {
        #include "./ccn-lite/ccn-lite-omnet.c"   // v0
  };
#endif // MULTIPLE_CCNL_VERSIONS





/*****************************************************************************
 * Class CcnKore which provides access to the CCN core code
 *****************************************************************************/

/*****************************************************************************
 * Ctor
 */
CcnCore::CcnCore (Ccn *owner, const char *nodeName, const char *coreName):
    relayName(nodeName),
    ccnModule(owner),
    ctrlBlock(0)
{
    /* Set debug level first so that we can provide debug output
     */
    this->debugLevel = ::defaultDebugLevel;

#ifdef MULTIPLE_CCNL_VERSIONS

    DBG(Info) << "Featuring multiple versions of CCN Lite code" << std::endl;

    /* decide which version of CCN Lite we are using
     */
    // TODO: re-implement this nicer
    if ( strcmp (coreName, "CcnLite.v0") == 0 )
    {
        createRelay = &(CcnLite_v0::ccnl_create_relay);
        destroyRelay = &(CcnLite_v0::ccnl_destroy_relay);
        addContent = &(CcnLite_v0::ccnl_add_content);
        addFwdRule = &(CcnLite_v0::ccnl_add_fwdrule);
        app2Relay = &(CcnLite_v0::ccnl_app2relay);
        lnk2Relay = &(CcnLite_v0::ccnl_lnk2relay);
    }
    else if ( strcmp (coreName, "CcnLite.v1") == 0 )
    {
        createRelay = &(CcnLite_v1::ccnl_create_relay);
        destroyRelay = &(CcnLite_v1::ccnl_destroy_relay);
        addContent = &(CcnLite_v1::ccnl_add_content);
        addFwdRule = &(CcnLite_v1::ccnl_add_fwdrule);
        app2Relay = &(CcnLite_v1::ccnl_app2relay);
        lnk2Relay = &(CcnLite_v1::ccnl_lnk2relay);

    };

    DBG(Info) << "Node " << nodeName << " uses CCN core: " << coreName << std::endl;
    EV << "Node " << nodeName << " uses CCN core: " << coreName << std::endl;

#else // !MULTIPLE_CCNL_VERSIONS

    createRelay = &(CcnLiteRelay::ccnl_create_relay);
    destroyRelay = &(CcnLiteRelay::ccnl_destroy_relay);
    addContent = &(CcnLiteRelay::ccnl_add_content);
    addFwdRule = &(CcnLiteRelay::ccnl_add_fwdrule);
    app2Relay = &(CcnLiteRelay::ccnl_app2relay);
    lnk2Relay = &(CcnLiteRelay::ccnl_lnk2relay);
#endif // MULTIPLE_CCNL_VERSIONS




    coreVersion = coreName;

    int numMacIds = ccnModule->numMacIds;
    char macAddrArray[numMacIds][6];

    for (int i = 0 ; i < numMacIds ; i++)
    {
        memset(macAddrArray[i], 0, 6);
        ccnModule->macIds[i].getAddressBytes(macAddrArray[i]);
    }

    ctrlBlock = createRelay(
            (void *) this,
            macAddrArray,
            numMacIds,
            ccnModule->getId(),
            ccnModule->getFullPath().c_str(),
            ccnModule->maxCacheBytes,
            ccnModule->maxCacheSlots,
            ccnModule->minInterTxTime
            );

    return;
};



/*****************************************************************************
 * Dtor
 */
CcnCore::~CcnCore ()
{
    destroyRelay(ctrlBlock);

    return;
}


/*****************************************************************************
 * text description of the object
 */
const char *
CcnCore::info ()
{
    std::string  text(relayName);

    text = relayName + " (" + coreVersion + ")";

    return text.c_str();
}


/*****************************************************************************
 * add a range of chunks from the same content to the cache
 */
int
CcnCore::addToCacheFixedSizeChunks(const char *contentName, const int seqNumStart, const int numChunks, const char *chunkPtrs[], const int chunkSize)
{
    int i = 0;

    for (i = 0 ; i < numChunks ; i++)
    {
        this->addContent (ctrlBlock, contentName, i+seqNumStart, (void *) chunkPtrs[i], chunkSize);
    }

    //TODO: fix this to return the truth
    DBG(Warn) << "UNDER CONSTRUCTION: Correct return value!" << std::endl;
    return i;
};


/*****************************************************************************
 * add a range of chunks from the same content to the cache
 */
int
CcnCore::addToCacheDummyContent (const char *contentName, const int seqNumStart, const int numChunks, const int chunkSize)
{
    char *oneChunkBuffer = new char[chunkSize];
    char **chunkPtrs = new char*[numChunks];

    for (int i = 0 ; i < numChunks ; i++)
        chunkPtrs[i] = oneChunkBuffer;

    this->addToCacheFixedSizeChunks (contentName, seqNumStart, numChunks, (const char **) chunkPtrs, chunkSize);

    //TODO: fix this to return the truth
    DBG(Warn) << "UNDER CONSTRUCTION: Correct return value!" << std::endl;
    return numChunks;
};


/*****************************************************************************
 * add a forwarding rule based on a L2 ID (MAC) to relay's FIB
 */
bool
CcnCore::addL2FwdRule (const char *contentName, const char *dstAddr, int localNetifIndex)
{
    this->addFwdRule (ctrlBlock, contentName, dstAddr, localNetifIndex);

    //TODO: fix this
    DBG(Warn) << "UNDER CONSTRUCTION: Correct return value!" << std::endl;
    return true;
}


/*****************************************************************************
 * add a forwarding rule based on a L3 ID (IP or socket) to relay's FIB
 */
bool
CcnCore::addL4FwdRule (const char *contentName, const char *dstAddr)
{
    // TODO:
    DBG(Warn) << "UNDER CONSTRUCTION: Method not implemented!" << std::endl;
    EV << "Functionality not implemented!" << std::endl;
    return true;
}

/*****************************************************************************
 * pass to relay request for content to create an Interest
 */
bool
CcnCore::requestContent(const char *contentName, const int seqNum)
{
    this->app2Relay(ctrlBlock, contentName, seqNum);

    //TODO: fix this
    DBG(Warn) << "UNDER CONSTRUCTION: Correct return value!" << std::endl;
    return true;
};


/*****************************************************************************
 * pass CCN packet from a UDP socket layer to the relay
 */
bool
CcnCore::fromMACFace (const char *dstAddr, const char *srcAddr, int arrNetIf, void *data, int len)
{
    if (!dstAddr || !srcAddr || !len || !data)
    {
        DBG(Err) << ccnModule->getFullName()
            << " - Consistency problem when attempting to pass data to the CCN core by the MAC layer. Ignore it "
            << std::endl;
        return false;
    }

    this->lnk2Relay(ctrlBlock, dstAddr, srcAddr, arrNetIf, data, len);

    return true;
};


/*****************************************************************************
 * pass CCN packet from a UDP socket layer to the relay
 */
bool
CcnCore::fromUDPFace (void)
{
    // TODO: Implement ...
    DBG(Warn) << "UNDER CONSTRUCTION: Method not implemented!" << std::endl;
    EV << "Functionality not implemented!" << std::endl;
    return true;
};


/*****************************************************************************
 * pass CCN packet from relay to MAC layer
 */
bool
CcnCore::toMACFace (const char *dstAddr, const char *srcAddr, void *data, int len)
{
    char srcAddrStr [18];
    char dstAddrStr [18];

    if ( !dstAddr || !srcAddr) {
        DBG(Warn) << ccnModule->getFullPath()
                << " - Endpoints for packet transmission not correctly specified. Aborting transmission."
                <<std::endl;
        return false;
    }

    if ( len && !data) {
        DBG(Warn) << ccnModule->getFullPath()
                << " - Packet buffer not provided for CCN packet transmission. Aborting transmission."
                <<std::endl;
        return false;
    }

    CcnContext *ctx = new CcnContext(AF_PACKET);

    // TODO: IF I BROADCAST THROUGH ALL MY INTERFACES, WOULD THAT BE CORRECT ?
    //const char bcastDst[] = "\xFF\xFF\xFF\xFF\xFF\xFF";
    //ctx->set802Info(bcastDst, dstAddr);
    ctx->set802Info(srcAddr, dstAddr);

    memset (srcAddrStr+17, '\0', sizeof(char));
    memset (dstAddrStr+17, '\0', sizeof(char));
    sprintf (srcAddrStr, "%2.2X-%2.2X-%2.2X-%2.2X-%2.2X-%2.2X",
            (unsigned char) srcAddr[0], (unsigned char) srcAddr[1], (unsigned char) srcAddr[2],
            (unsigned char) srcAddr[3], (unsigned char) srcAddr[4], (unsigned char) srcAddr[5]);
    sprintf (dstAddrStr, "%2.2X-%2.2X-%2.2X-%2.2X-%2.2X-%2.2X",
            (unsigned char) dstAddr[0], (unsigned char) dstAddr[1], (unsigned char) dstAddr[2],
            (unsigned char) dstAddr[3], (unsigned char) dstAddr[4], (unsigned char) dstAddr[5]);
    std::string pktInfo = " [" + std::string(srcAddrStr) + "] --ccn--> [" + std::string(dstAddrStr) + "] ";
    std::string txInfo = " [" + ctx->getSrcAddress802().str() + "] --ccn--> [" + ctx->getDstAddress802().str() + "] ";

    CcnPacket *pkt = new CcnPacket ();
    pkt->setName(pktInfo.c_str());
    pkt->setKind(PROT_CCN);
    pkt->setDataFromBuffer(data, len);
    pkt->addByteLength(len);
    pkt->setContextPointer(ctx);

    DBG(Info) << ccnModule->getFullPath()
            << "\n  CCN Core send data : " << pktInfo
            << "\n  CCN Preparing Transmission: " << txInfo
            << std::endl;

    ccnModule->toLowerLayer(pkt);

    return true;
};


/*****************************************************************************
 * pass CCN packet from relay to MAC layer
 */
bool
CcnCore::toUDPFace (void) {

    // TODO: Implement ...
    DBG(Warn) << "UNDER CONSTRUCTION: Method not implemented!" << std::endl;
    EV << "Functionality not implemented!" << std::endl;
    return true;
};


/*****************************************************************************
 * register or delete a timer with our owner Ccn module
 */
long
CcnCore::manageTimerEvent (char setOrReset, long usecOrHandle, void(*callback)(void *, void *), void * par1, void * par2)
{
    switch (setOrReset)
    {
    case 1:
        return ccnModule->setTimerEvent (usecOrHandle, callback, par1, par2); // usecOrHandle ~ usec

    case 0:
        return ccnModule->clearTimerEvent(usecOrHandle);    // usecOrHandle ~ handle

    default:
        DBG(Err) << ccnModule->getFullName()
            << " - Invalid timer operation (neither timer event registration nor removal)"
            << std::endl;
        break;
    }

    return -1;
};


/*****************************************************************************
 * deliver content from the relay to the layer above (app or strategy)
 */
bool
CcnCore::deliverContent (char *contentName, int seqNum, void *data, int len)
{
    if ( !contentName ) {
        DBG(Warn) << ccnModule->getFullPath()
                << " - Content without associated content name/chunk identifier. Drop data."
                <<std::endl;
        return false;
    }

    if ( len && !data) {
        DBG(Warn) << ccnModule->getFullPath()
                << " - Content chunk with inconsistent content data buffer. Drop data."
                <<std::endl;
        return false;
    }

    CcnAppMessage *appPkt = new CcnAppMessage;
    appPkt->setType(CCN_APP_CONTENT);
    appPkt->setAction(CONTENT_DELIVER);
    appPkt->setContentName(contentName);
    appPkt->setSeqNr(seqNum);
    appPkt->setDataFromBuffer(data, len);
    appPkt->addByteLength(len);
    appPkt->setArrivalTime(simTime());

    ccnModule->toUpperLayer(appPkt);

    return true;
};


/*****************************************************************************
 * Setter/Getter for the max cache memory size
 */
bool
CcnCore::setCacheSize(int)
{
    // TODO: Implement ...
    DBG(Warn) << "UNDER CONSTRUCTION: Method not implemented!" << std::endl;
    EV << "Functionality not implemented!" << std::endl;
    return true;
};

int
CcnCore::getCacheSize(void)
{
    // TODO: Implement ...
    DBG(Warn) << "UNDER CONSTRUCTION: Method not implemented!" << std::endl;
    EV << "Functionality not implemented!" << std::endl;
    return 0;
};


/*****************************************************************************
 * Setter/Getter for the packet inter-transmission pace
 */
bool
CcnCore::setTxPace (long)
{
    // TODO: Implement ...
    DBG(Warn) << "UNDER CONSTRUCTION: Method not implemented!" << std::endl;
    EV << "Functionality not implemented!" << std::endl;
    return true;
};

long
CcnCore::getTxPace(void)
{
    // TODO: Implement ...
    DBG(Warn) << "UNDER CONSTRUCTION: Method not implemented!" << std::endl;
    EV << "Functionality not implemented!" << std::endl;
    return 0;
};
