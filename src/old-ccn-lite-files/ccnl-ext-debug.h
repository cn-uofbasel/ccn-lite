/*
 * @f ccnl-ext-debug.h
 * @b CCNL debugging support, dumping routines, memory tracking, stats
 *
 * Copyright (C) 2011-13, Christian Tschudin, University of Basel
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
 * 2014-10-30 created <christopher.scherb@unibas.ch
 */

#ifndef CCNL_EXT_DEBUG_H
#define CCNL_EXT_DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
// #ifdef USE_DEBUG
// #  define USE_LOGGING
// #endif

#ifdef USE_DEBUG
#ifdef USE_DEBUG_MALLOC

struct mhdr {
    struct mhdr *next;
    char *fname;
    int lineno, size;
#ifdef CCNL_ARDUINO
    double tstamp;
#else
    char *tstamp; // Linux kernel (no double), also used for CCNL_UNIX
#endif
} *mem;

#endif // USE_DEBUG_MALLOC

#ifndef CCNL_LINUXKERNEL
struct ccnl_stats_s {
    void* log_handler;
    FILE *ofp;
    int log_sent;
    int log_recv;
    int log_recv_old;
    int log_sent_old;
    int log_need_rt_seqn;
    int log_content_delivery_rate_per_s;
    double log_start_t;
    double log_cdr_t;
    //
    FILE *ofp_r, *ofp_s, *ofp_q;
    double log_recv_t_i;
    double log_recv_t_c;
    double log_sent_t_i;
    double log_sent_t_c;
};
#endif // CCNL_LINUXKERNEL

#endif // USE_DEBUG



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



char* timestamp(void);

#ifdef USE_DEBUG_MALLOC

#ifdef CCNL_ARDUINO

#  define ccnl_malloc(s)        debug_malloc(s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_calloc(n,s)      debug_calloc(n, s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_realloc(p,s)     debug_realloc(p, s, PSTR(__FILE__), __LINE__)
#  define ccnl_strdup(s)        debug_strdup(s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_free(p)          debug_free(p, PSTR(__FILE__), __LINE__)

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



#else // !USE_DEBUG_MALLOC



# ifndef CCNL_LINUXKERNEL
#  define ccnl_malloc(s)        malloc(s)
#  define ccnl_calloc(n,s)      calloc(n,s)
#  define ccnl_realloc(p,s)     realloc(p,s)
#  define ccnl_strdup(s)        strdup(s)
#  define ccnl_free(p)          free(p)
# endif



#endif // !USE_DEBUG_MALLOC


#define free_2ptr_list(a,b)       ccnl_free(a), ccnl_free(b)
#define free_3ptr_list(a,b,c)     ccnl_free(a), ccnl_free(b), ccnl_free(c)
#define free_4ptr_list(a,b,c,d)   ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d);
#define free_5ptr_list(a,b,c,d,e) ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d), ccnl_free(e);



void free_packet(struct ccnl_pkt_s *pkt);




// -----------------------------------------------------------------
#ifdef USE_DEBUG

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

char* ccnl_addr2ascii(sockunion *su);

char *frag_protocol(int e);
void ccnl_dump(int lev, int typ, void *p);
int get_buf_dump(int lev, void *p, long *outbuf, int *len, long *next);
int get_prefix_dump(int lev, void *p, int *len, char **val);
int get_num_faces(void *p);
int get_faces_dump(int lev, void *p, int *faceid, long *next, long *prev, int *ifndx, int *flags, char **peer, int *type, char **frag);
int get_num_fwds(void *p);
int get_fwd_dump(int lev, void *p, long *outfwd, long *next, long *face, int *faceid, int *suite, int *prefixlen, char **prefix);
int get_num_interface(void *p);
int get_interface_dump(int lev, void *p, int *ifndx, char **addr, long *dev, int *devtype, int *reflect);
int get_num_interests(void *p);
int get_interest_dump(int lev, void *p, long *interest, long *next, long *prev, int *last, int *min, int *max, int *retries, long *publisher, int *prefixlen, char **prefix);
int get_pendint_dump(int lev, void *p, char **out);
int get_num_contents(void *p);
int get_content_dump(int lev, void *p, long *content, long *next, long *prev, int *last_use, int *served_cnt, int *prefixlen, char **prefix);
#endif

#ifdef USE_DEBUG_MALLOC
void *debug_malloc(int s, const char *fn, int lno, char *tstamp);
void *debug_calloc(int n, int s, const char *fn, int lno, char *tstamp);
int debug_unlink(struct mhdr *hdr);
void *debug_realloc(void *p, int s, const char *fn, int lno);
void *debug_strdup(const char *s, const char *fn, int lno, char *tstamp);
void debug_free(void *p, const char *fn, int lno);
#endif


// eof
#endif /* CCNL-EXT-DEBUG_H */
