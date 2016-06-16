/*
 * @f ccnl-ext-nfn.c
 * @b CCN-lite, NFN related routines
 *
 * Copyright (C) 2014, Christopher Scherb, University of Basel
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
 * 2014-02-06 <christopher.scherb@unibas.ch>created
 */

#ifdef USE_NFN

#include "ccnl-core.h"
#include "ccnl-ext-nfn.h"

struct builtin_s *op_extensions;
struct builtin_s bifs[];

#include "ccnl-ext-nfncommon.c"
#include "ccnl-ext-nfnparse.c"
#include "ccnl-ext-nfnkrivine.c"
#include "ccnl-ext-nfnops.c"

void
ZAM_init(void)
{
}

struct configuration_s*
ccnl_nfn_findConfig(struct configuration_s *config_list, int configid)
{
    struct configuration_s *config;

    for (config = config_list; config; config = config->next)
        if(config->configid == configid)
            return config;

    return NULL;
}

void
ccnl_nfn_continue_computation(struct ccnl_relay_s *ccnl, int configid, int continue_from_remove){
    DEBUGMSG(TRACE, "ccnl_nfn_continue_computation()\n");
    struct configuration_s *config = ccnl_nfn_findConfig(ccnl->km->configuration_list, -configid);

    if(!config){
        DEBUGMSG(DEBUG, "nfn_continue_computation: %d not found\n", configid);
        return;
    }

    //update original interest prefix to stay longer...reenable if propagate=0 do not protect interests
    struct ccnl_interest_s *original_interest;
    for(original_interest = ccnl->pit; original_interest; original_interest = original_interest->next){
        if(!ccnl_prefix_cmp(config->prefix, 0, original_interest->pkt->pfx, CMP_EXACT)){
            original_interest->last_used = CCNL_NOW();
            original_interest->retries = 0;
            original_interest->from->last_used = CCNL_NOW();
            break;
        }
    }
    ccnl_nfn(ccnl, NULL, NULL, config, NULL, 0, 0);
    TRACEOUT();
}

void
ccnl_nfn_nack_local_computation(struct ccnl_relay_s *ccnl,
                                struct ccnl_buf_s *orig,
                                struct ccnl_prefix_s *prefix,
                                struct ccnl_face_s *from,
                                int suite)
{
    DEBUGMSG(TRACE, "ccnl_nfn_nack_local_computation\n");

    ccnl_nfn(ccnl, prefix, from, NULL, NULL, suite, 1);
    TRACEOUT();
}

int
ccnl_nfn_already_computing(struct ccnl_relay_s *ccnl,
                                 struct ccnl_prefix_s *prefix)
{
    int i = 0;
    struct ccnl_prefix_s *copy;

    DEBUGMSG(TRACE, "ccnl_nfn_already_computing()\n");

    copy = ccnl_prefix_dup(prefix);
    ccnl_nfnprefix_set(copy, CCNL_PREFIX_NFN);
#ifdef USE_TIMEOUT_KEEPALIVE
    ccnl_nfnprefix_clear(copy, CCNL_PREFIX_KEEPALIVE);
#endif

    for (i = 0; i < -ccnl->km->configid; ++i) {
        struct configuration_s *config;

        config = ccnl_nfn_findConfig(ccnl->km->configuration_list, -i);
        if (!config)
            continue;
        if (!ccnl_prefix_cmp(config->prefix, NULL, copy, CMP_EXACT)) {
            free_prefix(copy);
            return 1;
        }
    }
    free_prefix(copy);

    return 0;
}

