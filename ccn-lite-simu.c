/*
 * @f ccn-lite-simu.c
 * @b prog with multiple CCNL relays, running a standalone simulation
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
 * 2011-11-22 created
 * 2011-12 simulation scenario and logging support s.braun@stud.unibas.ch
 * 2013-03-19 updated (ms): replacement code after renaming the field
 * 		ccnl_relay_s.client to ccnl_relay_s.aux
 */


#define CCNL_SIMULATION

#define USE_DEBUG
#define USE_DEBUG_MALLOC
//#define USE_FRAG
#define USE_SUITE_CCNB
#define USE_SUITE_NDNTLV
#define USE_SCHEDULER
#define USE_ETHERNET

#include "ccnl-includes.h"

#include "ccnl.h"
#include "ccnl-core.h"

#include "ccnl-ext-debug.c"
#include "ccnl-ext.h"
#include "ccnl-platform.c"

int ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

void ccnl_print_stats(struct ccnl_relay_s *relay, int code);
enum {STAT_RCV_I, STAT_RCV_C, STAT_SND_I, STAT_SND_C, STAT_QLEN, STAT_EOP1};

#include "ccnl-core.c"

#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-sched.c"
#include "pkt-ccnb-enc.c"
#include "pkt-ndntlv-enc.c"
#include "ccnl-ext-frag.c"

char suite = CCNL_SUITE_CCNB;

// ----------------------------------------------------------------------

#define SIMU_NUMBER_OF_CHUNKS	10
// #define SIMU_CHUNK_SIZE		4096
#define SIMU_CHUNK_SIZE		1600
// #define SIMU_CHUNK_SIZE		1340

static struct ccnl_relay_s* char2relay(char node);
static char relay2char(struct ccnl_relay_s *relay);
void ccnl_simu_fini(void *ptr, int aux);

void ccnl_simu_phase_two(void *ptr, void *dummy);


extern void ccnl_simu_client_RX(struct ccnl_relay_s *relay, char *name,
				int seqn, char *data, int len);
extern void ccnl_simu_ll_TX(struct ccnl_relay_s *relay, unsigned char *dst,
			   unsigned char *src, unsigned char *data, int len);
extern void ccnl_simu_LOG(struct ccnl_relay_s *relay, char *s);

void ccnl_simu_client(char node); // sending side

void ccnl_simu_ethernet(void *dummy, void *dummy2); // implements the media

void ccnl_simu_client_endlog(struct ccnl_relay_s *relay);

// ----------------------------------------------------------------------

static struct ccnl_prefix_s*
ccnl_path_to_prefix(const char *path)
{
    char *cp;
    struct ccnl_prefix_s *pr = (struct ccnl_prefix_s*) ccnl_calloc(1,
							   sizeof(*pr));
    DEBUGMSG(99, "ccnl_path_to_prefix <%s>\n", path);

    if (!pr)
        return NULL;
    pr->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char**));
    pr->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    pr->path = (unsigned char*) ccnl_malloc(strlen(path)+1);
    if (!pr->comp || !pr->complen || !pr->path) {
        ccnl_free(pr->comp);
        ccnl_free(pr->complen);
        ccnl_free(pr->path);
        ccnl_free(pr);
        return NULL;
    }

    strcpy((char*) pr->path, path);
    cp = (char*) pr->path;

    for (path = strtok(cp, "/");
		 path && pr->compcnt < CCNL_MAX_NAME_COMP;
		 path = strtok(NULL, "/")) {
        pr->comp[pr->compcnt] = (unsigned char*) path;
        pr->complen[pr->compcnt] = strlen(path);
        pr->compcnt++;
    }
    return pr;
}

// ----------------------------------------------------------------------

struct ccnl_relay_s relays[5];

#define relayChars "ABC12"

static struct ccnl_relay_s*
char2relay(char node)
{
    unsigned int i;

    for (i = 0; i < strlen(relayChars); i++) {
	if (node == relayChars[i])
	    return relays + i;
    }
    fprintf(stderr, "unknown node %c\n", node);
    return 0;
}

static char
relay2char(struct ccnl_relay_s *relay)
{
    unsigned int i;

    for (i = 0; i < strlen(relayChars); i++) {
	if (relay == (relays + i))
	    return relayChars[i];
    }
    fprintf(stderr, "unknown node %p\n", (void *) relay);
    return '?';
}


