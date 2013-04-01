/*
 * @f ccnl-simu-client.c
 * @b a multi-thread CCN lite client built for simulating CCN traffic
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
 * 2011-12-04 created
 * 2011-12 simulation scenario and logging support s.braun@stud.unibas.ch
 * 2013-03-19 updated (ms): replacement code after renaming the field
 *		ccnl_relay_s.client to ccnl_relay_s.aux
 */


#define MAX_PIPELINE	4
#define TIMEOUT		500000

#define LOG_INTERVAL    100000


struct ccnl_client_s {
    char *name;  // media URI to be downloaded
    int nextseq; // first seq# for which no interest as been sent now
    int lastseq; // media content's last seq# (=size of the media - 1)
    void* to_handlers[MAX_PIPELINE];
    int onthefly[MAX_PIPELINE];
    unsigned int nonces[MAX_PIPELINE];
    int threadcnt;
    int last_received;
    int retries, outofseq;
};

int pending_client_tasks;
int phaseOne;

void
ccnl_simu_client_timeout(void *ptr, void *aux)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) ptr;
    struct ccnl_client_s *cl = relay->aux;
    char node = relay2char(relay);
    void **vpp = (void**) aux;
    int ndx;

    cl->retries++;
    if (relay->stats)
	relay->stats->log_need_rt_seqn++;
    ndx = vpp - cl->to_handlers;
    DEBUGMSG(99, "*** node %c timeout: new timeout for ndx%d, seqno=%d\n",
	     node, ndx, cl->onthefly[ndx]);
    *vpp = ccnl_set_timer(TIMEOUT, ccnl_simu_client_timeout, ptr, aux);
    cl->nonces[ndx] = random();
    ccnl_client_TX(node, cl->name, cl->onthefly[ndx], cl->nonces[ndx]);
}


void
ccnl_simu_client_logger(void *ptr, void *dummy)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) ptr;
    struct ccnl_client_s *cl = relay->aux;
    struct ccnl_stats_s *st = relay->stats;

    if (st->ofp) {
        // calc content delivery rate
        if(current_time() - st->log_cdr_t < 1){
	    st->log_content_delivery_rate_per_s +=
		st->log_recv - st->log_recv_old;
        } else {
	    st->log_cdr_t = current_time();
	    st->log_content_delivery_rate_per_s +=
		st->log_recv_old - st->log_recv_old;
        }

        fprintf(st->ofp, "%.4g\t %i\t %i\t %i\t %i\t %i\t %i\t %i\n",
                current_time(), st->log_sent, st->log_recv,
		st->log_sent - st->log_sent_old,
		st->log_recv - st->log_recv_old,
		cl->last_received, st->log_need_rt_seqn,
		st->log_content_delivery_rate_per_s);
        fflush(st->ofp);
        st->log_sent_old = st->log_sent;
        st->log_recv_old = st->log_recv;
        st->log_need_rt_seqn = -1;
    }
    // re-install the log_timoeout
    st->log_handler = ccnl_set_timer(LOG_INTERVAL,
			       ccnl_simu_client_logger, relay, dummy);
}


void
ccnl_simu_client_endlog(struct ccnl_relay_s *relay)
{
    struct ccnl_stats_s *st = relay->stats;

    if (st && st->ofp) {
        fprintf(st->ofp, "\n#Total transmission time [s]: %.4g",
		current_time() - st->log_start_t);
        fprintf(st->ofp, "\n#Avg content delivery rate [c/s]: %.4g",
		st->log_recv / (current_time() - st->log_start_t));
	fprintf(st->ofp, "\n");
        fclose(st->ofp);
	st->ofp =  NULL;
    }
}


