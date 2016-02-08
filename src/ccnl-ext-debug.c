/*
 * @f ccnl-ext-debug.c
 * @b CCNL debugging support, dumping routines, memory tracking
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
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
 * 2011-04-19 created
 * 2013-03-18 updated (ms): removed omnet related code
 * 2013-03-31 merged with ccnl-debug.h and ccnl-debug-mem.c
 */

#ifdef USE_DEBUG
#  define USE_LOGGING
#endif

#include "ccnl-ext-debug.h"

#ifdef CCNL_ARDUINO
#  define CONSOLE(FMT, ...)   do { \
     sprintf_P(logstr, PSTR(FMT), ##__VA_ARGS__); \
     Serial.print(logstr); \
     Serial.print("\r"); \
   } while(0)
#  define CONSTSTR(s)         ccnl_arduino_getPROGMEMstr(PSTR(s))

#elif defined(CCNL_ANDROID)

static char android_logstr[1024];
void jni_append_to_log(char *line);

#  define CONSOLE(...)        do { sprintf(android_logstr, __VA_ARGS__); \
                                   jni_append_to_log(android_logstr); \
                              } while(0)
#  define CONSTSTR(s)         s

#elif !defined(CCNL_LINUXKERNEL)

#  define CONSOLE(...)        fprintf(stderr, __VA_ARGS__)
#  define CONSTSTR(s)         s

#else // CCNL_LINUXKERNEL

#  define CONSOLE(FMT, ...)   printk(FMT, ##__VA_ARGS__)
#  define CONSTSTR(s)         s

#endif

// ----------------------------------------------------------------------
#ifdef USE_DEBUG

char* ccnl_addr2ascii(sockunion *su);

// ----------------------------------------------------------------------

static void
blob(struct ccnl_buf_s *buf)
{
    unsigned char *cp = buf->data;
    unsigned int i;

    for (i = 0; i < buf->datalen; i++, cp++)
        CONSOLE("%02x", *cp);
}

#ifdef USE_FRAG

char*
frag_protocol(int e)
{
    static char* names[] = { "none", "sequenced2012", "ccnx2013" };
    static char buf[100];
    if (e >= 0 && e <= 2)
        return names[e];
    sprintf(buf, "%d", e);
    return buf;
}
#endif

enum { // numbers for each data type
    CCNL_BUF = 1,
    CCNL_PREFIX,
    CCNL_RELAY,
    CCNL_FACE,
    CCNL_FRAG,
    CCNL_FWD,
    CCNL_INTEREST,
    CCNL_PENDINT,
    CCNL_PACKET,
    CCNL_CONTENT
};