void
ccnl_simu_add2cache(char node, const char *name, int seqn, void *data, int len)
{
    struct ccnl_relay_s *relay = char2relay(node);
    struct ccnl_buf_s *bp;
    struct ccnl_prefix_s *pp;
    char *name2, tmp[10], *n;
    char *namecomp[20];
    unsigned char tmp2[8192];
    int cnt, len2;
    struct ccnl_content_s *c;

    cnt = 0;
    n = name2 = strdup(name);
    name2 = strtok(name2, "/");
    while (name2) {
	namecomp[cnt++] = name2;
	name2 = strtok(NULL, "/");
    }
    sprintf(tmp, ".%d", seqn);
    namecomp[cnt++] = tmp;
    namecomp[cnt] = 0;

    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
	len2 = mkContent(namecomp, (char*) data, len, tmp2);
	break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
	len2 = sizeof(tmp2);
	ccnl_ndntlv_mkContent(namecomp, data, len, &len2,
			      (unsigned char*) tmp2);
	memmove(tmp2, tmp2 + len2, sizeof(tmp2) - len2);
	len2 = sizeof(tmp2) - len2;
	break;
#endif
    default:
	len2 = 0;
	break;
    }
    free(n);

    if (len2) {
	bp = ccnl_buf_new(tmp2, len2);

	strcpy((char*) tmp2, name);
	sprintf(tmp, "/.%d", seqn);
	strcat((char*) tmp2, tmp);

	pp = ccnl_path_to_prefix((const char*)tmp2);

	c = ccnl_content_new(relay, suite, &bp, &pp, NULL, 0, 0);
	if (c)
	    ccnl_content_add2cache(relay, c);
    }

}

// ----------------------------------------------------------------------

void
ccnl_client_TX(char node, char *name, int seqn, unsigned int nonce)
{
    char *namecomp[20], *n;
    char tmp[512], tmp2[10];
    int len, cnt;
    struct ccnl_relay_s *relay = char2relay(node);
    if (!relay)	return;

    DEBUGMSG(10, "ccnl_client_tx node=%c: request %s #%d\n", node, name, seqn);

    cnt = 0;
    n = name = strdup(name);
    name = strtok(name, "/");
    while (name) {
	namecomp[cnt++] = name;
	name = strtok(NULL, "/");
    }
    sprintf(tmp2, ".%d", seqn);
    namecomp[cnt++] = tmp2;
    namecomp[cnt] = 0;

    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
	len = mkInterest(namecomp, &nonce, (unsigned char*) tmp);
	break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
	len = sizeof(tmp);
	ccnl_ndntlv_mkInterest(namecomp, -1, &len, (unsigned char*) tmp);
	memmove(tmp, tmp + len, sizeof(tmp) - len);
	len = sizeof(tmp) - len;
	break;
#endif
    default:
	len = 0;
	break;
    }
    free(n);

    // inject it into the relay:
    if (len)
	ccnl_core_RX(relay, -1, (unsigned char*)tmp, len, 0, 0);
}

// ----------------------------------------------------------------------
// simulated ethernet segment

struct ccnl_ethernet_s {
    struct ccnl_ethernet_s *next;
    unsigned char *dst, *src;
    unsigned short protocol;
    unsigned char *data;
    int len;
};
struct ccnl_ethernet_s *etherqueue;

void
ccnl_simu_ethernet(void *dummy, void *dummy2)
{
    if (etherqueue) {
	int i, qlen;
	struct ccnl_ethernet_s **pp, *p;

	// pick the last element in the queue
	for (qlen = 1, pp = &etherqueue; (*pp)->next; pp = &((*pp)->next))
	    qlen++;
	p = *pp;
	*pp = NULL;

	for (i = 0; i < 5; i++) {
	    if (!memcmp(p->dst, &relays[i].ifs[0].addr.eth.sll_addr, ETH_ALEN)) {
		break;
	    }
	}
	if (i < 5) {
	    sockunion sun;

	    sun.sa.sa_family = AF_PACKET;
	    memcpy(sun.eth.sll_addr, p->src, ETH_ALEN);

	    DEBUGMSG(10, "simu_ethernet: sending %d Bytes to %s, (qlen=%d)\n",
		     p->len, eth2ascii(p->dst), qlen);

	    ccnl_core_RX(relays + i, 0, (unsigned char*) p->data,
			p->len, &sun.sa, sizeof(sun.eth));
	} else {
	    DEBUGMSG(10, "simu_ethernet: dest %s not found\n",
		     eth2ascii(etherqueue->dst));
	}
	ccnl_free(p);
    }
    ccnl_set_timer(2000, ccnl_simu_ethernet, dummy, dummy2);
}

