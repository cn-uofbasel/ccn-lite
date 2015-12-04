/*
 * CcnAdmin.cc
 *
 * Toolkit for parsing scenario files and configuring a ccn network
 *
 *  Created on: Sep 17, 2012
 *      Author: manolis
 */



#include <cstring>
#include <fstream>

#include "CcnAdmin.h"
#include "Parser.h"
#include "Ccn.h"


Define_Module(CcnAdmin);


/*****************************************************************************
 *
 * Class CcnAdmin
 *
 *****************************************************************************/


const char * const CcnAdmin::configType2Str[] = {"NA", "SCHED_INTEREST", "PRE_CACHE", "SET_FWD_RULE"};


/*****************************************************************************
 * Init omnet state
 */
void
CcnAdmin::initialize() {

    bool iSetGlobalDbgLvl = false;

    /* Immediately set my debug level (and the global default if not set)
     * so that I can use the DBGPRT() macro
     */
    cModule *netTopology = getParentModule();

    if (::defaultDebugLevel == 0) {
        if (netTopology->par("defaultDebugLevel").isSet())
            ::defaultDebugLevel = netTopology->par("defaultDebugLevel");
        else
            ::defaultDebugLevel = CCNL_DEFAULT_DBG_LVL;
        iSetGlobalDbgLvl = true;
    }

    debugLevel = (par("debugLevel").isSet()) ? par("debugLevel") : (::defaultDebugLevel) ;


    if ( iSetGlobalDbgLvl ) {
        DBGPRT(EVAUX, Info, this->getFullPath())
            << "Global debug level (for modules that do not have it set explicitly) is set to: "
            << ::defaultDebugLevel
            << std::endl;
    }

    DBGPRT(EVAUX, Info, this->getFullPath()) << " debug level set to " << debugLevel << std::endl;


    /* Reset initial state
     */
    registryStart = 0;
    registryEnd = 0;
    registrySize = 0;

    return;
};


/*****************************************************************************
 * D'tor
 */
CcnAdmin::~CcnAdmin()
{
    NodeInfo *ni;

    if (registrySize != 0) {
        while ( (ni = registryStart) ) {
            if (ni->configStart)
                deleteNodeConfig(ni);
            registryStart = ni->next;
            delete ni;
        }
    }

    return;
};


/*****************************************************************************
 * look up NodeInfo struct containing the configuration events, by the omnet
 * assigned id of the Ccn Module
 */
CcnAdmin::NodeInfo *
CcnAdmin::getNodeInfo(int id)
{
    NodeInfo *ni;

    if (!id) {
        DBGPRT(AUX, Warn, this->getFullPath())
                << " getNodeInfo(int ID) wall called with ID=0"
                << std::endl;
        return 0;
    }

    for (ni = registryStart ; ni ; ni = ni->next)
    {
        if (ni->nodeId == id)
            return ni;
    }

    return 0;
};


/*****************************************************************************
 * look up NodeInfo struct containing the configuration events, by the
 * Ccn object ptr
 */
CcnAdmin::NodeInfo *
CcnAdmin::getNodeInfo(cModule *module)
{
    NodeInfo *ni;

    if (!module) {
        DBGPRT(AUX, Warn, this->getFullPath())
                << " getNodeInfo(cModule *module) called with module=0"
                << std::endl;
        return 0;
    }

    for (ni = registryStart ; ni ; ni = ni->next)
    {
        if (ni->nodePtr == module)
            return ni;
    }

    return 0;
};


/*****************************************************************************
 * free configuration events for a node
 */
void
CcnAdmin::deleteNodeConfig(NodeInfo *node)
{
    ConfigRequest *top, *next;

    if (!node)
        return;

    top = node->configStart;

    while ( top ) {

        cancelAndDelete(top->event);
        next = top->next;
        delete top;
        top = next;

        node->numConfigEvents--;

        if (node->numConfigEvents < 0)
        {
            DBGPRT(AUX, Detail, this->getFullPath())
                    << " the counter of scenario configuration events for Ccn node " << node->nodeId
                    << " is zero, but I see ConfigRecords allocated!"
                    << std::endl;

            opp_error("Consistency problem encountered when freeing Ccn node's scenario configuration");
        }
    }

    node->configStart = node->configEnd = 0;

    return;
};