void
ccnl_dump(int lev, int typ, void *p)
{
    struct ccnl_buf_s      *buf = (struct ccnl_buf_s      *) p;
    struct ccnl_prefix_s   *pre = (struct ccnl_prefix_s   *) p;
    struct ccnl_relay_s    *top = (struct ccnl_relay_s    *) p;
    struct ccnl_face_s     *fac = (struct ccnl_face_s     *) p;
#ifdef USE_FRAG
    struct ccnl_frag_s     *frg = (struct ccnl_frag_s     *) p;
#endif
    struct ccnl_forward_s  *fwd = (struct ccnl_forward_s  *) p;
    struct ccnl_interest_s *itr = (struct ccnl_interest_s *) p;
    struct ccnl_pendint_s  *pir = (struct ccnl_pendint_s  *) p;
    struct ccnl_pkt_s      *pkt = (struct ccnl_pkt_s      *) p;
    struct ccnl_content_s  *con = (struct ccnl_content_s  *) p;
    int i, k;

#define INDENT(lev)   for (i = 0; i < lev; i++) CONSOLE("  ")

    switch(typ) {
    case CCNL_BUF:
        while (buf) {
            INDENT(lev);
            CONSOLE("%p BUF len=%zd next=%p\n", (void *) buf, buf->datalen,
                (void *) buf->next);
            buf = buf->next;
        }
        break;
    case CCNL_PREFIX:
        INDENT(lev);
        CONSOLE("%p PREFIX len=%d val=%s\n",
               (void *) pre, pre->compcnt, ccnl_prefix_to_path(pre));
        break;
    case CCNL_RELAY:
        INDENT(lev);
        CONSOLE("%p RELAY\n", (void *) top); lev++;
        INDENT(lev); CONSOLE("interfaces:\n");
        for (k = 0; k < top->ifcount; k++) {
            INDENT(lev+1);
            CONSOLE("ifndx=%d addr=%s", k,
                    ccnl_addr2ascii(&top->ifs[k].addr));
#ifdef CCNL_LINUXKERNEL
            if (top->ifs[k].addr.sa.sa_family == AF_PACKET)
                CONSOLE(" netdev=%p", top->ifs[k].netdev);
            else
                CONSOLE(" sockstruct=%p", top->ifs[k].sock);
#elif !defined(CCNL_RIOT)
            CONSOLE(" sock=%d", top->ifs[k].sock);
#endif
            if (top->ifs[k].reflect)
                CONSOLE(" reflect=%d", top->ifs[k].reflect);
            CONSOLE("\n");
        }
        if (top->faces) {
            INDENT(lev); CONSOLE("faces:\n");
            ccnl_dump(lev+1, CCNL_FACE, top->faces);
        }
        if (top->fib) {
            INDENT(lev); CONSOLE("fib:\n");
            ccnl_dump(lev+1, CCNL_FWD, top->fib);
        }
        if (top->pit) {
            INDENT(lev); CONSOLE("pit:\n");
            ccnl_dump(lev+1, CCNL_INTEREST, top->pit);
        }
        if (top->contents) {
            INDENT(lev); CONSOLE("contents:\n");
            ccnl_dump(lev+1, CCNL_CONTENT, top->contents);
        }
        break;
    case CCNL_FACE:
        while (fac) {
            INDENT(lev);
            CONSOLE("%p FACE id=%d next=%p prev=%p ifndx=%d flags=%02x",
                   (void*) fac, fac->faceid, (void *) fac->next,
                   (void *) fac->prev, fac->ifndx, fac->flags);
            if (0) {}
#ifdef USE_IPV4
            else if (fac->peer.sa.sa_family == AF_INET)
                CONSOLE(" ip=%s", ccnl_addr2ascii(&fac->peer));
#endif
#ifdef USE_IPV6
            else if (fac->peer.sa.sa_family == AF_INET6)
                CONSOLE(" ip=%s", ccnl_addr2ascii(&fac->peer));
#endif
#ifdef USE_LINKLAYER
            else if (fac->peer.sa.sa_family == AF_PACKET)
                CONSOLE(" eth=%s", ccnl_addr2ascii(&fac->peer));
#endif
#ifdef USE_UNIXSOCKET
            else if (fac->peer.sa.sa_family == AF_UNIX)
                CONSOLE(" ux=%s", ccnl_addr2ascii(&fac->peer));
#endif
            else
                CONSOLE(" peer=?");
            if (fac->frag)
                ccnl_dump(lev+2, CCNL_FRAG, fac->frag);
            CONSOLE("\n");
            if (fac->outq) {
                INDENT(lev+1); CONSOLE("outq:\n");
                ccnl_dump(lev+2, CCNL_BUF, fac->outq);
            }
            fac = fac->next;
        }
        break;
#ifdef USE_FRAG
    case CCNL_FRAG:
        CONSOLE(" fragproto=%s mtu=%d",
                frag_protocol(frg->protocol), frg->mtu);
        break;
#endif
    case CCNL_FWD:
        while (fwd) {
            INDENT(lev);
            CONSOLE("%p FWD next=%p face=%p (id=%d suite=%s)\n",
                    (void *) fwd, (void *) fwd->next, (void *) fwd->face,
                    fwd->face->faceid, ccnl_suite2str(fwd->suite));
            ccnl_dump(lev+1, CCNL_PREFIX, fwd->prefix);
            fwd = fwd->next;
        }
        break;
    case CCNL_INTEREST:
        while (itr) {
            INDENT(lev);
            CONSOLE("%p INTEREST next=%p prev=%p last=%d retries=%d\n",
                   (void *) itr, (void *) itr->next, (void *) itr->prev,
                    itr->last_used, itr->retries);
            ccnl_dump(lev+1, CCNL_PACKET, itr->pkt);
            if (itr->pending) {
                INDENT(lev+1);
                CONSOLE("pending:\n");
                ccnl_dump(lev+2, CCNL_PENDINT, itr->pending);
            }
            itr = itr->next;

        }
        break;
    case CCNL_PENDINT:
        while (pir) {
            INDENT(lev);
            CONSOLE("%p PENDINT next=%p face=%p last=%d\n",
                   (void *) pir, (void *) pir->next,
                   (void *) pir->face, pir->last_used);
            pir = pir->next;
        }
        break;
    case CCNL_PACKET:
        INDENT(lev);
        CONSOLE("%p PACKET %s typ=%d cont=%p contlen=%d finalBI=%d flags=0x%04x\n",
                (void *) pkt, ccnl_suite2str(pkt->suite), pkt->type,
                (void*) pkt->content, pkt->contlen, pkt->val.final_block_id,
                pkt->flags);
        ccnl_dump(lev+1, CCNL_PREFIX, pkt->pfx);
        switch(pkt->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            INDENT(lev+1);
            CONSOLE("minsfx=%d maxsfx=%d aok=%d scope=%d",
                    pkt->s.ccnb.minsuffix, pkt->s.ccnb.maxsuffix,
                    pkt->s.ccnb.aok, pkt->s.ccnb.scope);
            if (pkt->s.ccnb.nonce) {
                CONSOLE(" nonce="); blob(pkt->s.ccnb.nonce);
            }
            CONSOLE("\n");
            if (pkt->s.ccnb.ppkd) {
                INDENT(lev+1);
                CONSOLE("ppkd="); blob(pkt->s.ccnb.ppkd);
                CONSOLE("\n");
            }
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
            if (pkt->s.ccntlv.keyid) {
                INDENT(lev+1);
                CONSOLE("keyid="); blob(pkt->s.ccntlv.keyid);
                CONSOLE("\n");
            }
            break;
#endif
#ifdef USE_SUITE_IOTTLV
        case CCNL_SUITE_IOTTLV:
            INDENT(lev+1);
            CONSOLE("ttl=%d\n", pkt->s.iottlv.ttl);
            break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            INDENT(lev+1);
            CONSOLE("minsfx=%d maxsfx=%d mbf=%d scope=%d",
                    pkt->s.ndntlv.minsuffix, pkt->s.ndntlv.maxsuffix,
                    pkt->s.ndntlv.mbf, pkt->s.ndntlv.scope);
            if (pkt->s.ndntlv.nonce) {
                CONSOLE(" nonce="); blob(pkt->s.ndntlv.nonce);
            }
            CONSOLE("\n");
            if (pkt->s.ndntlv.ppkl) {
                INDENT(lev+1);
                CONSOLE("ppkl="); blob(pkt->s.ndntlv.ppkl);
                CONSOLE("\n");
            }
            break;
#endif
        default:
            INDENT(lev+1);
            CONSOLE("... suite-specific packet details here ...\n");
        }
        ccnl_dump(lev+1, CCNL_BUF, pkt->buf);
        break;
    case CCNL_CONTENT:
        while (con) {
            INDENT(lev);
            CONSOLE("%p CONTENT  next=%p prev=%p last_used=%d served_cnt=%d\n",
                   (void *) con, (void *) con->next, (void *) con->prev,
                    con->last_used, con->served_cnt);
            //            ccnl_dump(lev+1, CCNL_PREFIX, con->pkt->pfx);
            ccnl_dump(lev+1, CCNL_PACKET, con->pkt);
            con = con->next;
        }
        break;
    default:
        INDENT(lev);
        CONSOLE("unknown data type %d at %p\n", typ, p);
    }
}

