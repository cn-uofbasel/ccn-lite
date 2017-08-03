/*
 * @f ccnl-pkt-builder.c
 * @b CCN lite - packet builder
 *
 * Copyright (C) 2011-17, University of Basel
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
 */


#include "ccnl-pkt-builder.h"

#ifdef USE_SUITE_CCNB

int ccnb_isContent(unsigned char *buf, int len)
{
    int num, typ;

    if (len < 0 || ccnl_ccnb_dehead(&buf, &len, &num, &typ))
        return -1;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ)
        return 0;
    return 1;
}
#endif // USE_SUITE_CCNB

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNTLV

struct ccnx_tlvhdr_ccnx2015_s*
ccntlv_isHeader(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = (struct ccnx_tlvhdr_ccnx2015_s*)buf;

    if ((unsigned int)len < sizeof(struct ccnx_tlvhdr_ccnx2015_s)) {
        DEBUGMSG(ERROR, "ccntlv header not large enough\n");
        return NULL;
    }
    if (hp->version != CCNX_TLV_V1) {
        DEBUGMSG(ERROR, "ccntlv version %d not supported\n", hp->version);
        return NULL;
    }
    if (ntohs(hp->pktlen) < len) {
        DEBUGMSG(ERROR, "ccntlv packet too small (%d instead of %d bytes)\n",
                 ntohs(hp->pktlen), len);
        return NULL;
    }
    return hp;
}

int ccntlv_isData(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Data;
}

int ccntlv_isFragment(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Fragment;
}

#endif // USE_SUITE_CCNTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CISTLV

int cistlv_isData(unsigned char *buf, int len)
{
    struct cisco_tlvhdr_201501_s *hp = (struct cisco_tlvhdr_201501_s*)buf;
    unsigned short hdrlen, pktlen; // payloadlen;

    TRACEIN();

    if (len < (int) sizeof(struct cisco_tlvhdr_201501_s)) {
        DEBUGMSG(ERROR, "cistlv header not large enough");
        return -1;
    }
    hdrlen = hp->hlen; // ntohs(hp->hdrlen);
    pktlen = ntohs(hp->pktlen);
    //    payloadlen = ntohs(hp->payloadlen);

    if (hp->version != CISCO_TLV_V1) {
        DEBUGMSG(ERROR, "cistlv version %d not supported\n", hp->version);
        return -1;
    }

    if (pktlen < len) {
        DEBUGMSG(ERROR, "cistlv packet too small (%d instead of %d bytes)\n",
                 pktlen, len);
        return -1;
    }
    buf += hdrlen;
    len -= hdrlen;

    TRACEOUT();

    if(hp->pkttype == CISCO_PT_Content)
        return 1;
    else
        return 0;
}
#endif // USE_SUITE_CISTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_IOTTLV
// return 1 for Reply, 0 for Request, -1 if invalid
int iottlv_isReply(unsigned char *buf, int len)
{
    int enc = 1, suite;
    unsigned int typ;
    int vallen;

    while (!ccnl_switch_dehead(&buf, &len, &enc));
    suite = ccnl_enc2suite(enc);
    if (suite != CCNL_SUITE_IOTTLV)
        return -1;
    DEBUGMSG(DEBUG, "suite ok\n");
    if (len < 1 || ccnl_iottlv_dehead(&buf, &len, &typ, &vallen) < 0)
        return -1;
    DEBUGMSG(DEBUG, "typ=%d, len=%d\n", typ, vallen);
    if (typ == IOT_TLV_Reply)
        return 1;
    if (typ == IOT_TLV_Request)
        return 0;
    return -1;
}

int iottlv_isFragment(unsigned char *buf, int len)
{
    int enc;
    while (!ccnl_switch_dehead(&buf, &len, &enc));
    return ccnl_iottlv_peekType(buf, len) == IOT_TLV_Fragment;
}

#endif // USE_SUITE_IOTTLV

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------

ccnl_isContentFunc
ccnl_suite2isContentFunc(int suite)
{
    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        return &ccnb_isContent;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        return &ccntlv_isData;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        return &cistlv_isData;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        return &iottlv_isReply;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        return &ndntlv_isData;
#endif
    }

    DEBUGMSG(WARNING, "unknown suite %d in %s:%d\n",
                      suite, __func__, __LINE__);
    return NULL;
}

ccnl_isFragmentFunc
ccnl_suite2isFragmentFunc(int suite)
{
    switch(suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        return &ccntlv_isFragment;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        return &iottlv_isFragment;
#endif
    }

    DEBUGMSG(DEBUG, "unknown suite %d in %s of %s:%d\n",
                    suite, __func__, __FILE__, __LINE__);
    return NULL;
}
