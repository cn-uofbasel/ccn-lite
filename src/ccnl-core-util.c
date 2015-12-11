/*
 * @f ccnl-core-util.c
 * @b CCN lite, common utility procedures (used by utils as well as relays)
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
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
 * 2014-06-18 created
 */

//#ifndef CCNL_CORE_UTIL_H
//#define CCNL_CORE_UTIL_H

#include "ccnl-core.h"

// ----------------------------------------------------------------------
// collect the USE_* macros in a string

#ifdef CCNL_ARDUINO
const char compile_string[] PROGMEM = ""
#else
const char *compile_string = ""
#endif

#ifdef USE_CCNxDIGEST
        "CCNxDIGEST, "
#endif
#ifdef USE_DEBUG
        "DEBUG, "
#endif
#ifdef USE_DEBUG_MALLOC
        "DEBUG_MALLOC, "
#endif
#ifdef USE_ECHO
        "ECHO, "
#endif
#ifdef USE_ETHERNET
        "ETHERNET, "
#endif
#ifdef USE_FRAG
        "FRAG, "
#endif
#ifdef USE_HMAC256
        "HMAC256, "
#endif
#ifdef USE_HTTP_STATUS
        "HTTP_STATUS, "
#endif
#ifdef USE_KITE
        "KITE, "
#endif
#ifdef USE_LOGGING
        "LOGGING, "
#endif
#ifdef USE_MGMT
        "MGMT, "
#endif
#ifdef USE_NACK
        "NACK, "
#endif
#ifdef USE_NFN
        "NFN, "
#endif
#ifdef USE_NFN_MONITOR
        "NFN_MONITOR, "
#endif
#ifdef USE_NFN_NSTRANS
        "NFN_NSTRANS, "
#endif
#ifdef USE_SCHEDULER
        "SCHEDULER, "
#endif
#ifdef USE_SIGNATURES
        "SIGNATURES, "
#endif
#ifdef USE_SUITE_CCNB
        "SUITE_CCNB, "
#endif
#ifdef USE_SUITE_CCNTLV
        "SUITE_CCNTLV, "
#endif
#ifdef USE_SUITE_CISTLV
        "SUITE_CISTLV, "
#endif
#ifdef USE_SUITE_IOTTLV
        "SUITE_IOTTLV, "
#endif
#ifdef USE_SUITE_LOCALRPC
        "SUITE_LOCALRPC, "
#endif
#ifdef USE_SUITE_NDNTLV
        "SUITE_NDNTLV, "
#endif
#ifdef USE_UNIXSOCKET
        "UNIXSOCKET, "
#endif
        ;

// ----------------------------------------------------------------------

int
ccnl_is_local_addr(sockunion *su)
{
    if (!su)
        return 0;
#ifdef USE_UNIXSOCKET
    if (su->sa.sa_family == AF_UNIX)
        return -1;
#endif
#ifdef USE_IPV4
    if (su->sa.sa_family == AF_INET)
        return su->ip4.sin_addr.s_addr == htonl(0x7f000001);
#endif
#ifdef USE_IPV6
    if (su->sa.sa_family == AF_INET6) {
        static const unsigned char loopback_bytes[] =
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        static const unsigned char mapped_ipv4_loopback[] =
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1 };
        return ((memcmp(su->ip6.sin6_addr.s6_addr, loopback_bytes, 16) == 0) ||
                (memcmp(su->ip6.sin6_addr.s6_addr, mapped_ipv4_loopback, 16) == 0));
    }

#endif
    return 0;
}

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
#ifdef USE_SUITE_IOTTLV
    if (!strcmp(cp, CONSTSTR("iot2014")))
        return CCNL_SUITE_IOTTLV;
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
#ifdef USE_SUITE_IOTTLV
    if (suite == CCNL_SUITE_IOTTLV)
        return CONSTSTR("iot2014");
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
#ifdef USE_SUITE_IOTTLV
    if (suite == CCNL_SUITE_IOTTLV)
        return NDN_UDP_PORT;
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
#ifdef USE_SUITE_IOTTLV
    if (suite == CCNL_SUITE_IOTTLV)
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

// ----------------------------------------------------------------------

typedef int (*ccnl_isContentFunc)(unsigned char*, int);
typedef int (*ccnl_isFragmentFunc)(unsigned char*, int);

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
#endif


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
#endif


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
#endif


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
#endif