#ifdef USE_MGMT
int
get_buf_dump(int lev, void *p, long *outbuf, int *len, long *next)
{
    struct ccnl_buf_s  *buf = (struct ccnl_buf_s      *) p;
    int line = 0;
    while (buf) {
//        INDENT(lev);
        outbuf[line] = (long) (void *) buf;
        len[line] = buf->datalen;
        next[line] = (long) buf->next;
        buf = buf->next;
        ++line;
    }
    return line;
}

int
get_prefix_dump(int lev, void *p, int *len, char** val)
{
    struct ccnl_prefix_s   *pre = (struct ccnl_prefix_s   *) p;
//    int i;
//    INDENT(lev);
    //*prefix =  (void *) pre;
    *len = pre->compcnt;
    //*val = ccnl_prefix_to_path(pre);
    sprintf(*val, "%s", ccnl_prefix_to_path(pre));
    return 1;
}

int
get_faces_dump(int lev, void *p, int *faceid, long *next, long *prev,
               int *ifndx, int *flags, char **peer, int *type, char **frag)
{
    struct ccnl_relay_s    *top = (struct ccnl_relay_s    *) p;
    struct ccnl_face_s     *fac = (struct ccnl_face_s     *) top->faces;
    int line = 0;

    while (fac) {
//        INDENT(lev);

        faceid[line] = fac->faceid;
        next[line] = (long)(void *) fac->next;
        prev[line] = (long)(void *) fac->prev;
        ifndx[line] = fac->ifndx;
        flags[line] = fac->flags;
        sprintf(peer[line], "%s", ccnl_addr2ascii(&fac->peer));

        if (0) {}
#ifdef USE_IPV4
        if (fac->peer.sa.sa_family == AF_INET)
            type[line] = AF_INET;
#endif
#ifdef USE_IPV6
        if (fac->peer.sa.sa_family == AF_INET6)
            type[line] = AF_INET6;
#endif
#ifdef USE_LINKLAYER
        else if (fac->peer.sa.sa_family == AF_PACKET)
            type[line] = AF_PACKET;
#endif
#ifdef USE_UNIXSOCKET
        else if (fac->peer.sa.sa_family == AF_UNIX)
            type[line] = AF_UNIX;
#endif
        else
            type[line] = 0;
#ifdef USE_FRAG
        if (fac->frag)
            sprintf(frag[line], "fragproto=%s mtu=%d",
                    frag_protocol(fac->frag->protocol), fac->frag->mtu);
        else
#endif
            frag[line][0] = '\0';

        fac = fac->next;
        ++line;
    }
    return line;
}

