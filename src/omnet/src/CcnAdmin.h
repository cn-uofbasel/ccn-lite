/*
 * CcnAdmin.h
 *
 * Toolkit for parsing scenario files and configuring a ccn network
 *
 *  Created on: Sep 14, 2012
 *      Author: manolis sifalakis
 */

#ifndef CCN_ADMIN_H_
#define CCN_ADMIN_H_


#include <string>
#include "shared.h"




/*************************************************************************************************
 * Class CcnAdmin performs the configuration and administration of the CCN scenarios
 *
 * - Parses the scenario config file and stores the settings
 * - Configures the CCN nodes at init time with the pre-run setup
 * - At runtime it acts "Dark City" style transparently altering the state of the CCN
 *   network to reflect transient conditions
 *************************************************************************************************/



class CcnAdmin: public cSimpleModule
{
private:

    /**
     * Various possible scenario configuration options
     */

    enum ConfigType {   // left is the section name in the config file, right is resp. config cmd
        eCommentsMode = 0, NOOP=0,
        eInterestMode = 1, SEND_INTEREST = 1,
        ePreCacheMode = 2, PRE_CACHE=2,
        eFwdRulesMode = 3, ADD_FWD_RULES=3
    };

    static const char * const configType2Str[];


    /**
     * Generic scenario configuration event data
     */
    typedef struct _ConfigRequest {
        _ConfigRequest  *next;

        int             nodeId;         // owner of the request (Ccn module ID)
        bool            isScheduled;    // whether a configuration event has been scheduled for execution
        bool            isExecuted;     // whether a configuration event has been executed
        cMessage        *event;         // ptr to event that will trigger the config action

        ConfigType      type;           // ConfigType enum value
        std::string     namedData;      // Named content (prefix)
        double          execTime;       // time of execution

        union {
            struct {  // FIB rule (ADD_FWD_RULE)
                cModule *nextHop;
                cModule *accessFrom;
            };

            struct {  // Add to cache content (PRE_CACHE) and Interest request (SEND_INTEREST)
                int     startChunk;     // starting chunk in PRE_CACHE request or I-request
                int     numChunks;      // number of chunks in PRE_CACHE request or I-request
            };
        };
    } ConfigRequest;





    /**
     * Registry of all the CCN modules in the omnet environment
     */
    typedef struct _NodeInfo {
        struct _NodeInfo    *next;
        struct _NodeInfo    *prev;
        int                 nodeId;         // getId() of Ccn module (NOT the node compound module); can also be arbitrary
        cModule             *nodePtr;       // ptr of Ccn module (NOT the node compound module)
        ConfigRequest       *configStart;
        ConfigRequest       *configEnd;
        int                 numConfigEvents;
    } NodeInfo;

    NodeInfo        *registryStart, *registryEnd;
    int             registrySize;



    int         debugLevel;



    /**
     * Utility functions for the Ccn Modules registry
     */
    NodeInfo *  getNodeInfo(int id);
    NodeInfo *  getNodeInfo(cModule *module);
    void        deleteNodeConfig(NodeInfo *node);
    void        handleMessage (cMessage *msg);



public:

    CcnAdmin ():
        cSimpleModule(),
        registryStart(NULL),
        registryEnd(NULL),
        registrySize(0),
        debugLevel(0)
    {return;};

    ~CcnAdmin ();


    /**
     * Initialization and termination of the module
     */
    virtual void    initialize();
    virtual void    finish() {return; };


    /**
     * Exported service functions (for direct method invocation by other modules)
     */
    bool registerCcnNode (cModule *node, int nodeId);
    bool unRegisterCcnNode (cModule *node, int nodeId);
    bool parseNodeConfig (cModule *node, const std::string &file);
    void scheduleConfigEvents (cModule *node);

};




#endif /* CCN_ADMIN_H_ */
