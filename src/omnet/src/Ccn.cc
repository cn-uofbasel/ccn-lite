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
 * Class TimerList
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
 * Class Ccn
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

       this->minInterTxTime = (par("minTxPace").isSet()) ? par("minTxPace").doubleValue() : 0.0; // read in ms
       this->maxCacheSlots = (par("maxCacheSlots").isSet()) ? par("maxCacheSlots").longValue() : 0;
       this->maxCacheBytes = (par("maxCacheBytes").isSet()) ? par("maxCacheBytes").longValue() : CCNL_DEFAULT_CACHE_BYTESIZE;  // read in bytes
       this->nodeScenarioFile = (par("ccnScenarioFile").isSet()) ? par("ccnScenarioFile").stdstringValue() : "";
       this->verCore = (par("ccnCoreVersion").isSet()) ? par("ccnCoreVersion").stdstringValue() : CCNL_DEFAULT_CORE;

       DBGPRT(EVAUX, Info, this->getFullPath())
                << "\n (Init 0 completed) Read node configuration, from ini file:"
                << "\n\t  minTxPace=" << this->minInterTxTime << " ms"
                << "\n\t  maxCacheSlots=" << this->maxCacheSlots << " (0=disable caching)"
                << "\n\t  maxCacheBytes=" << this->maxCacheBytes << " bytes (default=" << CCNL_DEFAULT_CACHE_BYTESIZE << ")"
                << "\n\t  ccnScenarioFile=" << this->nodeScenarioFile
                << "\n\t  version of CCN core=" << this->verCore << " (default is " << CCNL_DEFAULT_CORE << ")"
                << std::endl;
    }
    else if(stage == 1)
    {
        /**
         * Stage 1 - internal state
         */

        cModule *mdlNetIf, *mdlNet, *mdlNode;
        std::string idstr(""), delim("\t");


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
            DBGPRT(AUX, Err, this->getFullPath())
                    << " Scenario administrator module (CcnAdmin) not found"
                    << std::endl;

            // block simulation
            opp_error("Cannot find a CCN Scenario Administrator module! Aborting simulation");
        }


        /* Initialise timer accounting
         */
        timerList = new TimerList(CCNL_MAX_TIMERS);
        if (!timerList) {
            DBGPRT(AUX, Err, this->getFullPath())
                    << " Allocation of a TimersList, for" << CCNL_MAX_TIMERS << " events failed."
                    << std::endl;

            // block simulation
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

        for (int i = 0 ; i < numMacIds; i++)
        {
            mdlNetIf = mdlNode->getSubmodule("eth",i);
            macIds[i].setAddress( mdlNetIf->getSubmodule("mac")->par("address") );
            DBGPRT(AUX, Info, this->getFullPath())
                << "MAC Addr eth[" << i << "] = " << macIds[i].str() << std::endl;

            idstr += macIds[i].str();
            idstr += delim;
        }

        DBGPRT(EVAUX, Info, this->getFullPath())
                << "\n (Init 1 completed) Internal node state:"
                << "\n\t  Timer list set to hold max " << timerList->size() << " timers"
                << "\n\t  Node configured with " << numMacIds << " MAC IDs: "
                << idstr << std::endl;
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
            opp_error("Registration of CCN node for the scenario failed. Abort simulation");
        else isRegistered = true;


        /* Parse scenario file for node and create schedule of configuration events
         *
         * NOTE this might be possible only after all nodes in the scenario have been instantiated
         * and registered so that fwd rules can be resolved and checked
         */
        if ( !scenarioAdmin->parseNodeConfig (this, nodeScenarioFile) )
        {
            // TODO: raise exception here ?
        }
        else {
            configuredScenario=true;
            scenarioAdmin->scheduleConfigEvents(this);
        }


        DBGPRT(EVAUX, Info, this->getFullPath())
                << "\n (Init 2 completed) Topology setup, and scenario config:"
                << "\n\t  Node registration with the CCN admin module - " << ((isRegistered) ? "Done" : "Failed")
                << "\n\t  Processing of scenario configuration - " << ((configuredScenario) ? "Done" : "Failed")
                << "\n\t  Init Ext. Logger - " << ((startedLogger) ? "Yes" : "No")
                << std::endl;

    }
    else if(stage == 3)
    {
        /**
         * Stage 3 - CCN-lite relay init
         */

        cModule *mdlNode = this->getParentModule();
        this->ccnCore = new CcnCore (this, mdlNode->getName(), this->verCore.c_str());

        if ( this->ccnCore ) {
            DBGPRT(EVAUX, Info, this->getFullPath())
                    << "\n (Init 3 completed) CCN Core init"
                    << "\n\t  Spawned a new CcnCore on " << ccnCore->info()
                    << std::endl;
        }
        else
        {
            delete this->ccnCore;
            DBGPRT(EVAUX, Info, this->getFullPath())
                    << "\n (Init 3 failed) CCN Core init"
                    << std::endl;

            opp_error(" CCN Core %s could not be initialized! ", this->getFullName());
        }
    }

    return;
};