int
get_fwd_dump(int lev, void *p, long *outfwd, long *next, long *face, int *faceid,
        int *suite, int *prefixlen, char **prefix)
{
    struct ccnl_relay_s    *top = (struct ccnl_relay_s    *) p;
    struct ccnl_forward_s  *fwd = (struct ccnl_forward_s  *) top->fib;
    int line = 0;
    while (fwd) {
//        INDENT(lev);
        /*pos += sprintf(out[line] + pos, "%p FWD next=%p face=%p (id=%d)",
                (void *) fwd, (void *) fwd->next,
                (void *) fwd->face, fwd->face->faceid);*/
        outfwd[line] = (long)(void *) fwd;
        next[line] = (long)(void *) fwd->next;
        face[line] = (long)(void *) fwd->face;
        faceid[line] = fwd->face ? fwd->face->faceid : 0;
        suite[line] = fwd->suite;

        if (fwd->prefix)
            get_prefix_dump(lev, fwd->prefix, &prefixlen[line], &prefix[line]);
        else {
            prefixlen[line] = 99;
            prefix[line] = "?";
        }

        fwd = fwd->next;
        ++line;
    }
    return line;
}

int
get_interface_dump(int lev, void *p, int *ifndx, char **addr, long *dev,
        int *devtype, int *reflect)
{
    struct ccnl_relay_s *top = (struct ccnl_relay_s    *) p;

    int k;
    for (k = 0; k < top->ifcount; k++) {
//        INDENT(lev+1);

        ifndx[k] = k;
        //        sprintf(addr[k], ccnl_addr2ascii(&top->ifs[k].addr));
        strcpy(addr[k], ccnl_addr2ascii(&top->ifs[k].addr));

#ifdef CCNL_LINUXKERNEL
        if (top->ifs[k].addr.sa.sa_family == AF_PACKET) {
            dev[k] = (long) (void *) top->ifs[k].netdev; //%p
            devtype[k] = 1;
        } else {
            dev[k] = (long) (void *) top->ifs[k].sock; //%p
            devtype[k] = 2;
        }
#else
        dev[k] = (long) top->ifs[k].sock;
        devtype[k] = 3;
#endif
        if (top->ifs[k].reflect)
            reflect[k] = top->ifs[k].reflect;
    }
    return top->ifcount;
}

