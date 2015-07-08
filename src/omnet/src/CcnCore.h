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
#include "CcnInet.h"
#include "CcnPacket_m.h"





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

    unsigned int    debugLevel;

    /** Not allowed default Ctor  */
    CcnCore ();

    /** Look for identifier of suite in a name (if one is found it is removed from the name) */
    int extractSuiteFromName(INOUT char ** contentName, OUT char ** suiteStr);

public:

    ~CcnCore();

    CcnCore (Ccn *owner, const char *nodeName, const char *coreVer);


    /**
     * API: Ccn omnet module --> CCN Lite core
     */
    /** Some text info about the interface the module creates */
    std::string info ();

    /** @brief Update relay configuration (e.g. cache store size, cache policy, tx pace etc) */
    bool updateRelayConfig(void *);

    /** Add to cache a buffer of content chunks of equal size. Return the number of chunks successfully added to CS */
    int addToCacheFixedSizeChunks (const char *contentName, const int seqNumStart, const int numChunks, const char *chunkPtrs[], const int chunkSize);

    /** Add a FIB rule using L2 ID (MAC address). Return success/fail */
    bool addL2FwdRule (const char *contentName, const char *peerAddr, int localNetifIndex);

    /** Express Interest for named content. Return 1/true on success or 0/false on failure */
    bool requestContent (const char *contentName, const int seqNum);

    /** Pass packet received from the MAC layer to the relay. Return 1/true on success or 0/false on failure */
    bool fromMACFace (CcnContext *ccnCtx, int arrNetIf, CcnPacket *ccnPkt);



    /**
     * API: Ccn omnet module <-- CCN Lite core
     *
     * these methods are accessed by the CCN lite code through C wrappers in CcnCore.cc
     */
    /** Deliver named content chunks to the layer above (app or strategy). Return success(1)/failure(0) */
    bool deliverContent (char *contentName, int seqNum, void *data, int len);

    /** Prepare CCN packet from the relay for the MAC layer in omnet. Return success(1)/failure(0) */
    bool toMACFace (const char *dstAddr, const char *srcAddr, const char typeIorC, const char suite, const char *name, int chunk, void *data, int len);

    /** Set/clear a timer in omnet for the CCN core. Return >0 for timer handle (success), or <=0 for failure */
    long manageTimerEvent (char setOrReset, long usecOrHandle, void(*callback)(void *, void *), void *par1, void *par2);

};


#endif /* CCN_KORE_H_ */