#ifdef USE_SUITE_NDNTLV
int ndntlv_isData(unsigned char *buf, int len)
{
    unsigned int typ, vallen;

    if (len < 0 || ccnl_ndntlv_dehead(&buf, &len, &typ, &vallen))
        return -1;
    return typ == NDN_TLV_Data || typ == NDN_TLV_NDNLP ? 1 : 0;
}
#endif


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

// ----------------------------------------------------------------------

int
ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src, int srclen)
{
    int len = 0;

    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: {
        unsigned short *sp = (unsigned short*) dst;
        *sp++ = htons(CCNX_TLV_N_NameSegment);
        len = srclen;
        *sp++ = htons(len);
        memcpy(sp, src, len);
        len += 2*sizeof(unsigned short);
        break;
    }
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV: {
        unsigned short *sp = (unsigned short*) dst;
        *sp++ = htons(CISCO_TLV_NameComponent);
        len = srclen;
        *sp++ = htons(len);
        memcpy(sp, src, len);
        len += 2*sizeof(unsigned short);
        break;
    }
#endif
    default:
        len = srclen;
        memcpy(dst, src, len);
        break;
    }

    return len;
}

int
ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf)
{
    int len = strlen(src);

    DEBUGMSG_CUTL(TRACE, "ccnl_pkt_prependComponent(%d, %s)\n", suite, src);

    if (*offset < len)
        return -1;
    memcpy(buf + *offset - len, src, len);
    *offset -= len;

#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV) {
        unsigned short *sp = (unsigned short*) (buf + *offset) - 1;
        if (*offset < 4)
            return -1;
        *sp-- = htons(len);
        *sp = htons(CCNX_TLV_N_NameSegment);
        len += 2*sizeof(unsigned short);
        *offset -= 2*sizeof(unsigned short);
    }
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV) {
        unsigned short *sp = (unsigned short*) (buf + *offset) - 1;
        if (*offset < 4)
            return -1;
        *sp-- = htons(len);
        *sp = htons(CISCO_TLV_NameComponent);
        len += 2*sizeof(unsigned short);
        *offset -= 2*sizeof(unsigned short);
    }
#endif

    return len;
}

// ----------------------------------------------------------------------

// FIXME!
// This method currently does not correctly distinguish between CCNTLV and
// CISTLV. The reason is that this method only looks at the first two unsigned
// chars. However, CCNTLV and CISTLV both have many overlapping TLV definitions,
// starting by CCNX_TLV_V1 == CISCO_TLV_V1. In the current implementation CCNTLV
// is almost always picked whether the packet is CCNTLV or CISTLV.
// This is obviously a problem and should be fixed. However fixing this is not
// quite straight-forward because it means that this method has to look at more
// data than the two first chars. Ideally this method should try parsing the
// packet in each suite's encoding and return the (hopefully) remaining one.
int
ccnl_pkt2suite(unsigned char *data, int len, int *skip)
{
    int enc = -1, suite = -1;
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
    if (*data == NDN_TLV_Interest || *data == NDN_TLV_Data
                                  || *data == NDN_TLV_NDNLP)
        return CCNL_SUITE_NDNTLV;
#endif

/*
#ifdef USE_SUITE_IOTTLV
        if (*data == IOT_TLV_Request || *data == IOT_TLV_Reply)
            return CCNL_SUITE_IOTTLV;
#endif

#ifdef USE_SUITE_LOCALRPC
        if (*data == LRPC_PT_REQUEST || *data == LRPC_PT_REPLY)
            return CCNL_SUITE_LOCALRPC;
#endif
    }
*/

    return -1;
}

void
free_packet(struct ccnl_pkt_s *pkt)
{
    if (pkt) {
        if (pkt->pfx) {
            switch (pkt->pfx->suite) {
#ifdef USE_SUITE_CCNB
            case CCNL_SUITE_CCNB:
                ccnl_free(pkt->s.ccnb.nonce);
                ccnl_free(pkt->s.ccnb.ppkd);
                break;
#endif
#ifdef USE_SUITE_CCNTLV
            case CCNL_SUITE_CCNTLV:
                ccnl_free(pkt->s.ccntlv.objHashRestr);
                ccnl_free(pkt->s.ccntlv.keyIdRestr);
                break;
#endif
#ifdef USE_SUITE_NDNTLV
            case CCNL_SUITE_NDNTLV:
                ccnl_free(pkt->s.ndntlv.nonce);
                ccnl_free(pkt->s.ndntlv.ppkl);
                ccnl_free(pkt->s.ndntlv.dataHashRestr);
                break;
#endif
#ifdef USE_SUITE_CISTLV
            case CCNL_SUITE_CISTLV:
#endif
#ifdef USE_SUITE_IOTTLV
            case CCNL_SUITE_IOTTLV:
#endif
#ifdef USE_SUITE_LOCALRPC
            case CCNL_SUITE_LOCALRPC:
#endif
            default:
                break;
            }
            free_prefix(pkt->pfx);
        }
        ccnl_free(pkt->buf);
        ccnl_free(pkt);
    }
}

