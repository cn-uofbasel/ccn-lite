/*
 * @f ccnl-pkt-util.c
 *
 * Copyright (C) 2011-15, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2017-06-20 created
 */

#ifndef CCNL_LINUXKERNEL
#include "ccnl-pkt-util.h"
#include "ccnl-defs.h"
#include "ccnl-os-time.h"
#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-cistlv.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-switch.h"
#include "ccnl-logging.h"
#else
#include <ccnl-pkt-util.h>
#include <ccnl-defs.h>
#include <ccnl-os-time.h>
#include <ccnl-pkt-ccnb.h>
#include <ccnl-pkt-ccntlv.h>
#include <ccnl-pkt-cistlv.h>
#include <ccnl-pkt-ndntlv.h>
#include <ccnl-pkt-switch.h>
#include <ccnl-logging.h>
#endif


int
ccnl_str2suite(char *cp)
{
    if (!cp)
        return -1;
#ifdef USE_SUITE_CCNB
    if (!strcmp(cp, CONSTSTR("ccnb")))
        return CCNL_SUITE_CCNB;
#endif
#ifdef USE_SUITE_CCNTLV
    if (!strcmp(cp, CONSTSTR("ccnx2015")))
        return CCNL_SUITE_CCNTLV;
#endif
#ifdef USE_SUITE_CISTLV
    if (!strcmp(cp, CONSTSTR("cisco2015")))
        return CCNL_SUITE_CISTLV;
#endif
#ifdef USE_SUITE_LOCALRPC
    if (!strcmp(cp, CONSTSTR("localrpc")))
        return CCNL_SUITE_LOCALRPC;
#endif
#ifdef USE_SUITE_NDNTLV
    if (!strcmp(cp, CONSTSTR("ndn2013")))
        return CCNL_SUITE_NDNTLV;
#endif
    return -1;
}

const char*
ccnl_suite2str(int suite)
{
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        return CONSTSTR("ccnb");
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        return CONSTSTR("ccnx2015");
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        return CONSTSTR("cisco2015");
#endif
#ifdef USE_SUITE_LOCALRPC
    if (suite == CCNL_SUITE_LOCALRPC)
        return CONSTSTR("localrpc");
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        return CONSTSTR("ndn2013");
#endif
    return CONSTSTR("?");
}

int
ccnl_suite2defaultPort(int suite)
{
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        return CCN_UDP_PORT;
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        return CCN_UDP_PORT;
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        return CCN_UDP_PORT;
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        return NDN_UDP_PORT;
#endif
    return NDN_UDP_PORT;
}

bool
ccnl_isSuite(int suite)
{
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        return true;
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        return true;
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        return true;
#endif
#ifdef USE_SUITE_LOCALRPC
    if (suite == CCNL_SUITE_LOCALRPC)
        return true;
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        return true;
#endif
    return false;
}

int
ccnl_pkt2suite(unsigned char *data, int len, int *skip)
{
    int enc, suite = -1;
    unsigned char *olddata = data;

    if (skip)
        *skip = 0;

    if (len <= 0)
        return -1;

    DEBUGMSG_CUTL(TRACE, "pkt2suite %d %d\n", data[0], data[1]);

    while (!ccnl_switch_dehead(&data, &len, &enc))
        suite = ccnl_enc2suite(enc);
    if (skip)
        *skip = data - olddata;
    if (suite >= 0)
        return suite;

#ifdef USE_SUITE_CCNB
    if (*data == 0x04)
        return CCNL_SUITE_CCNB;
    if (*data == 0x01 && len > 1 && // check for CCNx2015 and Cisco collision:
                                (data[1] != 0x00 && // interest
                                 data[1] != 0x01 && // data
                                 data[1] != 0x02 && // interestReturn
                                 data[1] != 0x03))  // fragment
        return CCNL_SUITE_CCNB;
#endif

#ifdef USE_SUITE_CCNTLV
    if (data[0] == CCNX_TLV_V1 && len > 1) {
        if (data[1] == CCNX_PT_Interest ||
            data[1] == CCNX_PT_Data ||
            data[1] == CCNX_PT_Fragment ||
            data[1] == CCNX_PT_NACK)
            return CCNL_SUITE_CCNTLV;
    }
#endif

#ifdef USE_SUITE_CISTLV
    if (data[0] == CISCO_TLV_V1 && len > 1) {
        if (data[1] == CISCO_PT_Interest ||
            data[1] == CISCO_PT_Content ||
            data[1] == CISCO_PT_Nack)
            return CCNL_SUITE_CISTLV;
    }
#endif

#ifdef USE_SUITE_NDNTLV
    if (*data == NDN_TLV_Interest || *data == NDN_TLV_Data ||
        *data == NDN_TLV_Fragment)
        return CCNL_SUITE_NDNTLV;
#endif

/*
#ifdef USE_SUITE_LOCALRPC
        if (*data == LRPC_PT_REQUEST || *data == LRPC_PT_REPLY)
            return CCNL_SUITE_LOCALRPC;
#endif
    }
*/
    return -1;
}

int
ccnl_cmp2int(unsigned char *cmp, int cmplen)
{
    long int i;
    char *str = (char *)ccnl_malloc(cmplen+1);
    DEBUGMSG(DEBUG, "  inter a: %i\n", cmplen);
    DEBUGMSG(DEBUG, "  inter b\n");
    memcpy(str, (char *)cmp, cmplen);
    str[cmplen] = '\0';
    DEBUGMSG(DEBUG, "  inter c: %s\n", str);
    i = strtol(str, NULL, 0);
    DEBUGMSG(DEBUG, "  inter d\n");
    ccnl_free(str);
    return (int)i;
}