// ----------------------------------------------------------------------
// the two functions called by the CCNL's core and frag code:

int
ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    int i;
    char tmp[200], tmp2[10];
    struct ccnl_prefix_s *p = c->name;

    tmp[0] = '\0';
    for (i = 0; i < p->compcnt-1; i++) {
	strcat((char*)tmp, "/");
	tmp[strlen(tmp) + p->complen[i]] = '\0';
	memcpy(tmp + strlen(tmp), p->comp[i], p->complen[i]);
    }
    memcpy(tmp2, p->comp[p->compcnt-1], p->complen[p->compcnt-1]);
    tmp2[p->complen[p->compcnt-1]] = '\0';

    DEBUGMSG(10, "app delivery at node %c, name=%s%s\n",
	     relay2char(ccnl), tmp, tmp2);

    ccnl_simu_client_RX(ccnl, tmp, atoi(tmp2+1),
		       (char*) c->content, c->contentlen);
    return 0;
}


void
ccnl_ll_TX(struct ccnl_relay_s *relay, struct ccnl_if_s *ifc,
	   sockunion *dst, struct ccnl_buf_s *buf)
{
    struct ccnl_ethernet_s *p;
    int cnt;

    for (cnt = 0, p = etherqueue; p; p = p->next, cnt++);

    DEBUGMSG(29, "eth(simu)_ll_TX to %s len=%d (qlen=%d) [0x%02x 0x%02x]\n",
	     ccnl_addr2ascii(dst), buf->datalen, cnt,
	     buf->data[0], buf->data[1]);
    DEBUGMSG(29, "  src=%s\n", ccnl_addr2ascii(&ifc->addr));

    p = ccnl_calloc(1, sizeof(*p) + 2*ETH_ALEN + buf->datalen);
    p->dst = (unsigned char*)p + sizeof(*p);
    p->src = (unsigned char*)p + sizeof(*p) + ETH_ALEN;
    p->data = (unsigned char*)p + sizeof(*p) + 2*ETH_ALEN;
    p->len = buf->datalen;
    memcpy(p->dst, dst->eth.sll_addr, ETH_ALEN);
    p->protocol = dst->eth.sll_protocol;
    memcpy(p->src, ifc->addr.eth.sll_addr, ETH_ALEN);
    memcpy(p->data, buf->data, buf->datalen);

    // prepend
    p->next = etherqueue;
    etherqueue = p;
}

void
ccnl_close_socket(int s)
{
    close(s);
}

// ----------------------------------------------------------------------


void 
ccnl_simu_node_log_start(struct ccnl_relay_s *relay, char node)
{
    char name[100];
    struct ccnl_stats_s *st = relay->stats;

    if (!st)
	return;

    sprintf(name, "node-%c-sent.log", node);
    st->ofp_s = fopen(name, "w");
    if (st->ofp_s) {
        fprintf(st->ofp_s,"#####################\n");
        fprintf(st->ofp_s,"# sent_log Node %c\n", node);
        fprintf(st->ofp_s,"#####################\n");
        fprintf(st->ofp_s,"# time\t sent i\t\t sent c\t\t sent i+c\n");
	fflush(st->ofp_s);
    }
    sprintf(name, "node-%c-rcvd.log", node);
    st->ofp_r = fopen(name, "w");
    if (st->ofp_r) {
        fprintf(st->ofp_r,"#####################\n");
        fprintf(st->ofp_r,"# rcvd_log Node %c\n", node);
        fprintf(st->ofp_r,"#####################\n");
        fprintf(st->ofp_r,"# time\t recv i\t\t recv c\t\t recv i+c\n");
	fflush(st->ofp_r);
    }
    sprintf(name, "node-%c-qlen.log", node);
    st->ofp_q = fopen(name, "w");
    if (st->ofp_q) {
        fprintf(st->ofp_q,"#####################\n");
        fprintf(st->ofp_q,"# qlen_log Node %c (relay->ifs[0].qlen)\n", node);
        fprintf(st->ofp_q,"#####################\n");
        fprintf(st->ofp_q,"# time\t qlen\n");
	fflush(st->ofp_q);
    }
}


