/*
 * Ccn.cc
 *
 * Main omnet module that integrates the CCN (Lite) code
 *
 *  Created on: Aug 11, 2012
 *      Author: manolis sifalakis
 */


#include "Ccn.h"
#include "CcnAppMessage_m.h"
#include "CcnPacket_m.h"


Define_Module(Ccn);






/*****************************************************************************
 *
 * Class Timers
 *
 *****************************************************************************/

/*****************************************************************************
 * create timer accounting list
 */
TimerList::TimerList (unsigned long size)
{
    /* Set debug level first so that we can provide debug output
     */
    this->debugLevel = ::defaultDebugLevel;


    this->tList = new TimerRecord[size+1];
    this->tListSize = size;

    /* we init size+1 records because we ll ingore 0th entry
     */
    for (unsigned long i=0 ; i<=size ; i++) {
        memset ((tList+i), 0, sizeof(TimerRecord));
        tList[i].isFree = 1;
    }

    tList[0].isFree = 0;   // we do not use 0th record or easier indexing
};


/*****************************************************************************
 * delete timer accounting list
 */
TimerList::~TimerList ()
{
    this->tListSize = 0;
    delete [] this->tList;
};


/*****************************************************************************
 * Add a timer record to next empty position
 */
unsigned long
TimerList::add (cMessage *timer)
{
    /* not using 0th position to make indexing simpler
     */
    for (unsigned long i=1 ; i<=tListSize ; i++) {
        if ( tList[i].pTimer && !(tList[i].isFree)) continue;

        tList[i].pTimer = timer;
        tList[i].isFree = 0;
        return i;
    };

    return 0;   // 0 means no free position found
};


/*****************************************************************************
 * Pop timer record by handle
 */
cMessage *
TimerList::popByHandle (unsigned long handle)
{
    cMessage    *timer;

    if (!handle)
        return NULL;

    timer = tList[handle].pTimer;
    tList[handle].pTimer = (cMessage *) NULL;
    tList[handle].isFree = 1;

    return timer;
};


/*****************************************************************************
 * Pop next timer record
 */
cMessage *
TimerList::popNext ()
{
    cMessage    *timer;

    /* pop next active timer record
     */
    for (unsigned long i=1 ; i<=tListSize ; i++) {
        if (tList[i].pTimer && !(tList[i].isFree))
        {
            timer = tList[i].pTimer;
            tList[i].pTimer = (cMessage *) NULL;
            tList[i].isFree = 1;

            return timer;
        }
    }

    return (cMessage *) NULL;
};


/*****************************************************************************
 * Pop next timer record
 */
int
TimerList::remByObject (cMessage *timer)
{
    if (!timer)
        return 0;

    /* not using 0th position to make indexing simpler
     */
    for (unsigned long i=1 ; i<=tListSize ; i++) {
        if (tList[i].pTimer == timer) {
            tList[i].pTimer = (cMessage *) NULL;
            tList[i].isFree = 1;
            return 1;
        }
    }

    return 0;   // timer not found
};





/*****************************************************************************
 *
 * Class Ccn
 *
 *****************************************************************************/

/*****************************************************************************
 * Module initialisation
 */
