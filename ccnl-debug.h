/*
 * @f ccnl-debug.h
 * @b CCN lite - debugging header file
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
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
 * 2011-12 created
 * 2013-03-18 updated (ms): removed omnet related code
 */

// ----------------------------------------------------------------------

#ifdef CCNL_DEBUG

#define DEBUGSTMT(LVL, ...) do { \
	if ((LVL)>debug_level) break; \
	__VA_ARGS__; \
} while (0)

#ifndef CCNL_KERNEL
#  define DEBUGMSG(LVL, ...) do {	\
	if ((LVL)>debug_level) break;   \
	fprintf(stderr, "%s: ", timestamp());	\
	fprintf(stderr, __VA_ARGS__);	\
    } while (0)

#else // CCNL_KERNEL
#  define DEBUGMSG(LVL, ...) do {	\
	if ((LVL)>debug_level) break;   \
	printk("%s: ", THIS_MODULE->name);	\
	printk(__VA_ARGS__);		\
    } while (0)
#  define printf(...)	printk(__VA_ARGS__)
#endif // CCNL_KERNEL

// ---------------------------------------------------------------------------
// (ms): Since the following is only used with OMNET i take care of it in
// the container file ccn-lite-omnet.c
/*
#ifdef CCNL_OMNET
# define OMNETDEBUGMSG(LVL, NID, FMT, PARAM) do { \
    char pbuf[200];         \
    if ((LVL)>debug_level) break;   \
    sprintf(pbuf, FMT, PARAM);  \
    ccnl_omnet_LOG(NID, pbuf);  \
} while (0)
#else
# define OMNETDEBUGMSG(LVL, NID, FMT, PARAM) do { \
    DEBUGMSG(LVL, "node %c: " FMT "\n", NID, PARAM); \
} while (0)
#endif // CCNL_OMNET
*/
// ---------------------------------------------------------------------------

#else // !CCNL_DEBUG
#  define DEBUGSTMT(LVL, ...)			do {} while(0)
#  define DEBUGMSG(LVL, ...)			do {} while(0)
#endif // CCNL_DEBUG

#if defined(CCNL_DEBUG_MALLOC)
#  define ccnl_malloc(s)		debug_malloc(s, __FILE__, __LINE__,timestamp())
#  define ccnl_calloc(n,s)	debug_calloc(n, s, __FILE__, __LINE__,timestamp())
#  define ccnl_realloc(p,s)	debug_realloc(p, s, __FILE__, __LINE__)
#  define ccnl_free(p)		debug_free(p, __FILE__, __LINE__)
#  define ccnl_buf_new(p,s)	debug_buf_new(p, s, __FILE__, __LINE__,timestamp())
#else
# ifndef CCNL_KERNEL
#  define ccnl_malloc(s)		malloc(s)
#  define ccnl_calloc(n,s) 	calloc(n,s)
#  define ccnl_realloc(p,s)	realloc(p,s)
#  define ccnl_free(p)		free(p)
#endif
#endif // CCNL_DEBUG_MALLOC

#define free_2ptr_list(a,b)	ccnl_free(a), ccnl_free(b)
#define free_3ptr_list(a,b,c)	ccnl_free(a), ccnl_free(b), ccnl_free(c)
#define free_4ptr_list(a,b,c,d)	ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d);

#define free_prefix(p)	do{ if(p) \
			free_4ptr_list(p->path,p->comp,p->complen,p); } while(0)
#define free_content(c) do{ free_prefix(c->prefix); \
			free_2ptr_list(c->data, c); } while(0)


#ifndef CCNL_KERNEL
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
#endif

// eof