void
ccnl_print_stats(struct ccnl_relay_s *relay, int code)
{
    //code 1 = recv interest
    //code 2 = recv content
    //code 3 = sent interest
    //code 4 = sent content
    //code 5 = queue length
    //code 6 = end phaseOne
    struct ccnl_stats_s *st = relay->stats;
    double t = current_time();
    double tmp;

    if (!st || !st->ofp_r || !st->ofp_s || !st->ofp_q)
	    return;

    switch (code) {
    case STAT_RCV_I:
	if (st->log_recv_t_i < st->log_recv_t_c) {
	    tmp = st->log_recv_t_c;
	} else {
	    tmp = st->log_recv_t_i;
	} 
	fprintf(st->ofp_r, "%.4g\t %f\t %c\t %f\n",
		t, 1/(t - st->log_recv_t_i), '?', 1/(t-tmp));
	fflush(st->ofp_r);
	st->log_recv_t_i = t;
	break;
    case STAT_RCV_C:
	if (st->log_recv_t_i < st->log_recv_t_c) {
	    tmp = st->log_recv_t_c;
	} else {
	    tmp = st->log_recv_t_i;
	} 
	fprintf(st->ofp_r, "%.4g\t %c\t %f\t %f\n",
		t, '?', 1/(t - st->log_recv_t_c), 1/(t-tmp));
	fflush(st->ofp_r);
	st->log_recv_t_c = t;
    case STAT_SND_I:
	if (st->log_sent_t_i < st->log_sent_t_c) {
	    tmp = st->log_sent_t_c;
	} else {
	    tmp = st->log_sent_t_i;
	} 
	fprintf(st->ofp_s, "%.4g\t %f\t %c\t %f\n",
		t, 1/(t - st->log_sent_t_i), '?', 1/(t-tmp));
	fflush(st->ofp_s);
	st->log_sent_t_i = t;
	break;
    case STAT_SND_C:
	if (st->log_sent_t_i < st->log_sent_t_c) {
	    tmp = st->log_sent_t_c;
	} else {
	    tmp = st->log_sent_t_i;
	} 
	fprintf(st->ofp_s, "%.4g\t %c\t %f\t %f\n",
		t, '?', 1/(t - st->log_sent_t_c), 1/(t-tmp));
	fflush(st->ofp_s);
	st->log_sent_t_c = t;
	break;
    case STAT_QLEN:
	fprintf(st->ofp_q, "%.4g\t %i\n", t, relay->ifs[0].qlen);
	fflush(st->ofp_q);
	break;
    case STAT_EOP1: // end of phase 1
	fprintf(st->ofp_r, "%.4g\t %f\t %f\t %f\n", t, 0.0, 0.0, 0.0);
	fflush(st->ofp_r);
	break;
    default:
	break;
    }
}

void ccnl_simu_cleanup(void *dummy, void *dummy2);
#include "ccnl-simu-client.c"

// ----------------------------------------------------------------------

static int inter_ccn_interval = 0; // in usec
static int inter_packet_interval = 0; // in usec

#ifdef USE_SCHEDULER
struct ccnl_sched_s*
ccnl_simu_defaultFaceScheduler(struct ccnl_relay_s *ccnl,
				    void(*cb)(void*,void*))
{
    return ccnl_sched_pktrate_new(cb, ccnl, inter_ccn_interval);
}
#endif

void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

void
ccnl_simu_init_node(char node, const char *addr,
		    int max_cache_entries)
{
    struct ccnl_relay_s *relay = char2relay(node);
    struct ccnl_if_s *i;

    DEBUGMSG(99, "ccnl_simu_init_node\n");

    relay->id = relay - relays;
    relay->max_cache_entries = node == 'C' ? -1 : max_cache_entries;

    // add (fake) eth0 interface with index 0:
    i = &relay->ifs[0];
    i->addr.eth.sll_family = AF_PACKET;
    memcpy(i->addr.eth.sll_addr, addr, ETH_ALEN);
    //    i->mtu = 1400;
    i->mtu = 1200;
    i->reflect = 1;
    i->fwdalli = 1;
    relay->ifcount++;

#ifdef USE_SCHEDULER
    i->sched = ccnl_sched_pktrate_new(ccnl_interface_CTS, relay,
				      inter_packet_interval);
    relay->defaultFaceScheduler = ccnl_simu_defaultFaceScheduler;
#endif

    if (node == 'A' || node == 'B') {

	struct ccnl_client_s *client = ccnl_calloc(1,
					   sizeof(struct ccnl_client_s));
	client->lastseq = SIMU_NUMBER_OF_CHUNKS-1;
	client->last_received = -1;
	client->name = node == 'A' ?
	    "/ccnl/simu/movie1" : "/ccnl/simu/movie2";
	relay->aux = (void *) client;

	relay->stats = ccnl_calloc(1, sizeof(struct ccnl_stats_s));
    }

    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
    ccnl_simu_node_log_start(relay, node);
}