/*****************************************************************************
 * parse events for the CcnAdmin module (typically exp timers for config
 * operations)
 */
void
CcnAdmin::handleMessage (cMessage *msg)
{
    ConfigRequest   *config = 0;
    NodeInfo        *ni = 0;


    /* is timer ?
     */
    if (!msg->isSelfMessage()) {
        DBGPRT(AUX, Detail, this->getFullPath())
                << " Received message which is not a self-timer event! "
                << "\n\t  CcnAdmin module cannot receive messages (access via method calls only)."
                << std::endl;

        goto DISPOSE_AND_EXIT;
    }


    DBGPRT(AUX, Detail, this->getFullPath())
            << " Timed event for scenario configuration update "
            << "\n\t Message info:"
            << "\n\t   getName()=" << msg->getName()
            << "\n\t   getKind()=" << msg->getKind()
            << std::endl;


    /* is a scenario configuration event ?
     */
    if ( !(config = (ConfigRequest *) msg->getContextPointer()) )
    {
        DBGPRT(EVAUX, Warn, this->getFullPath())
                << " Scenario configuration update - no associated action! Ignore"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "\n\t struct ConfigRequest is missing! "
                << std::endl;

        goto DISPOSE_AND_EXIT;
    }


    /* are flags isExecuted and isScheduled correctly set ?
     */
    if ( ! (config->isScheduled && !config->isExecuted) )
    {
        DBGPRT(AUX, Detail, this->getFullPath())
                << " Scenario configuration update - bogus state!"
                << "\n\t Message state: isExecuted=" << config->isExecuted
                << "\n\t isScheduled=" << config->isScheduled
                << std::endl;

        opp_error("Bogus scenario configuration update request! %s has inconsistent state!", msg->getFullName() );   // should we really block ?
    }


    /* compare arrival time with execution schedule
     */
    if ( simTime() != config->execTime )
    {
        DBGPRT(EVAUX, Warn, this->getFullPath())
                << " Scenario configuration update - out of time!"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "\n\t Event schedule is for" << config->execTime
                << "\n\t Current time is " << simTime()
                << std::endl;

        if (simTime() < config->execTime)
        {
            scheduleAt(config->execTime, msg);
            DBGPRT(EVAUX, Info, this->getFullPath())
                    << "\n\t .. was too early! Rescheduled it"
                    << std::endl;
            return;
        } else
            goto DISPOSE_AND_EXIT;
    }


    /* check the ownership of the event
     */
    ni = getNodeInfo(config->nodeId);
    if ( !ni || !(check_and_cast<Ccn *>(ni->nodePtr)) )
    {
        DBGPRT(AUX, Detail, this->getFullPath())
                << " Scenario configuration update - cannot verify the refereed!"
                << "\n\t In registry getNodeInfo(" << config->nodeId << ")=" << ni
                << "\n\t (NodeInfo)->nodePtr does not match a valid Ccn module"
                << std::endl;

        opp_error("Received configuration update event but cannot verify its owner!");
    }


    /* select event handler
     */
    switch ( msg->getKind() )
    {
    case SEND_INTEREST:
        DBGPRT(EVAUX, Info, this->getFullPath())
                << " Start request for content (@time= "<< simTime() << "):"
                << "\n\t " << ni->nodePtr->getFullPath() << " I(" <<  config->namedData << ")"
                << "\n\t Info:"
                << "\n\t   startChunk=" << config->startChunk
                << "\n\t   numChunks=" << config->numChunks
                << std::endl;

        check_and_cast<Ccn *>(ni->nodePtr)->sendBatchInterests(config->namedData.c_str(), (const int) config->startChunk, config->numChunks);
        break;

    case PRE_CACHE:
        DBGPRT(EVAUX, Info, this->getFullPath())
                << " Load content in cache (@time= "<< simTime() << "):"
                << "\n\t " << ni->nodePtr->getFullPath() << " [@CS] <-- C(" <<  config->namedData << ")"
                << "\n\t Info:"
                << "\n\t   startChunk=" << config->startChunk
                << "\n\t   numChunks=" << config->numChunks
                << std::endl;

        check_and_cast<Ccn *>(ni->nodePtr)->addToCacheDummy(config->namedData.c_str(), (const int) config->startChunk, config->numChunks);
        break;

    case ADD_FWD_RULES:
    {

        /* For now we process MAC addr based FIB rules only
         */
        if (! config->nextHop->getSubmodule("mac") ) {
            DBGPRT(AUX, Detail, this->getFullPath())
                    << " Trying to add a FIB rule but node " << config->nextHop->getFullPath() << " has no 'mac' submodule (not IEtherMAC compatible)"
                    << "\n\t UNDER CONSTRUCTION: currently we only support MAC-based faces"
                    << std::endl;

            opp_error("Node module '%s' has no 'mac' submodule (not IEtherMAC compatible)", config->nextHop->getFullPath().c_str());
        }

        const char * dst = config->nextHop->getSubmodule("mac")->par("address").stringValue();
        int fromNetif = (config->accessFrom) ? config->accessFrom->getIndex() : -1;

        DBGPRT(EVAUX, Info, this->getFullPath())
                << " Install FIB rule (@time= "<< simTime() << "):"
                << "\n\t " << ni->nodePtr->getFullPath() << " [@FIB] <-- " <<  config->namedData
                << "\n\t Info:"
                << "\n\t   peer at " << dst
                << "\n\t   seen via local eth[" << fromNetif << "] (-1=BCAST)"
                << std::endl;

        check_and_cast<Ccn *>(ni->nodePtr)->addFwdRule(
                config->namedData.c_str(),
                Ccn::FaceT_ETHERNET,
                dst,
                fromNetif);

        break;
    }

    default:

        DBGPRT(AUX, Detail, this->getFullPath())
                << " Event handler not found! Ignore"
                << std::endl;

        goto DISPOSE_AND_EXIT;
    }



    /* flag as executed and remove reference to timer object
     */
    config->isExecuted = true;
    config->event = 0;


    DISPOSE_AND_EXIT:
    /* Dispose event
     *
     * NOTE: We do not delete the ConfigRequest because we will create
     * memory leaks and consistency problems at the node registry
     */
    delete msg;
    return;
};