void Ccn::initialize(int stage)
{

    /* Multistage initialisation
     */
    if (stage == 0)
    {
        /**
         * Stage 0 - configuration params
         */


        /* Initialise base interface to INET
         */
       CcnInet::initialize();


        if (par("minTxPace").isSet()) {
            this->minInterTxTime = par("minTxPace").doubleValue();  // read unit in ms
        } else {
            this->minInterTxTime = 0.0; // disable pacing
            DBG(Info) << this->getFullPath()
                    << " - configuration parameter minTxPace not set. Disabling pacing"
                    << std::endl;
        }

        if (par("maxCacheSlots").isSet()) {
            this->maxCacheSlots = par("maxCacheSlots").longValue();
        } else {
            this->maxCacheSlots = 0;   // disable caching
            DBG(Info) << this->getFullPath()
                    << " - configuration parameter maxCacheSlots not set. Disabling caching"
                    << std::endl;
        }

        if ( par("maxCacheBytes").isSet() ) {
            this->maxCacheBytes = par("maxCacheBytes").longValue();  // read unit in bytes
        } else {
            this->maxCacheBytes = CCNL_DEFAULT_CACHE_BYTESIZE;
            DBG(Info) << this->getFullPath()
                    << " - configuration parameter maxCacheBytes not found. Hard-coded default "
                    << CCNL_DEFAULT_CACHE_BYTESIZE << " bytes will be used"
                    << std::endl;
        }

        if ( par("ccnScenarioFile").isSet() )
            this->nodeScenarioFile = par("ccnScenarioFile").stdstringValue();
        else {
            this->nodeScenarioFile = "";
            DBG(Info) << this->getFullPath()
                    << " - node does not have file specifying a scenario configuration"
                    << std::endl;
        }

        if ( par("ccnCoreVersion").isSet() )
            this->verCore = par("ccnCoreVersion").stdstringValue();
        else {
            this->verCore = CCNL_DEFAULT_CORE;
            DBG(Info) << this->getFullPath()
                    << " - CCN core version not set, will use the default " << CCNL_DEFAULT_CORE
                    << std::endl;
        }

        EV << this->getFullPath()
                << "(Init 0) Set node configuration:"
                << "\n  minTxPace=" << this->minInterTxTime << "ms"
                << "\n  maxCacheSlots=" << this->maxCacheSlots
                << "\n  maxCacheBytes=" << this->maxCacheBytes << "bytes"
                << "\n  ccnScenarioFile=" << this->nodeScenarioFile
                << "\n  version of CCN core=" << this->verCore
                << std::endl;
    }
    else if(stage == 1)
    {
        /**
         * Stage 1 - internal state
         */


        cModule *mdlNetIf, *mdlNet, *mdlNode;


        /* Look for the CcnAdmin module at the network level
         */
        mdlNet = this->getParentModule()->getParentModule();
        for (cModule::SubmoduleIterator iter(mdlNet); !iter.end(); iter++)
        {
            mdlNode = iter();

            if ( !scenarioAdmin )
                scenarioAdmin = dynamic_cast<CcnAdmin *>(mdlNode);

            if ( scenarioAdmin )
                break;
        }

        if ( scenarioAdmin == 0 ) {
            DBG(Err) << this->getFullPath()
                    << "No scenario administrator module (CcnAdmin) found in this network"
                    << std::endl;

            opp_error("Cannot find a CCN Scenario Administrator module! Aborting simulation");
        }


        /* Initialise timer accounting
         */
        timerList = new TimerList(CCNL_MAX_TIMERS);
        if (!timerList) {
            DBG(Err) << this->getFullPath()
                    << "Failed to allocate a TimersList object, for holding a default of"
                    << CCNL_MAX_TIMERS << " timers"
                    << std::endl;
            opp_error(" Timer list for module %s could not be initialized! ", this->getFullName());
        }


        /* Find my MAC address(es)
         */
        mdlNode = getParentModule();

        for (int i = 0 ; (mdlNetIf = mdlNode->getSubmodule("eth",i)) ; i++)
        {
            if ( mdlNetIf->getSubmodule("mac") )
                numMacIds ++;
        }

        macIds = new MACAddress[numMacIds];

        DBG(Info) << getFullPath()
                << " - " << numMacIds << " MAC addresses configured on node:" << std::endl;

        for (int i = 0 ; i < numMacIds; i++)
        {
            mdlNetIf = mdlNode->getSubmodule("eth",i);
            macIds[i].setAddress( mdlNetIf->getSubmodule("mac")->par("address") );
            DBG(Info) << "  macId of eth[" << i << "] = " << macIds[i].str() << std::endl;
        }


        EV << this->getFullPath()
                << "(Init 1) Internal node state:"
                << "\n  Timer list set to hold max " << timerList->size() << " timers"
                << "\n  Node configured with " << numMacIds << " MAC ids:"
                << std::endl;
        for (int i = 0 ; i < numMacIds; i++) {
            EV << "\t " << macIds[i].str() << std::endl;
        }

    }
    else if(stage == 2)
    {
        /**
         * Stage 2 - network state and scenario configuration
         *
         * The following tasks CANNOT be completed in an earlier stage
         * because all modules need to be fully instantiated
         */

        bool isRegistered=false, configuredScenario=false, startedLogger=false;


        /* Register me with the CcnAdmin module
         */
        if (! scenarioAdmin->registerCcnNode (this, this->getId()))
        {
            DBG(Detail) << getFullPath()
                    << "Ccn module failed to register itself -registerCcnNode()- with the CcnAdmin module!"
                    << std::endl;

            opp_error("Registration of CCN node for the scenario failed. Abort");  //TODO
        }
        else isRegistered = true;


        /* Parse scenario file for node and create schedule of configuration events
         *
         * NOTE this might be possible only after all nodes in the scenario have been instantiated
         * and registered so that fwd rules can be resolved and checked
         */
        if ( !scenarioAdmin->parseNodeConfig (this, nodeScenarioFile) )
        {
            EV << getFullPath()
                    << "Parsing of scenario configuration for CCN node failed, this can be critical"
                    << "\n  Simulation will proceed with vanilla state for node" << std::endl;

            // TODO: raise exception here ?
        }
        else {
            configuredScenario=true;
            scenarioAdmin->scheduleConfigEvents(this);
        }


        EV << this->getFullPath()
                << "(Init 2) Net state, and scenario config:"
                << "\n  Registered node with the CCN admin module - " << isRegistered
                << "\n  Processed scenario configuration - " << configuredScenario
                << "\n  Started Ext Logger - " << startedLogger
                << std::endl;

    }
    else if(stage == 3)
    {
        /**
         * Stage 3 - CCN-lite relay init
         */

        cModule *mdlNode = this->getParentModule();
        this->ccnCore = new CcnCore (this, mdlNode->getName(), this->verCore.c_str());

        EV << this->getFullPath()
                << "(Init 3) CCN Core init"
                << "\n   Spawned a new CcnCore " << ccnCore->info()
                << std::endl;
    }

    return;
};