/*****************************************************************************
 * Finish
 */
void
Ccn::finish()
{
    scenarioAdmin->unRegisterCcnNode (this, getId());

    DBGPRT(EVAUX, Info, this->getFullPath())
            << " Finishing completed..."
            << "\n\t unregistered myself at the CcnAdmin module"
            << std::endl;

    //CcnInet::callFinish ();

    return;
};



/*****************************************************************************
 * D'tor
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

    DBGPRT(EVAUX, Info, this->getFullPath())
            << " Demolishing completed ..."
            << "\n\t disposed " << count << " unserviced timers"
            << "\n\t terminated CCN Core"
            << std::endl;

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
            DBGPRT(AUX, Info, this->getFullPath())
                    << "CCN Relay timer expired for callback with"
                    << "\n\t arg1: " << cb->arg1
                    << "\n\t arg2: " << cb->arg2
                    << "\n\t .. at simu time (sec): " << simTime() << endl;

            (*(cb->pFunc))(cb->arg1, cb->arg2);
        }
        else {
            DBGPRT(AUX, Warn, this->getFullPath())
                    << "Timer for CCN Core expired - no callback object attached" << endl;
        }


        /* remove timer record from my accounting list
         */
        if (! timerList->remByObject(msg))
            DBGPRT(AUX, Warn, this->getFullPath())
            << "Timer for CCN Core expired - no record found in my timer list" << endl;

    }
    else
    {
        DBGPRT(AUX, Err, this->getFullPath())
                << "A Self-event was captured other than CCN Core timer: getName()="
                << msg->getName()
                << ". Ignoring it"
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
            DBGPRT(EVAUX, Warn, this->getFullPath())
                    << "CcnAppMessage|CCN_APP_INTEREST with unspecified content name. Dropping it"
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
                DBGPRT(EVAUX, Warn, this->getFullPath())
                        << "CcnAppMessage|CCN_APP_CONTENT declaring 0 num of chunks to register under name "
                        << c->getContentName()
                        << ". Dropping it" << endl;
                goto CLEANUP_AND_EXIT;
            }

            unsigned int    chunksBufferSize = numChunks * chunkSize;
            char            **chunksPtrs = new char*[numChunks];
            char            *chunksBuffer = new char[chunksBufferSize];

            if ( c->copyDataToBuffer(chunksBuffer, chunksBufferSize) != chunksBufferSize )
            {
                DBGPRT(EVAUX, Err, this->getFullPath())
                        << "CcnAppMessage|CCN_APP_CONTENT - "
                        << "Mismatch between actual size of content data buffer and the estimated one by getNumChunks() x getChunkSize()"
                        << ". Dropping it" << endl;
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
            DBGPRT(EVAUX, Err, this->getFullPath()) << "CcnAppMessage|CCN_APP_CONTENT requesting unknown action. Ignore it" << std::endl;
            DBGPRT(AUX, Detail, this->getFullPath()) << "CcnAppMessage|CCN_APP_CONTENT - UNDER CONSTRUCTION!" << std::endl;
            break;

        default:

            DBGPRT(EVAUX, Err, this->getFullPath()) << "CcnAppMessage|CCN_APP_CONTENT requesting unknown action. Ignore it" << std::endl;
            goto CLEANUP_AND_EXIT;
        }

    }
    else {
        DBGPRT(EVAUX, Err, this->getFullPath()) << "CcnAppMessage|??? (unknown type). Ignore it" << std::endl;
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
        DBGPRT(EVAUX, Err, this->getFullPath())
                << "Dropping incoming pkt from net - Not a valid CcnPacket"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "Recv non CcnPacket from the layer below"
                << "\n\t with getKind()=" << ccnPkt->getKind()
                << " (should have been PROT_CCN=" << PROT_CCN << ")"
                << std::endl;

        goto CLEANUP_AND_EXIT;
    }


    /* CcnContext check
     */
    ccnCtx = (CcnContext *) ccnPkt->getContextPointer();

    if (!ccnCtx ) {
        DBGPRT(EVAUX, Err, this->getFullPath())
                << "Dropping incoming pkt from net - Not a valid CcnPacket"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "Recv CcnPacket without a CcnContext attached."
                << std::endl;
        goto CLEANUP_AND_EXIT;
    }


    /* endpoint info; check if I m the true recipient
     */
    if ( ccnCtx->isAddressFamily(AF_PACKET) ) {

        /* look for match to one of the node-local addresses
         */
        for (int i = 0 ; i < numMacIds; i++) {
            if (macIds[i].compareTo( ccnCtx->getDstAddress802() ) == 0) {

                DBGPRT(EVAUX, Info, this->getFullPath()) << "Received " << std::string(ccnPkt->getName()) << std::endl;
                ccnCore->fromMACFace(ccnCtx, netIfIndexFromMac(ccnCtx->getDstAddress802()), ccnPkt);

                goto CLEANUP_AND_EXIT;
            }
        }

        DBGPRT(EVAUX, Warn, this->getFullPath())
                << "Dropping incoming pkt from net - Destination MAC addr: "
                << ccnCtx->getDstAddress802().str()
                << " is not locally configured"
                << std::endl;

    }
    else {
        DBGPRT(EVAUX, Warn, this->getFullPath())
                << "Dropping incoming pkt from net - Not AF_PACKET CcnContext"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "Currently we only support CCN Faces at the MAC level - saw CcnContext other than AF_PACKET"
                << std::endl;
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
    CcnPacket       *ccnPkt = dynamic_cast <CcnPacket *>(msg);

    DBGPRT(EVAUX, Info, this->getFullPath()) << "Sending " << std::string(ccnPkt->getName()) << std::endl;
    CcnInet::toLowerLayer(msg);
};