/*****************************************************************************
 * add registry record for the configuration of a new CCN node
 */
bool
CcnAdmin::registerCcnNode (cModule *node, int nodeId)
{
    Enter_Method("CcnAdmin::registerCcnNode()");

    NodeInfo *ni = 0;

    if ( (ni = getNodeInfo(node)) || (ni = getNodeInfo(nodeId)) ) {

        if ( (ni->nodeId == nodeId) && (ni->nodePtr == node) ){
            DBGPRT(AUX, Warn, this->getFullPath())
                    << " Duplicate registration for Ccn node module " << nodeId << std::endl;
            return true;
        }

        DBGPRT(AUX, Err, this->getFullPath())
                << " Conflicting registration of Ccn node module with existing one"
                << "\n\t New registration (node:" << node << ", Id:" << nodeId << ")"
                << "\n\t Old registration (node:" << ni->nodePtr << ", Id:" << ni->nodeId << ")"
                << std::endl;

        opp_error("Already an entry in registry with different node ID or module address");
    }
    else {
        ni = new NodeInfo;
        ni->nodeId = nodeId;
        ni->next = 0;
        ni->prev = 0;
        ni->configStart = ni->configEnd = 0;
        ni->numConfigEvents = 0;
        ni->nodePtr = node;

        if ( registryStart == 0 ) {
            registryStart = registryEnd = ni;
        } else {
            registryEnd->next = ni;
            ni->prev = registryEnd;
            registryEnd = ni;
        }

        registrySize++;

        return true;
    }


    /**
     * NOTE!! We do not check if the information provided is consistent
     * in the context of the current simulation environment.
     * (Eg. that the module ID corresponds to the module pointer).
     *
     * We are thus free to use the module ID as a node ID, or else
     * an arbitrary ID
     */

    return false;
};






/*****************************************************************************
 * remove the registry record for a CCN node (we dispose its config info)
 */