/*****************************************************************************
 * Destroy
 */
Ccn::~Ccn()
{
    cMessage    *timer;
    long        count = 0;

    while ( (timer = timerList->popNext())) {
        count++;
        cancelAndDelete(timer);
    }

    delete timerList;
    delete ccnCore;

    scenarioAdmin->unRegisterCcnNode (this, getId());

    DBG(Info) << getFullName() << " finishing..."
            << "\n disposed " << count << " unserviced timers"
            << "\n terminated CCN core"
            << "\n unregistered myself at the CcnAdmin module"
            << std::endl;

    CcnInet::callFinish ();

    return;
};


/*****************************************************************************
 * Handle timers for CCN core
 */
void
Ccn::handleSelfMsg(cMessage* msg)
{
    Ccn::Callback   *cb = NULL;


    /* Service timer by invoking attached callback
     */
    if (    (msg->isSelfMessage()) &&
            (strcmp (msg->getName(), "CCN_CORE_TIMER") == 0) &&
            (msg->getTimestamp() >= simTime())
            )
    {
        cb = (Callback *) msg->getContextPointer();

        if (cb->pFunc)
        {
            DBG(Info) << getFullName()
                    << " - Timer for CCN Core callback will be executed"
                    << "\n   arg1: " << cb->arg1
                    << "\n   arg2: " << cb->arg2
                    << "\n   .. at simu time (sec): " << simTime() << endl;

            EV << getFullName() << " - CCN Core timer timed out executing callback" << std::endl;

            (*(cb->pFunc))(cb->arg1, cb->arg2);
        }
        else {
            DBG(Warn) << getFullName()
                << " - Timer for CCN Core expired but no callback object is attached"
                << endl;
        }


        /* remove timer record from my accounting list
         */
        if (! timerList->remByObject(msg)) {
            DBG(Err) << getFullName()
                    << " - WARNING: Timer for CCN Core was not recorded in my timer list"
                    << endl;
        }
    }
    else
    {
        DBG(Info) << getFullName()
                << " - Unknown self-triggered event occured, "
                << "getName()=" << msg->getName() << ". Ignoring it"
                << std::endl;
    }

    delete msg;

    return;
};