// ----------------------------------------------------------------------

const char*
ccnl_addr2ascii(sockunion *su)
{
#ifdef USE_UNIXSOCKET
#   define CCNL_ADDR2ASCII_SIZE 256
#else
#   define CCNL_ADDR2ASCII_SIZE 25
#endif
    static char buf[CCNL_ADDR2ASCII_SIZE];
    int numChars = -1;

    if (!su)
        return CONSTSTR("(local)");

    switch (su->sa.sa_family) {
#ifdef USE_ETHERNET
    case AF_PACKET: {
        struct sockaddr_ll *ll = &su->eth;
        // FIXME: eth2ascii is only available with USE_DEBUG! This function should be as well
        numChars = snprintf(buf, CCNL_ADDR2ASCII_SIZE, "%s/0x%04x",
                            eth2ascii(ll->sll_addr), ntohs(ll->sll_protocol));
        break;
    }
#endif
#ifdef USE_IPV4
    case AF_INET: {
        numChars = snprintf(buf, CCNL_ADDR2ASCII_SIZE, "%s/%u",
                            inet_ntoa(su->ip4.sin_addr),
                            ntohs(su->ip4.sin_port));
        break;
    }
#endif
#ifdef USE_IPV6
    case AF_INET6: {
        char str[INET6_ADDRSTRLEN];
        sprintf(result, "%s/%u", inet_ntop(AF_INET6, su->ip6.sin6_addr.s6_addr, str, INET6_ADDRSTRLEN),
                ntohs(su->ip6.sin6_port));
        return result;
                   }
#endif
#ifdef USE_UNIXSOCKET
    case AF_UNIX: {
        numChars = snprintf(buf, CCNL_ADDR2ASCII_SIZE, "%s", su->ux.sun_path);
        break;
    }
#endif
    default:
        break;
    }

    if (numChars < 0) {
        DEBUGMSG(INFO, "Could not convert address, returning NULL.\n");
        return NULL;
    }

    return buf;
}


#ifdef USE_IPV4

int
ccnl_setIpSocketAddr(struct sockaddr_in *ip4, const char *addr, uint16_t port)
{
    ip4->sin_family = AF_INET;
    ip4->sin_port = htons(port);
    return inet_aton(addr, &ip4->sin_addr);
}

int
ccnl_setSockunionIpAddr(sockunion *su, const char *addr, uint16_t port)
{
    su->sa.sa_family = AF_INET;
    return ccnl_setIpSocketAddr(&su->ip4, addr, port);
}

#endif


#ifdef USE_UNIXSOCKET

int
ccnl_setUnixSocketPath(struct sockaddr_un *ux, const char *path)
{
    ux->sun_family = AF_UNIX;
    return snprintf(ux->sun_path, CCNL_ARRAY_SIZE(ux->sun_path), "%s", path);
}

int
ccnl_setSockunionUnixPath(sockunion *su, const char *path) {
    su->sa.sa_family = AF_UNIX;
    return ccnl_setUnixSocketPath(&su->ux, path);
}

#endif

// ----------------------------------------------------------------------

