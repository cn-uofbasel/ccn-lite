/*
 * shared.cc
 *
 *  Created on: Oct 19, 2012
 *      Author: manolis
 */


#include "shared.h"



/**
 * Debug level set for the entire simulation
 */
unsigned int defaultDebugLevel = 0;

std::ostream    NullStream (0); // a bufferless ostream for emulating /dev/null as a stream
std::ostream *  DebugOutput::_auxLogger = &NullStream;
std::ostream *  DebugOutput::_nullStream = &NullStream;
RptChannel      DebugOutput::_currChannel= NONE;
DebugOutput *   DebugOutput::_theSingleton = 0;

const char rptLabel [][15] = {
        "NONE: ",
        "ERROR: ",
        "WARN: ",
        "INFO: ",
        "DETAIL: ",
};