int
ccnl_nfn(struct ccnl_relay_s *ccnl, // struct ccnl_buf_s *orig,
         struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
         struct configuration_s *config, struct ccnl_interest_s *interest,
         int suite, int start_locally)
{
    struct ccnl_buf_s *res = NULL;
    char str[CCNL_MAX_PACKET_SIZE];
    int i, len = 0;

    DEBUGMSG(TRACE, "ccnl_nfn(%p, %s, %p, config=%p)\n",
             (void*)ccnl, ccnl_prefix_to_path(prefix),
             (void*)from, (void*)config);

    //    prefix = ccnl_prefix_dup(prefix);
    DEBUGMSG(DEBUG, "Namecomps: %s \n", ccnl_prefix_to_path(prefix));

    if (config){
        suite = config->suite;
        goto restart;
    }

    from->flags = CCNL_FACE_FLAGS_STATIC;

    if (ccnl_nfn_already_computing(ccnl, prefix)) { //TODO REMOVE?
        DEBUGMSG(DEBUG, "Computation for this interest is already running\n");
        return -1;
    }

    // Checks first if the interest has a routing hint and then searches for it locally.
    // If it exisits, the computation is started locally,  otherwise it is directly forwarded without entering the AM.
    // Without this mechanism, there will be situations where several nodes "overtake" a computation
    // applying the same strategy and, potentially, all executing it locally (after trying all arguments).
    // TODO: this is not an elegant solution and should be improved on, because the clients cannot send a
    // computation with a routing hint on which the network applies a strategy if the routable name
    // does not exist (because each node will just forward it without ever taking it into an abstract machine).
    // encoding the routing hint more explicitely as well as additonal information (e.g. already tried names)
    // could solve the problem. More generally speaking, additional state describing the exact situation will be required.

    if (interest && interest->pkt->pfx->compcnt > 1) { // forward interests with outsourced components
        struct ccnl_prefix_s *copy = ccnl_prefix_dup(prefix);
	
        copy->compcnt -= 1;
        DEBUGMSG(DEBUG, "   checking local available of %s\n", ccnl_prefix_to_path(copy));
        ccnl_nfnprefix_clear(copy, CCNL_PREFIX_NFN);
        if (!ccnl_nfn_local_content_search(ccnl, NULL, copy)) {
            free_prefix(copy);
            ccnl_interest_propagate(ccnl, interest);
            return 0;
        }
        free_prefix(copy);
        start_locally = 1;
    }

    //put packet together
#if defined(USE_SUITE_CCNTLV) || defined(USE_SUITE_CISTLV)
    if (prefix->suite == CCNL_SUITE_CCNTLV ||
                                        prefix->suite == CCNL_SUITE_CISTLV) {
        len = prefix->complen[prefix->compcnt-1] - 4;
        memcpy(str, prefix->comp[prefix->compcnt-1] + 4, len);
        str[len] = '\0';
    } else
#endif
    {
        len = prefix->complen[prefix->compcnt-1];
        memcpy(str, prefix->comp[prefix->compcnt-1], len);
        str[len] = '\0';
    }
    if (prefix->compcnt > 1)
        len += sprintf(str + len, " ");
    for (i = 0; i < prefix->compcnt-1; i++) {
#if defined(USE_SUITE_CCNTLV) || defined(USE_SUITE_CISTLV)
        if (prefix->suite == CCNL_SUITE_CCNTLV ||
                                      prefix->suite == CCNL_SUITE_CISTLV)
            len += sprintf(str+len,"/%.*s",prefix->complen[i]-4,prefix->comp[i]+4);
        else
#endif
            len += sprintf(str+len,"/%.*s",prefix->complen[i],prefix->comp[i]);
    }

    DEBUGMSG(DEBUG, "expr is <%s>\n", str);
    //search for result here... if found return...

    ++ccnl->km->numOfRunningComputations;
restart:
    res = Krivine_reduction(ccnl, str, start_locally, &config, prefix, suite);

    //stores result if computed
    if (res && res->datalen > 0) {
        struct ccnl_prefix_s *copy;
        struct ccnl_content_s *c;

        DEBUGMSG(INFO,"Computation finished: res: %.*s size: %d bytes. Running computations: %d\n",
                 (int) res->datalen, res->data, (int) res->datalen, ccnl->km->numOfRunningComputations);

        copy = ccnl_prefix_dup(config->prefix);
        c = ccnl_nfn_result2content(ccnl, &copy, res->data, res->datalen);
        c->flags = CCNL_CONTENT_FLAGS_STATIC;

        set_propagate_of_interests_to_1(ccnl, c->pkt->pfx);
        ccnl_content_serve_pending(ccnl,c);
        ccnl_content_add2cache(ccnl, c);
        --ccnl->km->numOfRunningComputations;

        DBL_LINKED_LIST_REMOVE(ccnl->km->configuration_list, config);
        ccnl_nfn_freeConfiguration(config);
        ccnl_free(res);
    }
#ifdef USE_NACK
    else if(config->local_done){
        struct ccnl_content_s *nack;
        nack = ccnl_nfn_result2content(ccnl, &config->prefix,
                                       (unsigned char*)":NACK", 5);
        ccnl_content_serve_pending(ccnl, nack);

    }
#endif

    TRACEOUT();
    return 0;
}

struct ccnl_interest_s*
ccnl_nfn_RX_request(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
                     struct ccnl_pkt_s **pkt)
{
    struct ccnl_interest_s *i;