void
ccnl_simu_add_fwd(char node, const char *name, char dstnode)
{
    struct ccnl_relay_s *relay = char2relay(node), *dst = char2relay(dstnode);
    struct ccnl_forward_s *fwd;
    sockunion sun;

    DEBUGMSG(99, "ccnl_simu_add_fwd\n");

    sun.eth.sll_family = AF_PACKET;
    memcpy(sun.eth.sll_addr, dst->ifs[0].addr.eth.sll_addr, ETH_ALEN);
    fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
    fwd->prefix = ccnl_path_to_prefix(name);
    fwd->face = ccnl_get_face_or_create(relay, 0, &sun.sa, sizeof(sun.eth));
#ifdef USE_FRAG
    //    fwd->face->frag = ccnl_frag_new(CCNL_FRAG_SEQUENCED2012, 1500);
    fwd->face->frag = ccnl_frag_new(CCNL_FRAG_CCNx2013, 1200);
#endif
    fwd->face->flags |= CCNL_FACE_FLAGS_STATIC;
    fwd->next = relay->fib;
    relay->fib = fwd;
}


int
ccnl_simu_init(int max_cache_entries)
{
    static char dat[SIMU_CHUNK_SIZE];
    static char init_was_visited;
    int i;

    if (init_was_visited)
	return 0;
    init_was_visited = 1;

    DEBUGMSG(99, "ccnl_simu_init\n");

 #ifdef USE_SCHEDULER
    // initialize the scheduling subsystem
    ccnl_sched_init();
#endif

    // define each node's eth address:
    ccnl_simu_init_node('A', "\x00\x00\x00\x00\x00\x0a",
		       max_cache_entries);
    ccnl_simu_init_node('B', "\x00\x00\x00\x00\x00\x0b",
		       max_cache_entries);
    ccnl_simu_init_node('C', "\x00\x00\x00\x00\x00\x0c",
		       max_cache_entries);
    ccnl_simu_init_node('1', "\x00\x00\x00\x00\x00\x01",
		       max_cache_entries);
    ccnl_simu_init_node('2', "\x00\x00\x00\x00\x00\x02",
		       max_cache_entries);

    // install the system's forwarding pointers:
    ccnl_simu_add_fwd('A', "/ccnl/simu", '2');
    ccnl_simu_add_fwd('B', "/ccnl/simu", '2');
    ccnl_simu_add_fwd('2', "/ccnl/simu", '1');
    ccnl_simu_add_fwd('1', "/ccnl/simu", 'C');

/*
    for (i = 0; i < 5; i++)
	ccnl_dump(0, CCNL_RELAY, relays+i);
*/

    // turn node 'C' into a repository for three movies
    sprintf(dat, "%d", (int) sizeof(dat));
    for (i = 0; i < SIMU_NUMBER_OF_CHUNKS; i++) {
	ccnl_simu_add2cache('C', "/ccnl/simu/movie1", i, dat, sizeof(dat));
	ccnl_simu_add2cache('C', "/ccnl/simu/movie2", i, dat, sizeof(dat));
	ccnl_simu_add2cache('C', "/ccnl/simu/movie3", i, dat, sizeof(dat));
    }

    ccnl_set_timer(5000, ccnl_simu_ethernet, 0, 0);

    // start clients:
    ccnl_set_timer( 500000, ccnl_simu_client_start, char2relay('A'), 0);
    ccnl_set_timer(1000000, ccnl_simu_client_start, char2relay('B'), 0);
    phaseOne = 1;

    printf("Press ENTER to start the simulation\n");
    while (getchar() != '\n');

    return 0;
}