bool
CcnAdmin::unRegisterCcnNode (cModule *node, int nodeId)
{
    Enter_Method("CcnAdmin::unRegisterCcnNode()");

    NodeInfo *ni = getNodeInfo(nodeId);

    if (!ni) {
        DBGPRT(AUX, Warn, this->getFullPath())
                << " Node " << nodeId << " not in CcnAdmin registry!"
                << std::endl;

        if ( (ni = getNodeInfo(node)) ) {
            DBGPRT(AUX, Err, this->getFullPath())
                    << " Consistency Problem ???"
                    << "\n\t Node " << nodeId << " could not be found in the CcnAdmin registry by ID but a lookup by module address was successful!!"
                    << std::endl;
        }
    }
    else {
        if (registrySize >1)
        {
            if (registryStart == ni) {
                ni->next->prev = 0;
                registryStart = ni->next;
            }
            else if (registryEnd == ni) {
                ni->prev->next = 0;
                registryEnd = ni->prev;
            }
            else {
                ni->prev->next = ni->next;
                ni->next->prev = ni->prev;
            }
        }
        else
            registryStart=registryEnd=0;

        registrySize--;

        deleteNodeConfig(ni);
        delete ni;

        return true;
    }

    return false;
};



/*****************************************************************************
 * go through the configuration record of a node in the registry and schedule
 * timers for configuration events in the course of the simulation
 */