    if (!ccnl_nfnprefix_isNFN((*pkt)->pfx) || 
#ifdef USE_TIMEOUT_KEEPALIVE
        ccnl_nfnprefix_isKeepalive((*pkt)->pfx) ||
#endif
        ccnl->km->numOfRunningComputations >= NFN_MAX_RUNNING_COMPUTATIONS)
        return NULL;
    i = ccnl_interest_new(ccnl, from, pkt);
    if (!i)
        return NULL;
    i->flags &= ~CCNL_PIT_COREPROPAGATES; // do not forward interests for running computations
    ccnl_interest_append_pending(i, from);
//    if (!(i->flags & CCNL_PIT_COREPROPAGATES))
    ccnl_nfn(ccnl, ccnl_prefix_dup(i->pkt->pfx), from, NULL, i, i->pkt->suite, 0);

    TRACEOUT();
    return i;
}


int
ccnl_nfn_RX_result(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_content_s *c)
{
    struct ccnl_interest_s *i_it = NULL;
    int found = 0;

    DEBUGMSG_CFWD(INFO, "data in rx result %.*s\n", c->pkt->contlen, c->pkt->content);
    TRACEIN();
#ifdef USE_NACK
    if (ccnl_nfnprefix_contentIsNACK(c)) {
        ccnl_nfn_nack_local_computation(relay, c->pkt->buf, c->pkt->pfx,
                                        from, c->pkt->pfx->suite);
        return -1;
    }
#endif // USE_NACK
    for (i_it = relay->pit; i_it;/* i_it = i_it->next*/) {
        //Check if prefix match and it is a nfn request
        if (!ccnl_prefix_cmp(c->pkt->pfx, NULL, i_it->pkt->pfx, CMP_EXACT) &&
                                        i_it->from && i_it->from->faceid < 0) {
            struct ccnl_face_s *from = i_it->from;
            int faceid = - from->faceid;

            DEBUGMSG(TRACE, "  interest faceid=%d\n", i_it->from->faceid);

#ifdef USE_TIMEOUT_KEEPALIVE
            if (!ccnl_nfnprefix_isKeepalive(c->pkt->pfx)) {
#endif
                ccnl_content_add2cache(relay, c);
#ifdef USE_TIMEOUT_KEEPALIVE
            }
#endif
            
	        DEBUGMSG_CFWD(INFO, "data in rx resulti after add to cache %.*s\n", c->pkt->contlen, c->pkt->content);
            DEBUGMSG(DEBUG, "Continue configuration for configid: %d with prefix: %s\n",
                  faceid, ccnl_prefix_to_path(c->pkt->pfx));
            i_it->flags |= CCNL_PIT_COREPROPAGATES;
            i_it->from = NULL;
            
#ifdef USE_TIMEOUT_KEEPALIVE
            if (!ccnl_nfnprefix_isKeepalive(c->pkt->pfx)) {
#endif
                ccnl_nfn_continue_computation(relay, faceid, 0);
#ifdef USE_TIMEOUT_KEEPALIVE
             }
#endif // USE_TIMEOUT_KEEPALIVE
            i_it = ccnl_interest_remove(relay, i_it);
            //ccnl_face_remove(relay, from);
            ++found;
            //goto Done;
        } else
            i_it = i_it->next;
    }
    TRACEOUT();
    return found > 0;
}

#ifdef USE_TIMEOUT_KEEPALIVE
int
ccnl_nfn_RX_keepalive(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      struct ccnl_content_s *c)
{
    TRACEIN();
    DEBUGMSG(DEBUG, "  RX keepalive <%s>\n", ccnl_prefix_to_path(c->pkt->pfx));
    struct ccnl_interest_s *i_it = NULL;
    int found = 0;
    for (i_it = relay->pit; i_it; i_it = i_it->next) {
        if (!ccnl_prefix_cmp(c->pkt->pfx, NULL, i_it->pkt->pfx, CMP_EXACT)) {
            DEBUGMSG(DEBUG, "    matches interest <%s>\n", ccnl_prefix_to_path(i_it->pkt->pfx));
            if (i_it->keepalive_origin != NULL) {
                DEBUGMSG(DEBUG, "      reset interest <%s>\n", 
                    ccnl_prefix_to_path(i_it->keepalive_origin->pkt->pfx));
                
                // reset original interest
                i_it->keepalive_origin->last_used = CCNL_NOW();
                i_it->keepalive_origin->retries = 0;

                // remove keepalive interest
                i_it->keepalive_origin->keepalive = NULL;
                i_it->keepalive_origin = NULL;
                // i_it = ccnl_nfn_interest_remove(relay, i_it);
                
                ++found;
            }
        }
    }
    DEBUGMSG(DEBUG, "  keeping %i interest%s alive\n", found, found == 1 ? "" : "s");
    TRACEOUT();
    return found > 0;
}
#endif

#endif //USE_NFN


// eof