void
ccnl_simu_phase_two(void *ptr, void *dummy)
{
    phaseOne = 0;
    ccnl_set_timer(0, ccnl_simu_client_start, char2relay('A'), 0);
    ccnl_set_timer(500000, ccnl_simu_client_start, char2relay('B'), 0);
}


// ----------------------------------------------------------------------

void
simu_eventloop()
{ 
    static struct timeval now;
    long usec;

    while (eventqueue) {
	struct ccnl_timer_s *t = eventqueue;
	// printf("  looping now %g\n", CCNL_NOW());
	gettimeofday(&now, 0);
	usec = timevaldelta(&(t->timeout), &now);
	if (usec >= 0) {
	    // usleep(usec);
	    struct timespec ts;
	    ts.tv_sec = usec / 1000000;
	    ts.tv_nsec = 1000 * (usec % 1000000);
	    nanosleep(&ts, NULL);
	}
	t = eventqueue;
	eventqueue = t->next;
	if (t->fct) {
	    void (*fct)(char,int) = t->fct;
	    char c = t->node;
	    int i = t->intarg;
	    ccnl_free(t);
	    (fct)(c, i);
	} else if (t->fct2) {
	    void (*fct2)(void*,void*) = t->fct2;
	    void *aux1 = t->aux1;
	    void *aux2 = t->aux2;
	    ccnl_free(t);
	    (fct2)(aux1, aux2);
	}
    }
    DEBUGMSG(0, "simu event loop: no more events to handle\n");
}

#define end_log(CHANNEL) do { if (CHANNEL){ \
  fprintf(CHANNEL, "#end\n"); \
  fclose(CHANNEL); \
  CHANNEL = NULL; \
} } while(0)

void
ccnl_simu_cleanup(void *dummy, void *dummy2)
{
    printf("Simulation ended; press ENTER to terminate\n");
    while (getchar() != '\n');

    int i;
    for (i = 0; i < 5; i++) {
	struct ccnl_relay_s *relay = relays + i;

    if (relay->aux) {
        ccnl_free(relay->aux);
        relay->aux = NULL;
    }

	if (relay->stats) {
	    end_log(relay->stats->ofp_s);
	    end_log(relay->stats->ofp_r);
	    end_log(relay->stats->ofp_q);
	    ccnl_free(relay->stats);
	    relay->stats = NULL;
	}
	ccnl_core_cleanup(relay);
    }

    while(eventqueue)
	ccnl_rem_timer(eventqueue);

    while(etherqueue) {
	struct ccnl_ethernet_s *e = etherqueue->next;
	ccnl_free(etherqueue);
	etherqueue = e;
    }

#ifdef USE_SCHEDULER
    ccnl_sched_cleanup();
#endif
#ifdef USE_DEBUG_MALLOC
    debug_memdump();
#endif
}

// ----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int opt;
    int max_cache_entries = CCNL_DEFAULT_MAX_CACHE_ENTRIES;

    //    srand(time(NULL));
    srandom(time(NULL));

    while ((opt = getopt(argc, argv, "hc:g:i:s:v:")) != -1) {
        switch (opt) {
        case 'c':
            max_cache_entries = atoi(optarg);
            break;
        case 'g':
            inter_packet_interval = atoi(optarg);
            break;
        case 'i':
            inter_ccn_interval = atoi(optarg);
            break;
        case 's':
            suite = atoi(optarg);
            break;
        case 'v':
            debug_level = atoi(optarg);
            break;
        case 'h':
        default:
            fprintf(stderr, "usage: %s [-h] [-c MAX_CONTENT_ENTRIES] "
		    "[-g MIN_INTER_PACKET_INTERVAL] "
		    "[-i MIN_INTER_CCNMSG_INTERVAL] "
#ifdef USE_SUITE_NDNTLV
		    "[-s SUITE (0=ccnb, 2=ndntlv)] "
#endif
		    "[-v DEBUG_LEVEL]\n",
		    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    time(&relays[0].startup_time);
    DEBUGMSG(1, "This is ccn-lite-simu, starting at %s",
	     ctime(&relays[0].startup_time) + 4);
    DEBUGMSG(1, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(1, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(1, "  compile options: %s\n", compile_string());

    ccnl_simu_init(max_cache_entries);

    DEBUGMSG(1, "simulation starts\n");
    simu_eventloop();
    DEBUGMSG(1, "simulation ends\n");

    return -1;
}


// eof
