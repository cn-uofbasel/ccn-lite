/*
 * CcnCore.h
 *
 * Generic c++ stub for the CCN lite core.
 *
 * The wrapper is written such that multiple versions (e.g. different versions of CCN lite or other
 * such stub) can be accommodated seamlessly in the same experimental setup. (If things won't
 * work when using different versions in an experiment chances are that the different versions are
 * shamefully incompatible!).
 *
 *  Created on: Aug 11, 2012
 *      Author: manolis sifalakis
 */


#ifndef CCN_KORE_H_
#define CCN_KORE_H_



#include <string.h>
#include "shared.h"


typedef void    (*API4DestroyRelay) (void *);
typedef void *  (*API4CreateRelay)  (void *, const char [][6], int, int, const char *, int, int, int);
typedef int     (*API4AddContent)   (void *, const char *, int, void *, int);
typedef int     (*API4AddFwdRule)   (void *, const char *, const char *, int);
typedef int     (*API4App2Relay)    (void *, const char *, int);
typedef int     (*API4Lnk2Relay)    (void *, const char *, const char *, int, void *, int);





/* ***********************************************************************************************
 * Class CcnKore provides access to the CCN protocol and relay code
 * ***********************************************************************************************/

class Ccn;

class CcnCore
{
private:

    std::string     relayName;
    std::string     coreVersion;
    Ccn             *ccnModule;
    void            *ctrlBlock;


    /**
     * Function ptrs to the right core's API
     */
    API4CreateRelay     createRelay;
    API4DestroyRelay    destroyRelay;
    API4AddContent      addContent;
    API4AddFwdRule      addFwdRule;
    API4App2Relay       app2Relay;
    API4Lnk2Relay       lnk2Relay;


    unsigned int    debugLevel;

    /**
     * Not allowed default Ctor
     */
    CcnCore ();


public:

    ~CcnCore();

    CcnCore (Ccn *owner, const char *nodeName, const char *coreVer);


    /**
     * API: Ccn module --> CCN Lite core
     */

    /** Some text info about the interface the module creates */
    std::string info ();

    /** @brief Update relay configuration (e.g. cache store size, cache policy, tx pace etc) */
    bool updateRelayConfig(void *);

    /** Add to cache a buffer of content chunks of equal size. Return the number of chunks successfully added to CS */
    int addToCacheFixedSizeChunks (const char *contentName, const int seqNumStart, const int numChunks, const char *chunkPtrs[], const int chunkSize);

    /** Add a FIB rule using L2 ID (MAC address). Return success/fail */
    bool addL2FwdRule (const char *contentName, const char *dstAddr, int localNetifIndex);

    /** Express Interest for named content. Return 1/true on success or 0/false on failure */
    bool requestContent (const char *contentName, const int seqNum);

    /** Pass packet received from the MAC layer to the relay. Return 1/true on success or 0/false on failure */
    bool fromMACFace (const char *dstAddr, const char *srcAddr, int arrNetIf, void *data, int len);





    /**
     * API: Ccn module <-- CCN Lite core
     */
    /** Deliver named content chunks to the layer above (app or strategy). Return success(1)/failure(0) */
    bool deliverContent (char *contentName, int seqNum, void *data, int len);

    /** Prepare CCN packet from the relay for the MAC layer in omnet. Return success(1)/failure(0) */
    bool toMACFace (const char *dstAddr, const char *srcAddr, void *data, int len);

    /** Set/clear a timer in omnet for the CCN core. Return >0 for timer handle (success), or <=0 for failure */
    long manageTimerEvent (char setOrReset, long usecOrHandle, void(*callback)(void *, void *), void *par1, void *par2);



    // TODO: delete only for tests
    void printMsg (const char *text) {EV << text << std::endl; };
};


#endif /* CCN_KORE_H_ */
