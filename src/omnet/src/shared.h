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
#include <assert.h>

#include <omnetpp.h>




#define IN
#define OUT
#define INOUT

#define PROT_CCN                        42              // the answer to the ultimate question of life, the universe and everything..
#define CCN_I                           43
#define CCN_C                           44

#define IP_PROT_CCN                     PROT_CCN
#define ETHERTYPE_CCN                   PROT_CCN

#define CCNL_DEFAULT_CACHE_BYTESIZE     (500*1024*1024) // default of 500 MB cache memory per relay
#define CCNL_DUMMY_CONTENT_CHUNK_SIZE   500             // chunk size for dummy content
#define CCNL_MAX_TIMERS                 500             // used by Ccn module initialisation to set the maximum number of timers provided to CCN Lite

#define CCNL_DEFAULT_CORE               "CCN-Lite v0.3.0"

#define CCNoIP_TTL                      -1  // if set to -1 the default value in IP is used
#define CCNoIP_TOS                      0   // if set to 0 no special handling applied over other traffic



/**
 * Debugging and output to EV and aux channel
 */
extern std::ostream     NullStream;
extern unsigned int     defaultDebugLevel;      // Note: every module may also define its own debug level


/* some log levels */
#define Detail 4 // detailed log
#define Info   3 // all activity
#define Warn   2 // requiring attention
#define Err    1 // errors
#define CCNL_DEFAULT_DBG_LVL    Info



#define DECORATE(who)   DebugOutput::StrIndexInSourceFile(__FILE__, __LINE__, __func__, (who))
#define DBGPRT(where,lvl,who)   ( ((lvl) <= (debugLevel)) ? \
        DebugOutput::Log( (where), rptLabel[ (lvl) ], DECORATE( (who) )) : \
        DebugOutput::Log( NONE, rptLabel[ (lvl) ], (who)) \
        )


extern const char rptLabel [][15];
enum  RptChannel {NONE, AUX, EVAUX};




class DebugOutput {
public:
    inline static
    DebugOutput &
    Init (std::ostream &auxChannel = NullStream) {
        if (!_theSingleton) {
            _theSingleton = new DebugOutput();
            _auxLogger = &auxChannel;
        }
        return *_theSingleton;
    };

    inline static
    std::string
    StrIndexInSourceFile (const char * file, const int lineno, const char * func, const std::string who = std::string("?"))
    {
        std::ostringstream os;
        assert (os);


        if ((*_auxLogger) != NullStream)
            os << "[" << file << ":" << lineno << ", " << func << "] <" << who << "> ";
        else
            os << " <" << who << "> ";

        return os.str();
    };

    inline static
    DebugOutput &
    Log (RptChannel where, const char *label, const std::string who)
    {
        _currChannel = where;

        switch (_currChannel) {
        case NONE:
            break;
        case EVAUX:
            EV << label << who;
        case AUX:
            (*_auxLogger) << label << who;
            break;
        }

        return *_theSingleton;
    };


    template<class T>
    DebugOutput &
    operator<<(const T& x)
    {
        switch (_currChannel) {
        case NONE:
            (*_nullStream) << x;
            break;
        case EVAUX:
            EV << x;
        case AUX:
            (*_auxLogger) << x;
            break;
        }

        return *_theSingleton;
    };

    inline
    DebugOutput &
    operator<<(std::ostream& (*f)(std::ostream&))
    {
        switch (_currChannel) {
        case NONE:
            (*_nullStream) << f;
            break;
        case EVAUX:
            EV << f;
        case AUX:
            (*_auxLogger) << f;
            break;
        }

        return *_theSingleton;
    };

protected:
    DebugOutput () {};
    DebugOutput (DebugOutput const &);
    ~DebugOutput ();

//private:
public:
    static RptChannel       _currChannel;
    static std::ostream *   _auxLogger;
    static std::ostream *   _nullStream;
    static DebugOutput *    _theSingleton;
};


#endif /* CCN_SHARED_H_ */
