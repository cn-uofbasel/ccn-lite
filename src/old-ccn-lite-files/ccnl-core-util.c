/*
 * @f ccnl-core-util.c
 * @b CCN lite, common utility procedures (used by utils as well as relays)
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
 * Copyright (C) 2016, Martine Lenders <mlenders@inf.fu-berlin.de>
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
#include "string.h"
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
#ifdef USE_LINKLAYER
        "ETHERNET, "
#endif
#ifdef USE_WPAN
        "WPAN, "
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

int ccnl_cmp2int(unsigned char *cmp, int cmplen) {
    char *str = (char *)ccnl_malloc(cmplen);
    memcpy(str, (char *)cmp, cmplen);
    long int i = strtol(str, NULL, 0);
    free(str);
    return (int)i;
}

// ----------------------------------------------------------------------

int
hex2int(unsigned char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    c = tolower(c);
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 0x0a;
    return 0;
}

int
unescape_component(char *comp) // inplace, returns len after shrinking
{
    char *in = comp, *out = comp;
    int len;

    for (len = 0; *in; len++) {
        if (in[0] != '%' || !in[1] || !in[2]) {
            *out++ = *in++;
            continue;
        }
        *out++ = hex2int(in[1])*16 + hex2int(in[2]);
        in += 3;
    }
    return len;
}

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

// fill in the compVector (watch out: this modifies the uri string)
int
ccnl_URItoComponents(char **compVector, unsigned int *compLens, char *uri)
{
    int i, len;

    if (*uri == '/')
        uri++;

    for (i = 0; *uri && i < (CCNL_MAX_NAME_COMP - 1); i++) {
        compVector[i] = uri;
        while (*uri && *uri != '/') {
            uri++;
        }
        if (*uri) {
            *uri = '\0';
            uri++;
        }
        len = unescape_component(compVector[i]);

        if (compLens)
            compLens[i] = len;

        compVector[i][len] = '\0';
    }
    compVector[i] = NULL;

    return i;
}

// turn an URI into an internal prefix (watch out: this modifies the uri string)
struct ccnl_prefix_s *
ccnl_URItoPrefix(char* uri, int suite, char *nfnexpr, unsigned int *chunknum)
{
    struct ccnl_prefix_s *p;
    char *compvect[CCNL_MAX_NAME_COMP];
    unsigned int complens[CCNL_MAX_NAME_COMP];
    int cnt, i, len, tlen;

    DEBUGMSG_CUTL(TRACE, "ccnl_URItoPrefix(suite=%s, uri=%s, nfn=%s)\n",
             ccnl_suite2str(suite), uri, (nfnexpr != NULL) ? nfnexpr : "none");

    if (strlen(uri))
        cnt = ccnl_URItoComponents(compvect, complens, uri);
    else
        cnt = 0;

    if (nfnexpr && *nfnexpr)
        cnt += 1;

    p = ccnl_prefix_new(suite, cnt);
    if (!p)
        return NULL;

    for (i = 0, len = 0; i < cnt; i++) {
        if (i == (cnt-1) && nfnexpr && *nfnexpr)
            len += strlen(nfnexpr);
        else
            len += complens[i];//strlen(compvect[i]);
    }
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        len += cnt * 4; // add TL size
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        len += cnt * 4; // add TL size
#endif

    p->bytes = (unsigned char*) ccnl_malloc(len);
    if (!p->bytes) {
        free_prefix(p);
        return NULL;
    }

    for (i = 0, len = 0, tlen = 0; i < cnt; i++) {
        int isnfnfcomp = i == (cnt-1) && nfnexpr && *nfnexpr;
        char *cp = isnfnfcomp ? nfnexpr : (char*) compvect[i];

        if (isnfnfcomp)
            tlen = strlen(nfnexpr);
        else
            tlen = complens[i];

        p->comp[i] = p->bytes + len;
        tlen = ccnl_pkt_mkComponent(suite, p->comp[i], cp, tlen);
        p->complen[i] = tlen;
        len += tlen;
    }

    p->compcnt = cnt;
#ifdef USE_NFN
    if (nfnexpr && *nfnexpr)
        p->nfnflags |= CCNL_PREFIX_NFN;
#endif

    if(chunknum) {
        p->chunknum = (int*) ccnl_malloc(sizeof(int));
        *p->chunknum = *chunknum;
    }

    return p;
}



// ----------------------------------------------------------------------

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

// ----------------------------------------------------------------------
static inline char
_half_byte_to_char(uint8_t half_byte)
{
    return (half_byte < 10) ? ('0' + half_byte) : ('a' + (half_byte - 10));
}

/* translates link-layer address into a string */
char*
ll2ascii(unsigned char *addr, size_t len)
{
    size_t i;
    static char out[CCNL_LLADDR_STR_MAX_LEN + 1];

    out[0] = '\0';

    for (i = 0; i < len; i++) {
        out[(3 * i)] = _half_byte_to_char(addr[i] >> 4);
        out[(3 * i) + 1] = _half_byte_to_char(addr[i] & 0xf);

        if (i != (len - 1)) {
            out[(3 * i) + 2] = ':';
        }
        else {
            out[(3 * i) + 2] = '\0';
        }
    }

    return out;
}