int
get_interest_dump(int lev, void *p, long *interest, long *next, long *prev,
        int *last, int *min, int *max, int *retries, long *publisher,
        int *prefixlen, char **prefix)
{

    struct ccnl_relay_s *top = (struct ccnl_relay_s    *) p;
    struct ccnl_interest_s *itr = (struct ccnl_interest_s *) top->pit;

    int line = 0;
    while (itr) {
//        INDENT(lev);

        interest[line] = (long)(void *) itr;
        next[line] = (long)(void *) itr->next;
        prev[line] = (long)(void *) itr->prev;
        last[line] = itr->last_used;
        retries[line] = itr->retries;
        min[line] = max[line] = -1;
        publisher[line] = 0L;
        if (itr->pkt->pfx)
            switch (itr->pkt->pfx->suite) {
            case CCNL_SUITE_CCNB:
                min[line] = itr->pkt->s.ccnb.minsuffix;
                max[line] = itr->pkt->s.ccnb.maxsuffix;
                publisher[line] = (long)(void *) itr->pkt->s.ccnb.ppkd;
                break;
            case CCNL_SUITE_NDNTLV:
                min[line] = itr->pkt->s.ndntlv.minsuffix;
                max[line] = itr->pkt->s.ndntlv.maxsuffix;
                publisher[line] = (long)(void *) itr->pkt->s.ndntlv.ppkl;
                break;
            default:
                break;
            }
        get_prefix_dump(lev, itr->pkt->pfx, &prefixlen[line], &prefix[line]);

        itr = itr->next;
        ++line;
    }
    return line;
}

int
get_pendint_dump(int lev, void *p, char **out){
    struct ccnl_relay_s *top = (struct ccnl_relay_s    *) p;
    struct ccnl_interest_s *itr = (struct ccnl_interest_s *) top->pit;
    struct ccnl_pendint_s  *pir = (struct ccnl_pendint_s  *) itr->pending;

    int pos = 0, line = 0;
    while (pir) {
//        INDENT(lev);
        pos = 0;
        pos += sprintf(out[line] + pos, "%p PENDINT next=%p face=%p last=%d",
               (void *) pir, (void *) pir->next,
               (void *) pir->face, pir->last_used);
        pir = pir->next;
        ++line;
    }
    return line;
}