void
ccnl_simu_client_RX(struct ccnl_relay_s *relay, char *name,
		   int seqn, char *data, int len) // receiving side
{
    char node = relay2char(relay);
    struct ccnl_client_s *cl = relay->aux;
    int i;

    DEBUGMSG(1, "*** node %c received %d content bytes for name %s, seq=%d\n",
	   node, len, name, seqn);

    if (strcmp(name, cl->name)) {
	DEBUGMSG(1, "*** content name does not match our request!!\n"
		    "    <%s> instead of <%s>\n", name, cl->name);
	return;
    }

    for (i = 0; i < MAX_PIPELINE; i++) {
	if (cl->onthefly[i] == seqn)
	    break;
    }
    if (i == MAX_PIPELINE) // must be old content, ignore
	return;

    if (relay->stats)
	relay->stats->log_recv++;

    if (seqn != (cl->last_received+1)) {
	DEBUGMSG(6, "*** content %d out of sequence (expected %d)\n",
		 seqn, cl->last_received+1);
	cl->outofseq++;
    }
    cl->last_received = seqn;

    if (cl->to_handlers[i]) {
	ccnl_rem_timer(cl->to_handlers[i]);
	cl->to_handlers[i] = 0;
    }

    if (cl->nextseq > cl->lastseq) {
	DEBUGMSG(1, "*** node %c, client thread %d terminated, %d remaining\n",
		 node, i, cl->threadcnt-1);
	cl->onthefly[i] = -1;
	cl->threadcnt--;
	if (cl->threadcnt <= 0) {
	    pending_client_tasks--;
	    DEBUGMSG(1, "task for node %c ended, %d retransmit(s), %d outofseq /%d\n",
		     node, cl->retries, cl->outofseq, pending_client_tasks);
	    if (pending_client_tasks <= 0) {
		if (phaseOne == 1){
		    DEBUGMSG(1, "Enter Phase-TWO (by node %c)\n\n", node);
		    //make a fake zero log
		    ccnl_print_stats(relay, STAT_EOP1);
		    ccnl_set_timer(500000, ccnl_simu_phase_two, 0, 0);
		} else {
		    DEBUGMSG(1, "all tasks of phase two terminated\n");
		    ccnl_simu_client_endlog(relay);
		    for (i = 0; i < MAX_PIPELINE; i++) {
			if (cl->to_handlers[i]) {
			    ccnl_rem_timer(cl->to_handlers[i]);
			    cl->to_handlers[i] = 0;
			}
		    }
		    // wait a little before cleaning up
		    // (e.g. eth drive to drain its queue)
		    ccnl_set_timer(2000000, ccnl_simu_cleanup, 0, 0);
		}
	    }
	}
	return;
    }

    cl->onthefly[i] = cl->nextseq;
    cl->nextseq++;
    cl->nonces[i] = random();
    cl->to_handlers[i] = ccnl_set_timer(TIMEOUT, ccnl_simu_client_timeout,
					relay, cl->to_handlers + i);
    ccnl_client_TX(node, cl->name, cl->onthefly[i], cl->nonces[i]);
}


void
ccnl_simu_client_kick(char node, int ndx)
{
    struct ccnl_relay_s *relay = char2relay(node);
    struct ccnl_client_s *cl = relay->aux;

    if (cl->nextseq > cl->lastseq)
	return;

    if (!cl->to_handlers[ndx]) {
	cl->onthefly[ndx] = cl->nextseq;
	cl->nextseq++;
	cl->nonces[ndx] = random();
	cl->to_handlers[ndx] = ccnl_set_timer(TIMEOUT, ccnl_simu_client_timeout,
					     relay, cl->to_handlers + ndx);
	cl->threadcnt++;
//	printf("number of threads: %d\n", cl->threadcnt);
	ccnl_client_TX(node, cl->name, cl->onthefly[ndx], cl->nonces[ndx]);
    }
}


void
ccnl_simu_client_start(void *ptr, void *dummy)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) ptr;
    struct ccnl_stats_s *st = relay->stats;
    char node = relay2char(relay);
    int i;

    if (!st)
	return;
    if (phaseOne == 1) {
	char tmp[100];

	// write the log here
	sprintf(tmp, "node-%c-client.log", node);
	st->ofp = fopen(tmp, "w");
	if (st->ofp) {
	    fprintf(st->ofp, "#############################\n");
	    fprintf(st->ofp, "# client_log Node %c \n", node);
	    fprintf(st->ofp, "# Retransmission timeout: %i \n", TIMEOUT);
	    fprintf(st->ofp, "# Max pipelining threads: %i \n", MAX_PIPELINE);
	    fprintf(st->ofp, "#############################\n");
	    fprintf(st->ofp, "#time\t p sent\t p recv\t dlta_s\t dlta_r\t lc_rcv\t retra\t cdr\n");
	    fflush(st->ofp);
	}
	ccnl_simu_client_logger(relay, 0);
    } else {
	struct ccnl_client_s *cl = relay->aux;
	cl->nextseq = 0;
	cl->name = node == 'A' ? "/ccnl/simu/movie2" : "/ccnl/simu/movie3";
    }
    pending_client_tasks++;

    // kick the client by installing MAX_PIPELINE threads
    for (i = 0; i < MAX_PIPELINE; i++)
	ccnl_simu_client_kick(node, i);
}

// eof