char*
ccnl_addr2ascii(sockunion *su)
{
#ifdef USE_UNIXSOCKET
    static char result[256];
#else
    /* each byte requires 2 chars + 1 for the colon/slash + 6 for the protocol + 1 for \0 */
    static char result[(CCNL_MAX_ADDRESS_LEN * 3) + 7];
#endif

    if (!su)
        return CONSTSTR("(local)");

    switch (su->sa.sa_family) {
#ifdef USE_LINKLAYER
    case AF_PACKET: {
        struct sockaddr_ll *ll = &su->linklayer;
        strcpy(result, ll2ascii(ll->sll_addr, ll->sll_halen));
        sprintf(result+strlen(result), "/0x%04x",
            ntohs(ll->sll_protocol));
        return result;
    }
#endif
#ifdef USE_WPAN
    case AF_IEEE802154: {
        struct sockaddr_ieee802154 *wpan = &su->wpan;
        sprintf(result, "PANID:0x%04x", wpan->addr.pan_id);
        switch (wpan->addr.addr_type) {
            case IEEE802154_ADDR_NONE:
                break;
            case IEEE802154_ADDR_SHORT:
                sprintf(result+strlen(result), ",0x%04x", wpan->addr.addr.short_addr);
                break;
            case IEEE802154_ADDR_LONG:
                sprintf(result+strlen(result), ",%s", ll2ascii(wpan->addr.addr.hwaddr, IEEE802154_ADDR_LEN));
                break;
            default:
                strcpy(result+strlen(result), ",UNKNOWN");
                break;
        }
        return result;
    }
#endif
#ifdef USE_IPV4
    case AF_INET:
        sprintf(result, "%s/%u", inet_ntoa(su->ip4.sin_addr),
                ntohs(su->ip4.sin_port));
        return result;
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
    case AF_UNIX:
        strncpy(result, su->ux.sun_path, sizeof(result)-1);
        result[sizeof(result)-1] = 0;
        return result;
#endif
    default:
        break;
    }

    (void) result; // silence compiler warning (if neither USE_LINKLAYER, USE_IPV4, USE_IPV6, nor USE_UNIXSOCKET is set)
    return NULL;
}

// ----------------------------------------------------------------------
#ifdef NEEDS_PREFIX_MATCHING

/* add a new entry to the FIB */
int
ccnl_fib_add_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx,
                   struct ccnl_face_s *face)
{
    struct ccnl_forward_s *fwd, **fwd2;

    char *s = NULL;
    DEBUGMSG_CUTL(INFO, "adding FIB for <%s>, suite %s\n",
             (s = ccnl_prefix_to_path(pfx)), ccnl_suite2str(pfx->suite));
    ccnl_free(s);

