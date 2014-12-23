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
 *              ccnl_relay_s.client to ccnl_relay_s.aux
 * 2014-12-18 removed log generation (cft)
 */


#define MAX_PIPELINE    4
#define TIMEOUT         500000

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
    ndx = vpp - cl->to_handlers;
    DEBUGMSG(WARNING, "*** node %c timeout: new timeout for ndx%d, seqno=%d\n",
             node, ndx, cl->onthefly[ndx]);
    *vpp = ccnl_set_timer(TIMEOUT, ccnl_simu_client_timeout, ptr, aux);
    cl->nonces[ndx] = random();
    ccnl_client_TX(node, cl->name, cl->onthefly[ndx], cl->nonces[ndx]);
}

void
ccnl_simu_client_RX(struct ccnl_relay_s *relay, char *name,
                   int seqn, char *data, int len) // receiving side
{
    char node = relay2char(relay);
    struct ccnl_client_s *cl = relay->aux;
    int i;

    DEBUGMSG(INFO, "*** node %c received %d content bytes for name %s, seq=%d\n",
           node, len, name, seqn);

    if (strcmp(name, cl->name)) {
        DEBUGMSG(WARNING, "*** content name does not match our request!!\n"
                    "    <%s> instead of <%s>\n", name, cl->name);
        return;
    }

    for (i = 0; i < MAX_PIPELINE; i++) {
        if (cl->onthefly[i] == seqn)
            break;
    }
    if (i == MAX_PIPELINE) // must be old content, ignore
        return;

    if (seqn != (cl->last_received+1)) {
        DEBUGMSG(INFO, "*** content %d out of sequence (expected %d)\n",
                 seqn, cl->last_received+1);
        cl->outofseq++;
    }
    cl->last_received = seqn;

    if (cl->to_handlers[i]) {
        ccnl_rem_timer(cl->to_handlers[i]);
        cl->to_handlers[i] = 0;
    }

    if (cl->nextseq > cl->lastseq) {
        DEBUGMSG(INFO, "*** node %c, client thread %d terminated, %d remaining\n",
                 node, i, cl->threadcnt-1);
        cl->onthefly[i] = -1;
        cl->threadcnt--;
        if (cl->threadcnt <= 0) {
            pending_client_tasks--;
            DEBUGMSG(INFO, "task for node %c ended, %d retransmit(s), %d outofseq /%d\n",
                     node, cl->retries, cl->outofseq, pending_client_tasks);
            if (pending_client_tasks <= 0) {
                if (phaseOne == 1){
                    DEBUGMSG(INFO, "Enter Phase-TWO (by node %c)\n\n", node);
                    //make a fake zero log
                    ccnl_set_timer(500000, ccnl_simu_phase_two, 0, 0);
                } else {
                    DEBUGMSG(INFO, "all tasks of phase two terminated\n");
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
//      printf("number of threads: %d\n", cl->threadcnt);
        ccnl_client_TX(node, cl->name, cl->onthefly[ndx], cl->nonces[ndx]);
    }
}


void
ccnl_simu_client_start(void *ptr, void *dummy)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) ptr;
    char node = relay2char(relay);
    int i;

    if (phaseOne != 1) {
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