bool
CcnAdmin::parseNodeConfig (cModule *node, const std::string &configFile)
{
    Enter_Method("CcnAdmin::parseNodeConfig()");

    NodeInfo        *ni = getNodeInfo(node);
    ConfigRequest   **newConfig;
    std::ifstream   file(configFile.c_str());
    std::string     line;
    int             countI = 0;
    int             lineno = 1;
    std::string     errMsg;
    ConfigRequest   *req_list_start = 0;
    ConfigRequest   *req_list_end = 0;


    /* Check that we have a node registered to associate with configuration
     */
    if (!ni) {
        DBGPRT(AUX, Err, this->getFullPath())
                << " Ccn Node " << node << " is not registered."
                << " Its scenario configuration file " << file
                << " is not parsed."
                << std::endl;
        return false;
    }

    /* place holder for the configuration
     */
    newConfig = &(ni->configStart);

    if (ni->numConfigEvents != 0)
    {
        DBGPRT(AUX, Warn, this->getFullPath())
                << " Ccn Node " << node
                << " already has "<< ni->numConfigEvents << " configuration events allocated."
                << "\n\t Will append also those from the scenario configuration file "
                << file << std::endl;

        while ( *newConfig ) {
            newConfig = &((*newConfig)->next);
        }
    }


    /* Open config file for reading.
     */
    if (!file.is_open()) {
        DBGPRT(AUX, Err, this->getFullPath())
                << " Cannot open scenario configuration file: " << configFile << std::endl;
        return false;
    } else {
        DBGPRT(EVAUX, Info, this->getFullPath())
                << " Reading scenario configuration file: " << configFile << std::endl;
    }


    /* Start line-by-line parsing
     */
    ConfigType mode = NOOP;

    while(file.good())  {
        getline(file,line);

        ParseInfo info(configFile, line);
        ParseIterator iter(info);

        iter.skipWhitespaces();

        if (iter.parseConstCharacter('['))  {

            /**
             * New section, identify it
             *
             * Section format: '[' <section> ']'
             * Currently: [ePreCacheMode], [eInterestMode], [eFwdRulesMode] and [eCommentsMode]
             */

            std::string sectionName;

            iter.skipWhitespaces();
            TEST_DO(iter.parseIdentifier(sectionName), errMsg = "EXPECTING SECTION NAME AFTER '['"; );
            iter.skipWhitespaces();
            TEST_DO(iter.parseConstCharacter(']'), errMsg = "EXPECTING ']' AFTER SECTION NAME"; );

            if (sectionName == "eInterestMode")  {
                mode = eInterestMode;
                DBGPRT(AUX, Info, this->getFullPath())
                        << " Parsing scenario file "
                        << configFile
                        << ": eInterestMode section"
                        << std::endl;
            }
            else if (sectionName == "ePreCacheMode")  {
                mode = ePreCacheMode;
                DBGPRT(AUX, Info, this->getFullPath())
                        << " Parsing scenario file "
                        << configFile
                        << ": ePreCacheMode section"
                        << std::endl;
            }
            else if (sectionName == "eFwdRulesMode")  {
                mode = eFwdRulesMode;
                DBGPRT(AUX, Info, this->getFullPath())
                        << " Parsing scenario file "
                        << configFile
                        << ": eFwdRulesMode section"
                        << std::endl;
            }
            else if (sectionName == "eCommentsMode")  {
                mode = eCommentsMode;
                DBGPRT(AUX, Info, this->getFullPath())
                        << " Parsing scenario file "
                        << configFile
                        << ": eCommentsMode section (Nothing to process)"
                        << std::endl;
            }
            else  {
                TEST_DO(false, errMsg = "UNKNOWN SECTION TYPE: '" + sectionName + "'"; );
            }
        }
        else if (iter.peekEnd())
        {
            /**
             * Empty line, ignore it
             */
        }
        else if (mode == eInterestMode)
        {
            /**
             * Process eInterestMode section lines
             *
             * Within each line parse '<key> = <val>' pairs separated by commas,
             * that provide the named data object and the attributes for this
             * section (i.e. time an Interest should be expressed).
             *
             * C style, C++ style and shell file style comments allowed (thanks
             * to Thomas' cool function !!)
             */

            std::string  contentName;
            int     startChunk = 0;
            int     noChunks = 0;
            double  contentReqTime;
            char    nameSet =0, startChunkSet=0, noChunksSet=0, timeSet=0;


            while ( !iter.peekEnd() )  {

                std::string qualifyerName;

                /* skip any ',' before the next key-val pair
                 */
                iter.skipWhitespaces();
                if (iter.parseConstCharacter(','))
                    continue;

                iter.skipWhitespaces();
                TEST_DO( iter.parseIdentifier(qualifyerName), errMsg = "EXPECTING QUALIFIER NAME AFTER ','"; );
                iter.skipWhitespaces();
                TEST_DO( iter.parseConstCharacter('='), errMsg = "EXPECTING '=' AFTER QUALIFIER NAME"; );
                iter.skipWhitespaces();

                if (qualifyerName == "ContentName")  {
                    TEST_DO( iter.parseIdentifier(contentName), errMsg = "EXPECTING A STRING AFTER 'ContentName =' "; );
                    nameSet = 1;
                }
                else if (qualifyerName == "StartChunk")  {
                    TEST_DO( iter.parseNumber(startChunk), errMsg = "EXPECTING NUMERIC AFTER 'StartChunk ='"; );
                    startChunkSet = 1;
                }
                else if (qualifyerName == "ChunksCount") {
                    TEST_DO( iter.parseNumber(noChunks), errMsg = "EXPECTING NUMERIC AFTER 'ChunksCount ='"; );
                    noChunksSet = 1;
                }
                else if (qualifyerName == "RequestTime")  {
                    TEST_DO( iter.parseNumber(contentReqTime), errMsg = "EXPECTING NUMERIC AFTER 'RequestTime ='"; );
                    timeSet = 1;
                }
                else  {
                    TEST_DO( false, errMsg = "UNKNOWN QUALIFIER TYPE: '" + qualifyerName + "'"; );
                }

                iter.skipWhitespaces();
            }


            /* Check that we have in one line named object and timed interest
             */
            if (!nameSet || !startChunkSet || !noChunksSet || !timeSet)
                TEST_DO( false, errMsg = "NOT ALL OF 'ContentName =<str>' AND 'StartChunk =<num>' AND 'ChunksCount =<num>' AND 'RequestTime =<num>' WERE SPECIFIED"; );


            /* .. almost done, create and chain up event record (ConfigRequest)
             */
            countI++;

            if (!req_list_start)
                req_list_end = req_list_start = new ConfigRequest;
            else {
                req_list_end->next = new ConfigRequest;
                req_list_end = req_list_end->next;
            }

            req_list_end->type = SEND_INTEREST;
            req_list_end->namedData = contentName;
            req_list_end->execTime = contentReqTime;
            req_list_end->startChunk = startChunk;
            req_list_end->numChunks = noChunks;
            req_list_end->next = (ConfigRequest *) NULL;
            req_list_end->isScheduled = false;
            req_list_end->isExecuted = false;
            req_list_end->nodeId = ni->nodeId;
            req_list_end->event = 0;

        }
        else if (mode == ePreCacheMode)
        {
            /**
             * ePreCacheMode section lines
             *
             * '<key> = <val>' pairs for content object chunk ranges and time
             * for content to added to cache
             */

            string  contentName;
            int     startChunk = 0;
            int     noChunks = 0;
            double  updateTime = 0;
            char    nameSet =0, startChunkSet=0, noChunksSet=0, updateTimeSet=0;

            while ( !iter.peekEnd() )  {

                string qualifyerName;

                /* skip any ',' before the next key-val pair
                 */
                iter.skipWhitespaces();
                if (iter.parseConstCharacter(','))
                    continue;

                iter.skipWhitespaces();
                TEST_DO( iter.parseIdentifier(qualifyerName), errMsg = "EXPECTING QUALIFIER NAME AFTER ','"; );
                iter.skipWhitespaces();
                TEST_DO( iter.parseConstCharacter('='), errMsg = "EXPECTING '=' AFTER QUALIFIER NAME"; );
                iter.skipWhitespaces();

                if (qualifyerName == "ContentName")  {
                    TEST_DO( iter.parseIdentifier(contentName), errMsg = "EXPECTING A STRING AFTER 'ContentName =' "; );
                    nameSet = 1;
                }
                else if (qualifyerName == "StartChunk")  {
                    TEST_DO( iter.parseNumber(startChunk), errMsg = "EXPECTING NUMERIC AFTER 'StartChunk ='"; );
                    startChunkSet = 1;
                }
                else if (qualifyerName == "ChunksCount") {
                    TEST_DO( iter.parseNumber(noChunks), errMsg = "EXPECTING NUMERIC AFTER 'ChunksCount ='"; );
                    noChunksSet = 1;
                }
                else if (qualifyerName == "UpdateTime") {
                    TEST_DO( iter.parseNumber(updateTime), errMsg = "EXPECTING NUMERIC AFTER 'UpdateTime ='"; );
                    updateTimeSet = 1;
                }
                else  {
                    TEST_DO( false, errMsg = "UNKNOWN QUALIFIER TYPE: '" + qualifyerName + "'"; );
                }

                iter.skipWhitespaces();
            }

            /* Check we have everything we need in one line (content name and chunk range)
             */
            if (!nameSet || !startChunkSet || !noChunksSet || !updateTimeSet)
                TEST_DO( false, errMsg = "NOT ALL OF 'ContentName =<str>' AND 'StartChunk =<num>' AND 'ChunksCount =<num>' AND 'UpdateTime =<num>' WERE SPECIFIED"; );


            /* .. almost done, create and chain up ConfigRequest
             */
            countI++;

            if (!req_list_start)
                req_list_end = req_list_start = new ConfigRequest;
            else {
                req_list_end->next = new ConfigRequest;
                req_list_end = req_list_end->next;
            }

            req_list_end->type = PRE_CACHE;
            req_list_end->namedData = contentName;
            req_list_end->execTime = updateTime;
            req_list_end->startChunk = startChunk;
            req_list_end->numChunks = noChunks;
            req_list_end->next = 0;
            req_list_end->isScheduled = false;
            req_list_end->isExecuted = false;
            req_list_end->nodeId = ni->nodeId;
            req_list_end->event = 0;
        }
        else if (mode == eFwdRulesMode)
        {

            /**
             * eFwdRulesMode section lines
             *
             * '<key> = <val>' pairs for name prefix and next hop neighbor
             */

            std::string  contentPrefix;
            std::string  nextHop;
            std::string  accessFrom;
            double  updateTime = 0;
            char    prefixSet =0, nextHopSet=0, accessFromSet=0, updateTimeSet=0;

            while ( !iter.peekEnd() )  {

                std::string qualifyerName;

                /* skip any ',' before the next key-val pair
                 */
                iter.skipWhitespaces();
                if (iter.parseConstCharacter(','))
                    continue;

                iter.skipWhitespaces();
                TEST_DO( iter.parseIdentifier(qualifyerName), errMsg = "EXPECTING QUALIFIER NAME AFTER ','"; );
                iter.skipWhitespaces();
                TEST_DO( iter.parseConstCharacter('='), errMsg = "EXPECTING '=' AFTER QUALIFIER NAME"; );
                iter.skipWhitespaces();

                if (qualifyerName == "ContentPrefix")  {
                    TEST_DO( iter.parseIdentifier(contentPrefix), errMsg = "EXPECTING A STRING AFTER 'ContentPrefix =' "; );
                    prefixSet = 1;
                }
                else if (qualifyerName == "NextHop")  {
                    TEST_DO( iter.parseIdentifierWithIndex(nextHop), errMsg = "EXPECTING A STRING (e.g. 'client1.linklayer[2]') AFTER 'NextHop =' "; );
                    nextHopSet = 1;
                }
                else if (qualifyerName == "AccessFrom")  {
                    TEST_DO( iter.parseIdentifierWithIndex(accessFrom), errMsg = "EXPECTING A STRING (e.g. 'client1.linklayer[2]') AFTER 'AccessFrom =' "; );
                    accessFromSet = 1;
                }
                else if (qualifyerName == "UpdateTime") {
                    TEST_DO( iter.parseNumber(updateTime), errMsg = "EXPECTING NUMERIC AFTER 'UpdateTime ='"; );
                    updateTimeSet = 1;
                }
                else  {
                    TEST_DO( false, errMsg = "UNKNOWN QUALIFIER TYPE: '" + qualifyerName + "'"; );
                }

                iter.skipWhitespaces();
            }

            /* Check we have everything we need in one line (prefix and next hop)
             */
            if (!prefixSet || !nextHopSet || !updateTimeSet)
                TEST_DO( false, errMsg = "NOT ALL OF 'ContentPrefix =<str>' AND 'NextHopId =<name>' AND 'UpdateTime =<num>' WERE SPECIFIED"; );

            if (!accessFromSet) {
                DBGPRT(AUX, Info, this->getFullPath())
                        << " AT LINE: " << lineno << " IN " << configFile
                        << " 'AccessFrom = ' NOT SET"
                        << "\n\t I MIGHT try to setup fwd rule for all node interfaces (activate broadcasting)"
                        << std::endl;
            }


            /* Find cModule ptrs for the given nextHop and accessFrom strings (expecting something like "client1.linklayer[0]")
             */
            cModule * topologyLevel = getParentModule();      // climb up to the network level of the hierarchy
            std::string accessFromRelpath = std::string(".") + accessFrom;
            std::string nextHopRelpath = std::string(".") + nextHop;
            cModule * localNetif = (accessFromSet) ? topologyLevel->getModuleByPath(accessFromRelpath.c_str()) : NULL;
            cModule * nextHopNetif = topologyLevel->getModuleByPath(nextHopRelpath.c_str());


            if (    (nextHopNetif == NULL) ||
                    ((accessFromSet) && (localNetif == NULL))  )
            {
                DBGPRT(EVAUX, Err, this->getFullPath())
                        << " AT LINE: " << lineno << " IN " << configFile
                        << ". Forgetting an invalid forwarding rule"
                        << std::endl;
                DBGPRT(AUX, Detail, this->getFullPath())
                        << "\n\t Node interface: " << ((nextHopNetif) ? accessFrom : nextHop ) << " does not exist in current topology."
                        << std::endl;

                continue;   // parse next line
            }
            else if ( ! (nextHopNetif->getSubmodule("mac")) )
            {
                DBGPRT(EVAUX, Err, this->getFullPath())
                        << " AT LINE: " << lineno << " IN " << configFile
                        << ". Forgetting an invalid forwarding rule"
                        << std::endl;
                DBGPRT(AUX, Detail, this->getFullPath())
                        << "\n\t Node " << nextHop << " does not have a MAC submodule."
                        << std::endl;

                continue;   // parse next line
            }


            /* .. almost done (chain up ConfigRequest records)
             */
            countI++;

            if (!req_list_start)
                req_list_end = req_list_start = new ConfigRequest;
            else {
                req_list_end->next = new ConfigRequest;
                req_list_end = req_list_end->next;
            }

            req_list_end->type = ADD_FWD_RULES;
            req_list_end->namedData = contentPrefix;
            req_list_end->execTime = updateTime;
            req_list_end->nextHop = nextHopNetif;
            req_list_end->accessFrom = localNetif;
            req_list_end->next = 0;
            req_list_end->isScheduled = false;
            req_list_end->isExecuted = false;
            req_list_end->nodeId = ni->nodeId;
            req_list_end->event = 0;
        }
        //else {}

        ++lineno;

    }       /* ---- End of line-by-line parsing ---- */


    *newConfig = req_list_start;
    ni->configEnd = req_list_end;
    ni->numConfigEvents += countI;

    DBGPRT(EVAUX, Info, this->getFullPath())
            << " Created " << countI  << " configuration events from scenario configuration file: "
            << configFile << std::endl;
    return true;


    FAILED:
    DBGPRT(EVAUX, Err, this->getFullPath())
            << " AT LINE: " << lineno << " IN " << configFile
            << "\n\t " << errMsg
            << std::endl;
    return false;
};




