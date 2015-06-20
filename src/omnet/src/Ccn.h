/*
 * Ccn.h
 *
 * Main omnet module that integrates the CCN (Lite) code
 *
 *  Created on: Sep 14, 2012
 *      Author: manolis sifalakis
 */


#ifndef CCN_H_
#define CCN_H_


#include <omnetpp.h>

#include "shared.h"
#include "CcnAdmin.h"
#include "CcnCore.h"
#include "CcnInet.h"






/*************************************************************************************************
 * Timer accounting list for the ccn-lite kernel functions at each node
 *************************************************************************************************/

class TimerList
{
private:
    struct TimerRecord {
        unsigned long   handle;
        char            isFree;
        cMessage        *pTimer;
    };

    struct TimerRecord  *tList;
    unsigned long       tListSize;

    unsigned int        debugLevel;

    TimerList () {};    // not to be used

public:
    TimerList (unsigned long size);
    ~TimerList ();

    /** @brief Add accounting for a new timer. Return a handle to it or 0 if fail */
    unsigned long   add (cMessage *timer);

    /** @brief Pop a timer record out of the list. Return its address, or NULL if not found */
    cMessage *      popByHandle (unsigned long handle);

    /** @brief Pop next active timer record out of the list. Return its address, or NULL if none found */
    cMessage *      popNext ();

    /** @brief Remove a timer record from the list. Return 1 on success, 0 if timer not exist */
    int             remByObject (cMessage *timer);

    /** @brief Num of timer records the list can hold */
    unsigned long   size(void) {return tListSize;};
};







/*****************************************************************************
 * Class Ccn implements the stateful container between the omnet environment
 * and the CCN Lite core.
 *
 * Subclasses (extends) from class CcnInet which provides the INET specific
 * abstraction layer
 *
 * Contains instance of the class CcnCore, which provides the c++ interface
 * to the ccn-lite kernel
 *
 * Refers to instance of class CcnAdmin, the ex-machina "god" of the simulation
 * scenario
 *****************************************************************************/

class Ccn: public CcnInet
{
private:

    /**
     * Private state we maintain
     */
    TimerList       *timerList;             /* timer accounting service */
    std::string     nodeScenarioFile;       /* file storing the CCN scenario config for this node */
    std::string     verCore;                /* version of CCN core that this module is using */
    int             numMacIds;              /* num of locally available MAC addresses == num of eth NICs */

    /**
     * Actors we use
     */
    CcnCore         *ccnCore;               /* ccn core stub */
    CcnAdmin        *scenarioAdmin;         /* ccn administrator module*/

    /**
     * Callback object definitions (executed when timers expire)
     */
    typedef struct _Callback {
        void (*pFunc) (void *, void *);
        void * arg1;
        void * arg2;
    } Callback;



protected:

    friend class CcnAdmin;  // needs to be able to access the protected interface
    friend class CcnCore;   // needs to be able to access the protected state

    /**
     * Enum for distinguishing different types of faces
     */
    enum FaceType {
        FaceT_ETHERNET,
        FaceT_PPP,
        FaceT_SOCKET
    };


    /**
     * Protected state accessible to friends and subclasses
     */
    double          minInterTxTime;         /* for pacing I-pkt transmissions in milliseconds */
    int             maxCacheSlots;          /* CCN relay max cache entries */
    int             maxCacheBytes;          /* CCN relay cache memory size in bytes */
    MACAddress      *macIds;                /* table of node MAC addresses */



    /**
     * Initialization and termination of the module
     */
    int             numInitStages() const {return 4; };
    virtual void    initialize(int);
    virtual void    finish();



    /**
     *  Methods that provide an interface for the OMNET
     *  world towards the CCN relay.
     *
     *  They override the dummies provided in CcnInet
     */
    /** @brief Handle self messages (timers) */
    virtual void handleSelfMsg(cMessage* msg);

    /** @brief Message coming from the upper layers */
    virtual void fromUpperLayer(cMessage* msg);

    /** @brief Messages coming from the lower layers */
    virtual void fromLowerLayer(cMessage* msg);



    /**
     *  Methods that provide the interface for the CCN relay
     *  towards the OMNET world. In addition to the toUpperLayer()
     *  and toLowerLayer() methods of CcnInet, we enable timer support.
     */
    /** @brief Schedule a timer and a callback event. Returns a handle to the timer object,
     * 0 for event completion without timer allocation, and -1 on error */
    virtual long setTimerEvent (long usec, void(*callback)(void *, void *), void *par1, void *par2);

    /** @brief Delete timer. Return deleted timer handle, -1 on error, or 0 if timer did not exist */
    virtual long clearTimerEvent (long handle);

    /** Methods to mangle packets on their way to layers below or above */
    virtual void toLowerLayer(cMessage* msg);
    virtual void toUpperLayer(cMessage* msg);



    /**
     *  used by the CcnAdmin module
     */
    int     addToCacheDummy (const char *contentName, const int startChunk, const int numChunks);
    bool    sendBatchInterests (const char *contentName, int startChunk, int numChunks);
    bool    addFwdRule (const char *contentName, FaceType faceTp, const char *dst, int aux);


public:

    Ccn ():
        CcnInet(),
        timerList(NULL),
        numMacIds(0),
        ccnCore(NULL),
        scenarioAdmin(NULL),
        minInterTxTime(0),
        maxCacheSlots(0),
        maxCacheBytes(0),
        macIds(NULL)
        {};

    ~Ccn ();

};


#endif /* CCN_H_ */