int
get_content_dump(int lev, void *p, long *content, long *next, long *prev,
        int *last_use, int *served_cnt, int *prefixlen, char **prefix){

    struct ccnl_relay_s *top = (struct ccnl_relay_s    *) p;
    struct ccnl_content_s  *con = (struct ccnl_content_s  *) top->contents;

    int line = 0;
    while (con) {
//        INDENT(lev);
        content[line] = (long)(void *) con;
        next[line] = (long)(void *) con->next;
        prev[line] = (long)(void *) con->prev;
        last_use[line] = con->last_used;
        served_cnt[line] = con->served_cnt;

        get_prefix_dump(lev, con->pkt->pfx, &prefixlen[line], &prefix[line]);

        con = con->next;
        ++line;
    }
    return line;
}
#endif // USE_MGMT

#endif // !USE_DEBUG

// -----------------------------------------------------------------

char* timestamp(void);

#ifdef USE_DEBUG_MALLOC

#ifdef CCNL_ARDUINO

#  define ccnl_malloc(s)        debug_malloc(s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_calloc(n,s)      debug_calloc(n, s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_realloc(p,s)     debug_realloc(p, s, PSTR(__FILE__), __LINE__)
#  define ccnl_strdup(s)        debug_strdup(s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_free(p)          debug_free(p, PSTR(__FILE__), __LINE__)
#  define ccnl_buf_new(p,s)     debug_buf_new(p, s, PSTR(__FILE__), __LINE__, CCNL_NOW())

void*
debug_malloc(int s, const char *fn, int lno, double tstamp)

#else

#  define ccnl_malloc(s)        debug_malloc(s, __FILE__, __LINE__,timestamp())
#  define ccnl_calloc(n,s)      debug_calloc(n, s, __FILE__, __LINE__,timestamp())
#  define ccnl_realloc(p,s)     debug_realloc(p, s, __FILE__, __LINE__)
#  define ccnl_strdup(s)        debug_strdup(s, __FILE__, __LINE__,timestamp())
#  define ccnl_free(p)          debug_free(p, __FILE__, __LINE__)
#  define ccnl_buf_new(p,s)     debug_buf_new(p, s, __FILE__, __LINE__,timestamp())

void*
debug_malloc(int s, const char *fn, int lno, char *tstamp)

#endif

// void*
// debug_malloc(int s, const char *fn, int lno, char *tstamp)
{
    struct mhdr *h = (struct mhdr *) malloc(s + sizeof(struct mhdr));

    if (!h)
        return NULL;
    h->next = mem;
    mem = h;
    h->fname = (char *) fn;
    h->lineno = lno;
    h->size = s;
#ifdef CCNL_ARDUINO
    h->tstamp = tstamp;
#else
    h->tstamp = strdup(tstamp);
#endif
    /*
    if (s == 32) CONSOLE("+++ s=%d %p at %s:%d\n", s,
                         (void*)(((unsigned char *)h) + sizeof(struct mhdr)),
                         (char*) fn, lno);
    */
    return ((unsigned char *)h) + sizeof(struct mhdr);
}

#ifdef CCNL_ARDUINO
void*
debug_calloc(int n, int s, const char *fn, int lno, double tstamp)
#else
void*
debug_calloc(int n, int s, const char *fn, int lno, char *tstamp)
#endif
{
    void *p = debug_malloc(n * s, fn, lno, tstamp);
    if (p)
        memset(p, 0, n*s);
    return p;
}


int
debug_unlink(struct mhdr *hdr)
{
    struct mhdr **pp = &mem;

    for (pp = &mem; pp; pp = &((*pp)->next)) {
        if (*pp == hdr) {
            *pp = hdr->next;
            return 0;
        }
    if (!(*pp)->next)
            break;
    }
    return 1;
}

