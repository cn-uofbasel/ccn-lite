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
    int debugLevel = defaultDebugLevel;
    DBGPRT(AUX, Detail, std::string(node) )
        << "-- ccn-lite kernel --: " << log; // << std::endl;
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
 * Encapsulate the CCN Lite C code base in a separate namespace.
 *****************************************************************************/

#include "./ccn-lite/ccnl-includes.h"  // platform includes common for ccn lite
namespace CcnLiteRelay {
#  include "./ccn-lite/ccn-lite-omnet.c"
};






/*****************************************************************************
 * Class CcnKore which provides access to the CCN core code
 *****************************************************************************/

/*****************************************************************************
 * Ctor
 */
CcnCore::CcnCore (Ccn *owner, const char *nodeName, const char *coreVer):
    relayName(nodeName),
    coreVersion(coreVer),
    ccnModule(owner),
    ctrlBlock(0)
{
    /* Set debug level first so that we can provide debug output
     */
    this->debugLevel = ::defaultDebugLevel;

    createRelay = &(CcnLiteRelay::ccnl_create_relay);
    destroyRelay = &(CcnLiteRelay::ccnl_destroy_relay);
    addContent = &(CcnLiteRelay::ccnl_add_content);
    addFwdRule = &(CcnLiteRelay::ccnl_add_fwdrule);
    app2Relay = &(CcnLiteRelay::ccnl_app2relay);
    lnk2Relay = &(CcnLiteRelay::ccnl_lnk2relay);

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
std::string
CcnCore::info ()
{
    std::string  text(relayName);

    text += " (" + coreVersion + ")";

    return text;
}


/*****************************************************************************
 * add a range of chunks from the same content to the cache
 */
int
CcnCore::addToCacheFixedSizeChunks(const char *contentName, const int seqNumStart, const int numChunks, const char *chunkPtrs[], const int chunkSize)
{
    int i = 0;
    int negcnt = 0;

    for (i = 0 ; i < numChunks ; i++)
    {
        // count failures (-1 returns)
        negcnt += this->addContent (ctrlBlock, contentName, i+seqNumStart, (void *) chunkPtrs[i], chunkSize);
    }

    if ((numChunks + negcnt) != 0)
        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
            << numChunks + negcnt << " only of " << numChunks << " were added to relay cache!"
            << std::endl;

    return numChunks + negcnt; // = numChunks - failures
};


/*****************************************************************************
 * add a forwarding rule based on a L2 ID (MAC) to relay's FIB
 */
bool
CcnCore::addL2FwdRule (const char *contentName, const char *dstAddr, int localNetifIndex)
{
    int res = this->addFwdRule (ctrlBlock, contentName, dstAddr, localNetifIndex);

    if ( res == -1) {
        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
            << "Installation of fwd rule " << contentName << "-->" << dstAddr
            << " at relay FIB, failed!"
            << std::endl;

        return false;
    }

    return true;
}



/*****************************************************************************
 * pass request for content to relay
 */
bool
CcnCore::requestContent(const char *contentName, const int seqNum)
{
    if ( this->app2Relay(ctrlBlock, contentName, seqNum) ==0 )
        return true;
    else {
        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
            << "Execution of content request " << contentName << "/" << seqNum
            << " failed!"
            << std::endl;

        return false;
    }
};


/*****************************************************************************
 * pass CCN packet received from the MAC layer to the relay
 */
bool
CcnCore::fromMACFace (const char *dstAddr, const char *srcAddr, int arrNetIf, void *data, int len)
{
    if (!dstAddr || !srcAddr || !len || !data)
    {
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "Incoming eth pkt was not delivered to CCN relay (endpoint information inconsistency)!"
            << std::endl;
        return false;
    }

    if ( this->lnk2Relay(ctrlBlock, dstAddr, srcAddr, arrNetIf, data, len) ==0)
        return true;

    return false;
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
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "Outgoing eth pkt dropped (endpoint information inconsistency)!"
            << std::endl;
        return false;
    }

    if ( len && !data) {
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "Outgoing eth pkt dropped (no packet buffer provided)!"
            <<std::endl;
        return false;
    }

    CcnContext *ctx = new CcnContext(AF_PACKET);

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

    DBGPRT(EVAUX, Info, ccnModule->getFullPath())
            << "\n  CCN Relay to-send data : " << pktInfo
            << "\n  CCN Preparing Transmission: " << txInfo
            << std::endl;

    ccnModule->toLowerLayer(pkt);

    return true;
};


/*****************************************************************************
 * register or delete a timer at owning Ccn module
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
        DBGPRT(AUX, Err, ccnModule->getFullName())
            << "Invalid timer operation (not a timer event registration or removal request)!"
            << std::endl;
        break;
    }

    return -1;
};


/*****************************************************************************
 * deliver content from the relay to the layer above (app)
 */
bool
CcnCore::deliverContent (char *contentName, int seqNum, void *data, int len)
{
    if ( !contentName ) {
        DBGPRT(EVAUX, Warn, ccnModule->getFullPath())
                << "Dropped content data, no associated content name/chunk identifier!"
                <<std::endl;
        return false;
    }

    if ( len && !data) {
        DBGPRT(AUX, Warn, ccnModule->getFullPath())
                << "Dropped content chunk with inconsistent content data buffer!"
                << std::endl;
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
 * Update relay configuration (e.g. cache store size, cache policy,
 * tx pace etc)
 */
bool
CcnCore::updateRelayConfig(void *)
{
    // TODO: Implement relay control block update/peek interface
    DBGPRT(EVAUX, Warn, ccnModule->getFullPath())
            << "UNDER CONSTRUCTION: function not implemented! \n"
            << "Cache store size in i-nodes can currently be set only at relay instantiation \n"
            << "Cache policy is fixed to FIFO \n"
            << "Tx pace can currently be set only at relay instantiation"
            << std::endl;
    return true;
};
