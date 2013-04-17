/*
 * shared.h
 *
 * Common defs and declarations for the CCN wrapper
 *
 *  Created on: Sep 15, 2012
 *      Author: manolis
 */

#ifndef CCN_SHARED_H_
#define CCN_SHARED_H_

#include <string>
#include <iostream>

#include <omnetpp.h>



//#define TEST1   // tests run solely for CcnInet

#define IN
#define OUT
#define INOUT

#define PROT_CCN                        42              // the answer to the ultimate question of life, the universe and everything..
#define IP_PROT_CCN                     PROT_CCN
#define ETHERTYPE_CCN                   PROT_CCN

#define CCNL_DEFAULT_CACHE_BYTESIZE     (500*1024*1024) // default of 500 MB cache memory per relay
#define CCNL_DUMMY_CONTENT_CHUNK_SIZE   500             // chunk size for dummy content
#define CCNL_MAX_TIMERS                 500             // used by Ccn module initialisation to set the maximum number of timers provided to CCN Lite

//#define MULTIPLE_CCNL_VERSIONS          // comment out if using only one version of CCN Lite and enable the desired one in CcnCore.cc
#define CCNL_DEFAULT_CORE                "CcnLite.v1"

#define CCNoIP_TTL                      -1  // if set to -1 the default value in IP is used
#define CCNoIP_TOS                      0   // if set to 0 no special handling applied over other traffic

//---
//#define B3C_TIMER                       101  // to identify timer events
//#define DELAY_TRANSMIT_EVENT            102
//#define LL_Q_RECORD                     103
//#define MIN_LLQ_POLL_INTVL              0.01 // sec
//#define MIN_INTERTRANSMISSION_TIME      0.01 // sec
//#define MIN_INTERTRANSMISSION_TIME      0.001 // sec
//#define MAX_B3C_TIMERS                  4000





/**
 * Debugging (sent at the console). It is not meant to replace or duplicate the EV channel information
 * but rather to prevent overhauling it with optional debugging info that can be turned off.
 */
extern std::ostream     Nullstream;
extern unsigned int     defaultDebugLevel;      // Note: every module may also define its own debug level

/* some log levels */
#define Detail  4 // detailed log
#define Err     3 // error
#define Warn    2 // requiring attention
#define Info    1 // informational (more than what is sent to EV)
#define None    0
#define CCNL_DEFAULT_DBG_LVL     Warn


#define DBG(t) (( (t <= debugLevel) ? std::cerr : Nullstream ) << "["<< __FILE__ << ":" << __LINE__ << ", " << __FUNCTION__<< "] " )







#endif /* CCN_SHARED_H_ */