// This method functions similarily to snprintf. It writes formatted output into
// the provided buffer, given that there is enough buffer capacity. It updates
// the provided buffer position for sequential calls. If an encoding error
// occurs, the buffer is set to NULL.
//
// The method returns the return value of the internal snprintf call. That is it
// returns a negative value if an encoding error occured. Otherwise, it returns
// the number of characters that would have been written if buffer capacity
// would had been sufficiently large, not counting the null terminator.
//
// Additionally it updates both buflen and totalLen. As with the return value,
// totalLen represents the total number of characters that would have been
// written if buffer capacity had been sufficiently large, not counting the null
// terminator. If an error occurs or the buffer has not enough capacity, buflen is set to 0.
// Otherwise it is incremented.
//
// Similar to snprintf, if the provided buflen is 0, totalLen is still updated
// correctly and *buf can be NULL. This is useful to calculate the needed amount
// of buffer capacity.
//
// This method is meant to use in multiple sequential calls to build a string,
// and doing error checking afterwards (buf == NULL? totalLen >= buffer length?).
// Example:
//     char buf[25]; char *tmpBuf = buf;
//     unsigned int remLen = CCNL_ARRAY_SIZE(25), totalLen = 0;
//     snprintf(&tmpBuf, &remLen, &totalLen, "Hello");
//     snprintf(&tmpBuf, &remLen, &totalLen, " World, ");
//     snprintf(&tmpBuf, &remLen, &totalLen, "%d!", 42);
//     if (!tmpBuf)
//         ... handle encoding error ...
//     if (totalLen >= CCNL_ARRAY_SIZE(25))
//         ... handle not enough buffer capacity ...
//
int
ccnl_snprintf(char **buf, unsigned int *buflen, unsigned int *totalLen, const char *format, ...)
{
    int numChars;
    va_list args;

    assert((*buf != NULL || *buflen == 0) && "buf can be (null) only if buflen is zero");

    va_start(args, format);
    #ifdef CCNL_ARDUINO
        numChars = vsnprintf_P(*buf, *buflen, format, args);
    #else
        numChars = vsnprintf(*buf, *buflen, format, args);
    #endif
    va_end(args);

    if (numChars < 0) {
        *buflen = 0;
        *buf = NULL;
        return numChars;
    }

    if (((unsigned int) numChars) >= *buflen) {
        *buflen = 0;
    } else {
        *buflen -= numChars;
        if (*buf) *buf += numChars;
    }

    *totalLen += numChars;

    assert(*buf != NULL || *buflen == 0);
    return numChars;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PACKET_CRAFTING

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, offs;

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillInterest(name, NULL, NULL, tmp, CCNL_MAX_PACKET_SIZE);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        len = ccnl_ccntlv_prependInterestWithHdr(name, &offs, tmp, 0);
        break;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        len = ccnl_cistlv_prependInterestWithHdr(name, &offs, tmp);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV: {
        int rc = ccnl_iottlv_prependRequest(name, NULL, &offs, tmp);
        if (rc <= 0)
            break;
        len = rc;
        rc = ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offs, tmp);
        len = (rc <= 0) ? 0 : len + rc;
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
      len = ccnl_ndntlv_prependInterest(name, NULL, -1, nonce, &offs, tmp);
        break;
#endif
    default:
        break;
    }

    if (len > 0)
        buf = ccnl_buf_new(tmp + offs, len);
    ccnl_free(tmp);

    return buf;
}

struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen, int *payoffset)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    char prefixBuf[CCNL_PREFIX_BUFSIZE];
    int len = 0, contentpos = 0, offs;

    DEBUGMSG_CUTL(DEBUG, "mkSimpleContent (%s, %d bytes)\n",
             ccnl_prefix2path(prefixBuf, CCNL_ARRAY_SIZE(prefixBuf), name), paylen);

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillContent(name, payload, paylen, &contentpos, tmp);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: {
        unsigned int lcn = 0; // lastchunknum
        len = ccnl_ccntlv_prependContentWithHdr(name, payload, paylen, &lcn,
                                                &contentpos, &offs, tmp);
        break;
    }
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        len = ccnl_cistlv_prependContentWithHdr(name, payload, paylen,
                                                NULL, // lastchunknum
                                                &offs, &contentpos, tmp);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV: {
        int rc = ccnl_iottlv_prependReply(name, payload, paylen,
                                         &offs, &contentpos, NULL, tmp);
        if (rc <= 0)
            break;
        len = rc;
        rc = ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offs, tmp);
        len = (rc <= 0) ? 0 : len + rc;
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        len = ccnl_ndntlv_prependContent(name, payload, paylen,
                                         NULL, &contentpos, &offs, tmp);
        break;
#endif
    default:
        break;
    }

    if (len) {
        buf = ccnl_buf_new(tmp + offs, len);
        if (payoffset)
            *payoffset = contentpos;
    }
    ccnl_free(tmp);

    return buf;
}

#endif // NEEDS_PACKET_CRAFTING
// eof