/*****************************************************************************
 * Handle CcnAppMessage received from layer above. Can be a strategy layer or
 * an application layer. Currently implements the minimum possible set of
 * functions. To extend the functions supported, one will also need to
 * extend the action definitions at CcnAppMessage.msg
 */
void
Ccn::fromUpperLayer(cMessage* msg) {

    if (!msg)
        return;

    CcnAppMessage *c = check_and_cast<CcnAppMessage *>(msg);


    if (c->getType() == CCN_APP_INTEREST) {

        if (strlen (c->getContentName()) == 0)
        {
            DBG(Info) << getFullName()
                    << " - Received CcnAppMessage with type CCN_APP_INTEREST from layer above, but with unspecified content name. Ignore it"
                    << std::endl;
            goto CLEANUP_AND_EXIT;
        }

        ccnCore->requestContent (c->getContentName(), c->getSeqNr());

    }
    else if (c->getType() == CCN_APP_CONTENT) {

        switch (c->getAction())
        {
        case CONTENT_REGISTER:
        {
            /* prepare content chunk pointers for en-mass registration
             */
            int numChunks = c->getNumChunks();
            int chunkSize = c->getChunkSize();

            if (!numChunks) {
                DBG(Info) << getFullName()
                        << " - Received CcnAppMessage of type CCN_APP_CONTENT from layer above, with 0 num of chunks to be registered."
                        << "\n   For named content " << c->getContentName()
                        << endl;
                goto CLEANUP_AND_EXIT;
            }

            unsigned int    chunksBufferSize = numChunks * chunkSize;
            char            **chunksPtrs = new char*[numChunks];
            char            *chunksBuffer = new char[chunksBufferSize];

            if ( c->copyDataToBuffer(chunksBuffer, chunksBufferSize) != chunksBufferSize )
            {
                DBG(Warn) << getFullName()
                        << " - In CcnAppMessage of type CCN_APP_CONTENT."
                        << "\n   Mispatch between the actual size of content data buffer and the estimated size by getNumChunks() x getChunkSize()"
                        << std::endl;
            }


            for (int i = 0 ; i < numChunks ; i++) {
                chunksPtrs[i] = chunksBuffer + (i * chunkSize);
            }

            /* push to cache and clean up
             */
            ccnCore->addToCacheFixedSizeChunks (c->getContentName(), c->getSeqNr(), numChunks, (const char **) chunksPtrs, chunkSize);

            delete chunksPtrs;
            delete chunksBuffer;

            break;
        }
        case CONTENT_UNREGISTER:

            /* TODO: The app can remove content content from the CS
             */
            DBG(Warn) << getFullName() << "UNDER CONSTRUCTION: Action not implemented!" << std::endl;

            break;

        default:

            DBG(Warn) << getFullName()
                << " - Received CcnAppMessage with type CCN_APP_CONTENT from layer above, but unknown action request. Ignore it"
                << std::endl;
            goto CLEANUP_AND_EXIT;
        }

    }
    else {
        DBG(Warn) << getFullName()
                << " - Received CcnAppMessage with unknown CcnAppMessageType (not CCN_APP_INTEREST not CCN_APP_CONTENT). Ignore it"
                << std::endl;
        goto CLEANUP_AND_EXIT;
    }


    CLEANUP_AND_EXIT:
    delete msg;
    return;
};


/*****************************************************************************
 * Handle CcnPacket received from layer below
 */