void*
debug_realloc(void *p, int s, const char *fn, int lno)
{
    struct mhdr *h = (struct mhdr *) (((unsigned char *)p) - sizeof(struct mhdr));

    if (p) {
        if (debug_unlink(h)) {
            CONSOLE("%s @@@ memerror - realloc(%s:%d) at "
                    "%s:%d does not find memory block\n",
                    timestamp(), h->fname, h->lineno, fn, lno);
            return NULL;
        }
        h = (struct mhdr *) realloc(h, s+sizeof(struct mhdr));
        if (!h)
            return NULL;
    } else
        h = (struct mhdr *) malloc(s+sizeof(struct mhdr));
    h->fname = (char *) fn;
    h->lineno = lno;
    h->size = s;
    h->next = mem;
    mem = h;
    return ((unsigned char *)h) + sizeof(struct mhdr);
}

#ifdef CCNL_ARDUINO
void*
debug_strdup(const char *s, const char *fn, int lno, double tstamp)
#else
void*
debug_strdup(const char *s, const char *fn, int lno, char *tstamp)
#endif
{
    char *cp;

    if (!s)
        return NULL;
    cp = (char*) debug_malloc(strlen(s)+1, fn, lno, tstamp);
    if (cp)
        strcpy(cp, s);
    return cp;
}

void
debug_free(void *p, const char *fn, int lno)
{
    struct mhdr *h = (struct mhdr *) (((unsigned char *)p) - sizeof(struct mhdr));

    if (!p) {
//      CONSOLE("%s: @@@ memerror - free() of NULL ptr at %s:%d\n",
//         timestamp(), fn, lno);
        return;
    }
    if (debug_unlink(h)) {
        CONSOLE(
           "%s @@@ memerror - free() at %s:%d does not find memory block %p\n",
                timestamp(), fn, lno, p);
        return;
    }
#ifndef CCNL_ARDUINO
    if (h->tstamp && *h->tstamp)
        free(h->tstamp);
#endif
    //free(h);
    // instead of free: do a
       memset(h+1, 0x8f, h->size);
    // to discover continued use of a freed memory zone
}

#ifdef CCNL_ARDUINO

struct ccnl_buf_s*
debug_buf_new(void *data, int len, const char *fn, int lno, double tstamp)

#else

struct ccnl_buf_s*
debug_buf_new(void *data, int len, const char *fn, int lno, char *tstamp)

#endif

{
    struct ccnl_buf_s *b =
         (struct ccnl_buf_s *) debug_malloc(sizeof(*b) + len, fn, lno, tstamp);

    if (!b)
        return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
        memcpy(b->data, data, len);
    return b;
}


#else // !USE_DEBUG_MALLOC

# ifndef CCNL_LINUXKERNEL
#  define ccnl_malloc(s)        malloc(s)
#  define ccnl_calloc(n,s)      calloc(n,s)
#  define ccnl_realloc(p,s)     realloc(p,s)
#  define ccnl_strdup(s)        strdup(s)
#  define ccnl_free(p)          free(p)
# endif

struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b = (struct ccnl_buf_s*) ccnl_malloc(sizeof(*b) + len);

    if (!b)
        return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
        memcpy(b->data, data, len);
    return b;
}

#endif // !USE_DEBUG_MALLOC


#define free_2ptr_list(a,b)       ccnl_free(a), ccnl_free(b)
#define free_3ptr_list(a,b,c)     ccnl_free(a), ccnl_free(b), ccnl_free(c)
#define free_4ptr_list(a,b,c,d)   ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d);
#define free_5ptr_list(a,b,c,d,e) ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d), ccnl_free(e);

#define free_prefix(p)  do{ if(!p) break; ccnl_free(p->chunknum);  \
                free_5ptr_list(p->bytes,p->comp,p->complen,p->chunknum,p); } while(0)
#define free_content(c) do{ /* free_prefix(c->name); */ free_packet(c->pkt); \
                        ccnl_free(c); } while(0)

void free_packet(struct ccnl_pkt_s *pkt);

// -----------------------------------------------------------------
int debug_level;

// eof
