/*
 * shared.cc
 *
 *  Created on: Oct 19, 2012
 *      Author: manolis
 */


#include "shared.h"


/**
 * A /dev/null stream for suppressing debug information
 */
std::ostream Nullstream (0);


/**
 * Debug level set for the entire simulation
 */
unsigned int defaultDebugLevel = 0;