void
Ccn::fromLowerLayer(cMessage* msg) {

    CcnContext      *ccnCtx = 0;
    CcnPacket       *ccnPkt = dynamic_cast <CcnPacket *>(msg);


    if ( (!ccnPkt) || (ccnPkt->getKind() != PROT_CCN) )
    {
        DBG(Detail) << getFullName()
                << " - Received a non CcnPacket from the layer below"
                << "\n   Dynamic cast to CcnPacket returned " << ccnPkt
                << "\n   getKind()=" << ccnPkt->getKind() << "[PROT_CCN=" << PROT_CCN << "]"
                << std::endl;
        EV << getFullName() << " - Invalid CcnPacket from layer below. Ignore it" << std::endl;

        goto CLEANUP_AND_EXIT;
    }


    /* CcnContext check
     */
    ccnCtx = (CcnContext *) ccnPkt->getContextPointer();

    if (!ccnCtx ) {
        DBG(Warn) << getFullName()
                << " - Received CcnPacket without endpoint information (CcnContext) attached to it. Ignore it"
                << std::endl;
        EV << getFullName() << " - Inconsistent CccPacket from layer below. Ignore it" << std::endl;

        goto CLEANUP_AND_EXIT;
    }


    /* endpoint info; check if I m the true recipient
     */
    if ( ccnCtx->isAddressFamily(AF_PACKET) ) {

        /* look for match to one of the node-local addresses
         */
        for (int i = 0 ; i < numMacIds; i++) {
            if (macIds[i].compareTo( ccnCtx->getDstAddress802() ) == 0) {

                /* unpack data and pass them to CCN-lite relay
                 */
                int     buffSize    = ccnPkt->getByteArray().getDataArraySize();
                char    *dataBuff   = new char [buffSize];
                int     dataLen     = ccnPkt->copyDataToBuffer(dataBuff, buffSize);
                char    srcMAC[6];
                char    dstMAC[6];

                memset (srcMAC, 0, 6);
                memset (dstMAC, 0, 6);
                ccnCtx->getSrcAddress802().getAddressBytes(srcMAC);
                ccnCtx->getDstAddress802().getAddressBytes(dstMAC);

                ccnCore->fromMACFace(dstMAC, srcMAC, netIfIndexFromMac(ccnCtx->getDstAddress802()), dataBuff, dataLen);

                delete dataBuff;
                goto CLEANUP_AND_EXIT;
            }
        }

        DBG(Warn) << getFullName()
                << " - CcnPacket with endpoint information (CcnContext) that does not match local node configuration. Ignore it"
                << std::endl;
    }
    else {
        DBG(Detail) << getFullName()
                << " - Received CcnPacket with CcnContext type other than neither AF_PACKET"
                << "\n   Currently we only support CCN Faces at the MAC level"
                << std::endl;
        opp_error("Received CcnPacket with unknown CcnContext type attached to it");
    };


    CLEANUP_AND_EXIT:
    if (ccnCtx) delete ccnCtx;
    delete ccnPkt;

    return;
};




/*****************************************************************************
 *
 */
void
Ccn::toLowerLayer(cMessage* msg)
{
    CcnInet::toLowerLayer(msg);
};


/*****************************************************************************
 *
 */
void
Ccn::toUpperLayer(cMessage* msg)
{
    CcnInet::toUpperLayer(msg);
};




/*****************************************************************************
 * Set timer and register callback event
 */
long
Ccn::setTimerEvent (long usec, void(*callback)(void *, void *), void * par1, void * par2)
{
    if (!callback) {
        DBG(Warn) << getFullName()
                << " - Timer requested but no callback has been provided. Ignore request."
                << std::endl;
        return 0;
    }

    if (usec < 0 ) {
        DBG(Err) << getFullName()
                << " - Timer planned in the past. Nothing to be done. Ignore request"
                << std::endl;
        return 0;
    }

    if (usec == 0) {
        EV << getFullName()
                << " - Executing callback immediately without setting a timer"
                << std::endl;
        (*callback) (par1, par2);
        return 0;
    }


    Callback * cb = new Callback;
    cb->pFunc = callback;
    cb->arg1 = par1;
    cb->arg2 = par2;

    double      usec2sec = ((double) usec / 1000000);
    simtime_t   expirationTime = simTime() + usec2sec;

    cMessage *timer = new cMessage ("CCN_CORE_TIMER");
    timer->setTimestamp(expirationTime);
    timer->setContextPointer((void *) cb);

    long handle = timerList->add(timer);

    if (handle) {
        scheduleAt(expirationTime, timer);
        EV << getFullName()
                << " - Successfully scheduled timer at position " << handle
                << std::endl;

        return handle;
    }

    DBG(Detail) << getFullName()
            << " - Attempt to create timer record failed. List is full!"
            << "\n  Try re-compiling after setting CCNL_MAX_TIMERS > " << CCNL_MAX_TIMERS
            << std::endl;

    opp_error ("Run out of timer objects !!!");
    //EV << getFullName () << "Timer list full !!!" << std::endl;

    delete cb;
    delete timer;
    return -1;
}