    for (fwd = relay->fib; fwd; fwd = fwd->next) {
        if (fwd->suite == pfx->suite &&
                        !ccnl_prefix_cmp(fwd->prefix, NULL, pfx, CMP_EXACT)) {
            free_prefix(fwd->prefix);
            fwd->prefix = NULL;
            break;
        }
    }
    if (!fwd) {
        fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
        if (!fwd)
            return -1;
        fwd2 = &relay->fib;
        while (*fwd2)
            fwd2 = &((*fwd2)->next);
        *fwd2 = fwd;
        fwd->suite = pfx->suite;
    }
    fwd->prefix = pfx;
    fwd->face = face;
    DEBUGMSG_CUTL(DEBUG, "added FIB via %s\n", ccnl_addr2ascii(&fwd->face->peer));

    return 0;
}

/* add a new entry to the FIB */
int
ccnl_fib_rem_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx,
                   struct ccnl_face_s *face)
{
    struct ccnl_forward_s *fwd;
    int res = -1;

    if (pfx != NULL) {
        char *s = NULL;
        DEBUGMSG_CUTL(INFO, "removing FIB for <%s>, suite %s\n",
                      (s = ccnl_prefix_to_path(pfx)), ccnl_suite2str(pfx->suite));
        ccnl_free(s);
    }

    struct ccnl_forward_s *last = NULL;
    for (fwd = relay->fib; fwd; last = fwd, fwd = fwd->next) {
        if (((pfx == NULL) || (fwd->suite == pfx->suite)) &&
            ((pfx == NULL) || !ccnl_prefix_cmp(fwd->prefix, NULL, pfx, CMP_EXACT)) &&
            ((face == NULL) || (fwd->face == face))) {
            res = 0;
            if (!last) {
                relay->fib = fwd->next;
            }
            else {
                last->next = fwd->next;
            }
            free_prefix(fwd->prefix);
            ccnl_free(fwd);
            break;
        }
    }
    DEBUGMSG_CUTL(DEBUG, "added FIB via %s\n", ccnl_addr2ascii(&fwd->face->peer));

    return res;
}


#endif

/* prints the current FIB */
void
ccnl_fib_show(struct ccnl_relay_s *relay)
{
#ifndef CCNL_LINUXKERNEL
    struct ccnl_forward_s *fwd;

    printf("%-30s | %-10s | %-9s | Peer\n",
           "Prefix", "Suite",
#ifdef CCNL_RIOT
           "Interface"
#else
           "Socket"
#endif
           );
    puts("-------------------------------|------------|-----------|------------------------------------");
    char *s = NULL;
    for (fwd = relay->fib; fwd; fwd = fwd->next) {
        printf("%-30s | %-10s |        %02i | %s\n", (s = ccnl_prefix_to_path(fwd->prefix)),
                                     ccnl_suite2str(fwd->suite), (int)
                                     /* TODO: show correct interface instead of always 0 */
#ifdef CCNL_RIOT
                                     (relay->ifs[0]).if_pid,
#else
                                     (relay->ifs[0]).sock,
#endif
                                     ccnl_addr2ascii(&fwd->face->peer));
        ccnl_free(s);
    }
#endif
}

void
ccnl_cs_dump(struct ccnl_relay_s *ccnl)
{
    struct ccnl_content_s *c = ccnl->contents;
    unsigned i = 0;
    char *s;
    while (c) {
        printf("CS[%u]: %s [%d]: %s\n", i++,
               (s = ccnl_prefix_to_path(c->pkt->pfx)),
               (c->pkt->pfx->chunknum)? *(c->pkt->pfx->chunknum) : -1,
               c->pkt->content);
        c = c->next;
        ccnl_free(s);
    }
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
        len = ccnl_ccnb_fillInterest(name, NULL, tmp, CCNL_MAX_PACKET_SIZE);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        len = ccnl_ccntlv_prependInterestWithHdr(name, &offs, tmp);
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
        len = ccnl_ndntlv_prependInterest(name, -1, nonce, &offs, tmp);
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
    int len = 0, contentpos = 0, offs;

    char *s = NULL;
    DEBUGMSG_CUTL(DEBUG, "mkSimpleContent (%s, %d bytes)\n",
             (s = ccnl_prefix_to_path(name)), paylen);
    ccnl_free(s);

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
                                         &contentpos, NULL, &offs, tmp);
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