/*****************************************************************************
 * set timers for scheduling the scenario configuration updates of a node
 */
void
CcnAdmin::scheduleConfigEvents (cModule *node)
{
    Enter_Method("CcnAdmin::scheduleConfigEvents()");

    NodeInfo        *ni = getNodeInfo(node);
    ConfigRequest   *config=0;
    unsigned int    eventsCnt=0;

    /* Sanity checks
     */
    if (!ni) {
        DBGPRT(EVAUX, Err, this->getFullPath())
                << " Failed to schedule scenario configuration events!" << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "\n\t Ccn node ("<< node << ") not found in the registry"
                << std::endl;
        return;
    }

    if ( ! (config = ni->configStart) ) {
        DBGPRT(EVAUX, Info, this->getFullPath())
                << " No configuration events to be scheduled for node " << ni->nodePtr->getFullName()
                << std::endl;
        return;
    }



    /* Start planing ..
     */
    char desc[500];

    for ( int i=0 ; config ; i++, config = config->next) {

        sprintf(desc, " Scenario configuration event %d for %s ", i, ni->nodePtr->getFullPath().c_str());

        DBGPRT(AUX, Detail, this->getFullPath())
                << " Preparing to schedule" << desc
                << "\n\t Info:"
                << "\n\t   Config Request Type " << configType2Str[config->type]
                << "\n\t   Status: isScheduled=" << config->isScheduled << ", isExecuted=" << config->isExecuted
                << std::endl;


        /* Already scheduled or executed ?
         */
        if (config->isExecuted || config->isScheduled) {
            DBGPRT(EVAUX, Warn, this->getFullPath())
                    << desc << " - Marked already as scheduled or executed ..Not Re-scheduled!" << std::endl;

            continue;
        }


        /* Request timed in the past ?
         */
        if ( config->execTime < simTime().dbl() ) {
            DBGPRT(EVAUX, Warn, this->getFullPath())
                    << desc << " - Is in the past .. Not Scheduled!"
                    << std::endl;

            continue;
        }


        /* Prepare event and schedule it
         */
        cMessage *event = new cMessage(desc);
        event->setKind(config->type);
        event->setContextPointer(config);
        config->event = event;

        simtime_t when  = config->execTime;

        scheduleAt(when, event);

        eventsCnt++;

        config->isScheduled = true;

        DBGPRT(EVAUX, Info, this->getFullPath())
                << desc << " - Scheduled at " << when <<"!"
                << std::endl;
    }

    DBGPRT(EVAUX, Info, this->getFullPath())
            << " Scheduled in total " << eventsCnt
            << "configuration events for node "
            << ni->nodePtr->getFullName()
            << std::endl;

    return;
};