/*****************************************************************************
 * pass to INET
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
        DBGPRT(AUX, Err, this->getFullPath())
                << " Timer request but no callback has been provided. Ignoring."
                << std::endl;
        return 0;
    }

    if (usec < 0 ) {
        DBGPRT(AUX, Warn, this->getFullPath())
                << " Timer request planned in the past. Ignoring."
                << std::endl;
        return 0;
    }

    if (usec == 0) {
        DBGPRT(AUX, Info, this->getFullPath())
                << " Timer request planned immediately. Executing callback without setting a timer"
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
        DBGPRT(AUX, Info, this->getFullPath())
                << " Successfully scheduled timer at index handle " << handle
                << std::endl;

        return handle;
    }

    DBGPRT(AUX, Err, this->getFullPath())
            << " Attempt to create timer record failed. Timer list is full!"
            << "\n\t Try re-compiling after setting CCNL_MAX_TIMERS > " << CCNL_MAX_TIMERS
            << std::endl;

    opp_error ("Run out of timer objects !!!"); // should we really block the simulation here ?

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

    DBGPRT(EVAUX, Info, this->getFullPath())
            << " Adding dummy content in my cache: "
            << contentName << "/[" << seqNumStart << "-" << seqNumStart + numChunks << "]"
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

        DBGPRT(EVAUX, Info, this->getFullPath())
                << " Adding CCN FIB rule: "
                << contentName << " from " << nextHop.str() << " via eth[" << netifIndex << "]"
                << std::endl;

        return ccnCore->addL2FwdRule (contentName, addrBytes, netifIndex);
    }
    else // currently only MAC-based faces are supported
    {
        DBGPRT(EVAUX, Err, this->getFullPath())
                << " FIB rule does not involve a MAC based FACE. Ingoring it!" << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << " UNDER CONSTRUCTION: currently we only support MAC-based faces" << std::endl;
    }

    return false;
};


/*****************************************************************************
 * Send Interests in batch, for a range of chunks of a named content. This is
 * only a convenience method that is currently used by the CcnAdmin module
 */
bool
Ccn::sendBatchInterests(const char *contentName, int startChunk, int numChunks)
{
    /* the CcnAdmin module accesses this method atomically
     */
    Enter_Method("Ccn::sendBatchInterests()");

    /*
     * IMPORTANT NOTE: This method as is does not handle prefix requests. E.g. it
     * always assumes an exact match for a content exists. To handle prefix requests
     * (a prefix is specified and the longest prefix match object is returned) we 'd
     * need to have a convention for interpreting startChunk and numChunks when out
     * of bounds. Eg. if numChunks is -1 could imply that the contentName is a prefix
     * and not an object name to be exactly matched, and in this case the startChunk
     * may be considered undefined. Then depending on the case the CcnCore::requestContent()
     * would be called with according values.
     */

    for (int i=0 ; i< numChunks ; i++) {
        DBGPRT(EVAUX, Info, this->getFullPath())
                << "Sending Interest for "
                << contentName << " - c:" << startChunk + i
                << std::endl;

        if (! ccnCore->requestContent (contentName, startChunk + i) )
            DBGPRT(EVAUX, Warn, this->getFullPath())
                    << "Interest request for "
                    << contentName << " - c: " << startChunk + i
                    << ", failed!"
                    << std::endl;
    }

    return true;
};