/*****************************************************************************
 * Delete timer
 */
long
Ccn::clearTimerEvent (long handle)
{
    cMessage *timer = timerList->popByHandle(handle);

    if (timer) {
        Callback *cb = (Callback *) timer->getContextPointer();
        delete cb;
        cancelAndDelete(timer);
        return handle;
    }

    return 0;
};


/*****************************************************************************
 * Create and add a range of dummy chunks representing a named content to the
 * CS of the relay
 */
int
Ccn::addToCacheDummy (const char *contentName, const int seqNumStart, const int numChunks)
{
    /* the CcnAdmin module accesses this method
     */
    Enter_Method("Ccn::addToCacheDummy()");

    char *oneChunkBuffer = new char[CCNL_DUMMY_CONTENT_CHUNK_SIZE];
    char **chunkPtrs = new char*[numChunks];

    for (int i = 0 ; i < numChunks ; i++)
        chunkPtrs[i] = oneChunkBuffer;

    DBG(Info) << getFullPath()
            << " - Adding dummy content to CS: "
            << contentName << " [" << seqNumStart << "-" << seqNumStart + numChunks << "]"
            << std::endl;

    return ccnCore->addToCacheFixedSizeChunks (contentName, seqNumStart, numChunks, (const char **) chunkPtrs, CCNL_DUMMY_CONTENT_CHUNK_SIZE);
};


/*****************************************************************************
 * Set a forwarding rule in the FIB of the relay. Depending on the face type
 * the respective next hop endpoint type is installed on the relay node's FIB.
 */
bool
Ccn::addFwdRule(const char *contentName, FaceType faceTp, const char *dst, int aux)
{
    /* the CcnAdmin module accesses this method
     */
    Enter_Method("Ccn::addFwdRule()");

    if (!dst || !contentName)
        return false;

    if (faceTp == FaceT_ETHERNET)       // Layer 2: MAC address endpoint
    {
        int netifIndex = aux;
        MACAddress nextHop(dst);

        char addrBytes[6];
        memset(addrBytes, 0, 6);

        nextHop.getAddressBytes(addrBytes);

        DBG(Info) << getFullPath()
                << " - Adding CCN FIB rule: "
                << contentName
                << " at " << dst
                << " via eth[" << netifIndex
                << std::endl;

        return ccnCore->addL2FwdRule (contentName, addrBytes, netifIndex);
    }
    else // currently only MAC-based faces are supported
    {
        DBG(Warn) << getFullName() << "UNDER CONSTRUCTION: currently only MAC-based faces are supported" << std::endl;
        EV << "FIB rule that involves a non-MAC based FACE. Ingoring it!" << std::endl;
    }

    return false;
};


/*****************************************************************************
 * Send Interests in batch, for a range of chunks of a named content. This is
 * only a convinience method that is currently used by the CcnAdmin module
 */
bool
Ccn::sendBatchInterests(const char *contentName, int startChunk, int numChunks)
{
    /* the CcnAdmin module accesses this method atomically
     */
    Enter_Method("Ccn::sendBatchInterests()");


    DBG(Info) << getFullPath()
            << " - Sending batch Interests for " << contentName
            << " [" << startChunk << "-" << startChunk + numChunks << "]"
            << std::endl;

    for (int i=0 ; i< numChunks ; i++)
        ccnCore->requestContent (contentName, startChunk + i);

    return true;
};

