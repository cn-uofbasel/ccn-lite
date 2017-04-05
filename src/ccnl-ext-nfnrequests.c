/*
 * @f ccnl-ext-nfncommon.c
 * @b CCN-lite, execution/state management of running computations
 *
 * Copyright (C) 2016, Balazs Faludi, University of Basel
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
 * 2016-02-10 created
 */


#ifdef USE_NFN_REQUESTS

char *nfn_request_names[NFN_REQUEST_TYPE_MAX] = {
    "START",
    "PAUSE",
    "RESUME",
    "CANCEL",
    "STATUS",
    "KEEPALIVE",
    "CIM",
    "GIM"};

struct ccnl_prefix_s* ccnl_prefix_dup(struct ccnl_prefix_s *prefix);
void ccnl_nfnprefix_set(struct ccnl_prefix_s *p, unsigned int flags);
void ccnl_nfnprefix_clear(struct ccnl_prefix_s *p, unsigned int flags);
struct configuration_s* ccnl_nfn_find_running_computation(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix);

// struct ccnl_prefix_s* ccnl_nfnprefix_mkComputePrefix(struct configuration_s *config, int suite);
// struct configuration_s* ccnl_nfn_find_running_computation(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix);

struct nfn_request_s*
nfn_request_new(unsigned char *comp, int complen)
{
    struct nfn_request_s *request = 
        (struct nfn_request_s *) ccnl_calloc(1, sizeof(struct nfn_request_s));

    DEBUGMSG_CORE(TRACE, "nfn_request_new(%.*s)\n", complen, comp);

    if (!request)
        return NULL;

    request->complen = complen;
    request->comp = ccnl_malloc(complen);
    memcpy(request->comp, comp, complen);

    request->type = NFN_REQUEST_TYPE_UNKNOWN;
    char *request_name = NULL;
    int request_len = 0;

    int i;
    for (i = 0; i < NFN_REQUEST_TYPE_MAX; i++) {
        request_name = nfn_request_names[i];
        request_len = strlen(request_name);
        if (request_len <= complen && strncmp(request_name, (char *)comp, request_len) == 0) {
            request->type = (enum nfn_request_type)i+1; // 0 is "unknown"
            break;
        }
    }

    if (request->type == NFN_REQUEST_TYPE_UNKNOWN) {
        DEBUGMSG_CORE(DEBUG, "Unknown request: %.*s\n", complen, comp);
        return request;
    }

    request->arg = NULL;
    if (complen >= request_len + 2 && comp[request_len] == ' ') {
        int arglen = complen - request_len - 1;
        // request->arg = request->comp + request_len + 1;
        request->arg = ccnl_malloc(arglen + 1);
        strncpy(request->arg, (char *)comp + request_len + 1, arglen);
        request->arg[arglen] = '\0';
    }

    return request;
}

struct nfn_request_s*
nfn_request_copy(struct nfn_request_s *request)
{
    if (request == NULL) {
        return NULL;
    } 
    return nfn_request_new(request->comp, request->complen);
}

void 
nfn_request_free(struct nfn_request_s *request)
{
    if (request) {
        if (request->comp) {
            ccnl_free(request->comp);
        }
        if (request->arg) {
            ccnl_free(request->arg);
        }
        ccnl_free(request);
    }
}

int 
nfn_request_get_arg_int(struct nfn_request_s* request)
{
    // TODO: verify arg, error handling
    // TODO: add arg index as parameter
    return strtol(request->arg, NULL, 0);
}

void // TODO: is this still needed?
nfn_request_set_arg_int(struct nfn_request_s* request, int arg)
{
    if (request->arg) {
        ccnl_free(request->arg);
    }
    int arglen = snprintf(NULL, 0, "%d", arg);
    request->arg = ccnl_malloc(arglen+1);
    sprintf(request->arg, "%d", arg);
}

void // TODO: is this still needed?
nfn_request_update_component(struct nfn_request_s *request)
{
    if (request->comp) {
        ccnl_free(request->comp);
    }
    char *command = nfn_request_names[request->type];
    int commandlen = strlen(command);
    int arglen = strlen(request->arg);
    request->complen = commandlen + 1 + arglen;
    request->comp = ccnl_malloc(request->complen);
    memcpy(request->comp, command, commandlen);
    request->comp[commandlen] = (unsigned char)" ";
    memcpy(request->comp + commandlen + 1, request->arg, arglen);
}

char *
nfn_request_description_new(struct nfn_request_s* request)
{
    int len = 0;
    char *buf = (char*) ccnl_malloc(256);
    
    if (request == NULL) {
        len += sprintf(buf + len, "request(NULL)");
    } else {
        len += sprintf(buf + len, "request(");
        if (request->complen > 0) {
            len += sprintf(buf + len, "comp: %.*s, ", request->complen, request->comp);
        }
        if (request->type == NFN_REQUEST_TYPE_UNKNOWN) {
            len += sprintf(buf + len, "type: %s, ", "UNKNOWN");
        } else {
            len += sprintf(buf + len, "type: %s, ", nfn_request_names[request->type-1]);
        }
        len += sprintf(buf + len, "arg: %s", request->arg);
        len += sprintf(buf + len, ")");
    }

    buf[len] = '\0';
    return buf;
}

// Return the highest consecutive intermediate number for the prefix, starts with 0.
// -1 if no intermediate result is found.
int nfn_request_intermediate_num(struct ccnl_relay_s *relay, struct ccnl_prefix_s *prefix) {
    struct ccnl_content_s *c;
    int highest = -1;
    for (c = relay->contents; c; c = c->next) {
        if (ccnl_nfnprefix_isIntermediate(c->pkt->pfx)) {
            if (prefix->compcnt == ccnl_prefix_cmp(prefix, NULL, c->pkt->pfx, CMP_LONGEST)) {
                int internum = nfn_request_get_arg_int(c->pkt->pfx->request);
                if (highest < internum) {
                    highest = internum;
                }
            }
        }
    }
    return highest;
}

// TODO: ccnl not needed.
struct ccnl_pkt_s*
nfn_request_interest_pkt_new(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pfx)
{
    int nonce = random();
    struct ccnl_pkt_s *pkt;
    // struct ccnl_face_s *from;

    DEBUGMSG(TRACE, "nfn_request_interest_pkt_new() nonce=%i\n", nonce);
    
    if (!pfx)
        return NULL;
    
    pkt = (struct ccnl_pkt_s *) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;

    // from = NULL;
    
    pkt->suite = pfx->suite;
    switch(pkt->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        pkt->s.ccnb.maxsuffix = CCNL_MAX_NAME_COMP;
        break;
#endif
    case CCNL_SUITE_NDNTLV:
        pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;
        break;
    default:
        break;
    }
    pkt->pfx = ccnl_prefix_dup(pfx);
    pkt->buf = ccnl_mkSimpleInterest(pkt->pfx, &nonce);
    pkt->val.final_block_id = -1;

    // i = ccnl_interest_new(ccnl, from, &pkt);

    // DEBUGMSG(TRACE, "  new request interest %p, from=%p, i->from=%p\n",
    //             (void*)i, (void*)from, (void*)i->from);
    return pkt;
}

struct ccnl_interest_s *
nfn_request_interest_new(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pfx)
{
    struct ccnl_interest_s *i;
    struct ccnl_pkt_s *pkt;    
    struct ccnl_face_s *from;
    
    DEBUGMSG(TRACE, "nfn_request_interest_new()\n");

    pkt = nfn_request_interest_pkt_new(ccnl, pfx);
    if (pkt == NULL)
        return NULL;

    from = NULL;
    i = ccnl_interest_new(ccnl, from, &pkt);

    DEBUGMSG(TRACE, "  new request interest %p, from=%p, i->from=%p\n",
                (void*)i, (void*)from, (void*)i->from);

    return  i;
}

struct ccnl_pkt_s*
nfn_request_content_pkt_new(struct ccnl_prefix_s *pfx, unsigned char* payload, int paylen)
{
    int nonce = random();
    struct ccnl_pkt_s *pkt;
    int dataoffset;

    DEBUGMSG(TRACE, "nfn_request_content_pkt_new() nonce=%i\n", nonce);
    
    if (!pfx)
        return NULL;
    
    pkt = (struct ccnl_pkt_s *) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;

    pkt->suite = pfx->suite;
    switch(pkt->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        pkt->s.ccnb.maxsuffix = CCNL_MAX_NAME_COMP;
        break;
#endif
    case CCNL_SUITE_NDNTLV:
        pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;
        break;
    default:
        break;
    }
    pkt->pfx = ccnl_prefix_dup(pfx);
    pkt->buf = ccnl_mkSimpleContent(pkt->pfx, payload, paylen, &dataoffset);
    pkt->content = pkt->buf->data + dataoffset;
    pkt->contlen = paylen;
    // pkt->val.final_block_id = -1;

    return pkt;
}

void
nfn_request_content_set_prefix(struct ccnl_content_s *c, struct ccnl_prefix_s *pfx) {
    c->pkt = nfn_request_content_pkt_new(pfx, c->pkt->content, c->pkt->contlen);
    // c->pkt->buf = ccnl_mkSimpleContent(
        // c->pkt->pfx, c->pkt->content, c->pkt->contlen, 0);
}

struct ccnl_interest_s*
nfn_request_find_pending_subcomputation(struct ccnl_relay_s *relay, struct configuration_s *config)
{
    DEBUGMSG(TRACE, "nfn_request_find_pending_subcomputation() configid=%i\n", config->configid);
    struct ccnl_interest_s *i;
    for (i = relay->pit; i; i = i->next) {
        if (i->from && i->from->faceid == config->configid) {
            DEBUGMSG_CFWD(DEBUG, "  found pending subcomputation\n");
            return i;
        }
    }
    DEBUGMSG_CFWD(DEBUG, "  did not find pending subcomputation\n");
    return NULL;
}

int
nfn_request_forward_to_computation(struct ccnl_relay_s *relay, struct ccnl_pkt_s **pkt)
{
    struct ccnl_prefix_s *pfx;
    struct ccnl_interest_s *i;
    struct configuration_s *config;
    
    DEBUGMSG(TRACE, "nfn_request_forward_to_computation()\n");

    config = ccnl_nfn_find_running_computation(relay, (*pkt)->pfx);
    if (!config) {
        return 0;
    }

    i = nfn_request_find_pending_subcomputation(relay, config);
    if (!i) {
        return 0;
    }

    pfx = ccnl_prefix_dup(i->pkt->pfx);
    ccnl_nfnprefix_set(pfx, CCNL_PREFIX_REQUEST);
    pfx->request = nfn_request_copy((*pkt)->pfx->request);

    char *s = NULL;
    DEBUGMSG_CFWD(INFO, "  new request=<%s>\n", (s = ccnl_prefix_to_path(pfx)));
    ccnl_free(s);

    i = nfn_request_interest_new(relay, pfx);
    ccnl_interest_propagate(relay, i);
    ccnl_interest_remove(relay, i);

    return 1;
}

int 
nfn_request_cancel_local_computation(struct ccnl_relay_s *relay, struct ccnl_pkt_s **pkt) 
{
    struct ccnl_prefix_s *pfx;
    struct ccnl_prefix_s *copy;
    struct ccnl_interest_s *i;
    struct configuration_s *config;

    char *s = NULL;
    
    DEBUGMSG_CFWD(DEBUG, "  nfn_request_cancel_local_computation()\n");

    config = ccnl_nfn_find_running_computation(relay, (*pkt)->pfx);
    if (!config) {
        return 0;
    }
    i = nfn_request_find_pending_subcomputation(relay, config);
    if (!i) {
        return 0;
    }

    
    pfx = ccnl_prefix_dup(i->pkt->pfx);
    DEBUGMSG_CFWD(INFO, "  removing interests related to computation =<%s>\n", (s = ccnl_prefix_to_path(pfx)));

    for (i = relay->pit; i; i = i->next) {
        DEBUGMSG_CFWD(INFO, "  testing interest=<%s>\n", (s = ccnl_prefix_to_path(i->pkt->pfx)));
        copy = ccnl_prefix_dup(i->pkt->pfx);
        ccnl_nfnprefix_clear(copy, CCNL_PREFIX_REQUEST);
        DEBUGMSG_CFWD(INFO, "  simplified interest=<%s>\n", (s = ccnl_prefix_to_path(copy)));
        if (!ccnl_prefix_cmp(copy, NULL, pfx, CMP_EXACT)) {
            
            DEBUGMSG_CFWD(INFO, "  removing interest=<%s>\n", (s = ccnl_prefix_to_path(i->pkt->pfx)));
            i = ccnl_interest_remove(relay, i);
        }
    }

    ccnl_free(s);
    // if (ccnl_interest_remove_pending(i, i->from)) {
    //     DEBUGMSG_CFWD(DEBUG, "  removed face from pending local computation\n");
    // } else {
    //     DEBUGMSG_CFWD(DEBUG, "  no face matching found to remove for pending local computation\n");
    // }
    return 1;
}

/* Returns 0 if the interest has been handled completely and no further
   processing is needed. 1 otherwise. */
int
nfn_request_handle_interest(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                            struct ccnl_pkt_s **pkt, cMatchFct cMatch)
{
    struct ccnl_interest_s *i;
    struct ccnl_interest_s *r;
    switch ((*pkt)->pfx->request->type) {
        case NFN_REQUEST_TYPE_START: {
            DEBUGMSG_CFWD(DEBUG, "  is a start computation interest\n");
                // Handled in ccnl_nfn_RX_request()
            return 1;
        }
        case NFN_REQUEST_TYPE_CANCEL: {
            DEBUGMSG_CFWD(DEBUG, "  is a cancel computation interest\n");
            
            nfn_request_forward_to_computation(relay, pkt);

            nfn_request_cancel_local_computation(relay, pkt);

            // find the matching pending interest that should be cancelled
            struct ccnl_prefix_s *copy = ccnl_prefix_dup((*pkt)->pfx);
            ccnl_nfnprefix_clear(copy, CCNL_PREFIX_REQUEST);
            for (i = relay->pit; i; i = i->next) {
                if (!ccnl_prefix_cmp(i->pkt->pfx, NULL, copy, CMP_EXACT)) {
                    DEBUGMSG_CFWD(DEBUG, "  found matching PIT entry\n");

                    // remove the incoming face from the pending interest
                    if (ccnl_interest_remove_pending(i, from)) {
                        DEBUGMSG_CFWD(DEBUG, "  removed face from pending interest\n");
                    } else {
                        DEBUGMSG_CFWD(DEBUG, "  no face matching found to remove\n");
                    }

                    // forward the cancel request
                    r = ccnl_interest_new(relay, from, pkt);
                    if (r) {
                        ccnl_interest_propagate(relay, r);
                        ccnl_interest_remove(relay, r);
                    }

                    // remove the pending interest if all faces have been removed
                    if (!i->pending) {
                        i = ccnl_interest_remove(relay, i);
                    }
                    if (!i) {
                        break;
                    }
                }
            }
            ccnl_free(copy);            
            return 0;
        }
        case NFN_REQUEST_TYPE_KEEPALIVE: {
            DEBUGMSG_CFWD(DEBUG, "  is a keepalive interest\n");
            if (ccnl_nfn_already_computing(relay, (*pkt)->pfx)) {
                DEBUGMSG_CFWD(DEBUG, "  running computation found");
                struct ccnl_buf_s *buf = ccnl_mkSimpleContent((*pkt)->pfx, NULL, 0, NULL);
                ccnl_face_enqueue(relay, from, buf);
                return 0;
            }
            DEBUGMSG_CFWD(DEBUG, "  no running computation found.\n");
            return 1;
        }
        case NFN_REQUEST_TYPE_COUNT_INTERMEDIATES: {
            DEBUGMSG_CFWD(DEBUG, "  is a count intermediates interest\n");
            if (ccnl_nfn_already_computing(relay, (*pkt)->pfx)) {
                int internum = nfn_request_intermediate_num(relay, (*pkt)->pfx);
                DEBUGMSG_CFWD(DEBUG, "  highest intermediate result: %i\n", internum);
                int offset;
                char reply[16];
                snprintf(reply, 16, "%d", internum);
                int size = internum >= 0 ? strlen(reply) : 0;
                struct ccnl_buf_s *buf  = ccnl_mkSimpleContent((*pkt)->pfx, (unsigned char *)reply, size, &offset);
                ccnl_face_enqueue(relay, from, buf);
                return 0;
            }
            DEBUGMSG_CFWD(DEBUG, "  no running computation found.\n");
            return 1;
        }
        case NFN_REQUEST_TYPE_GET_INTERMEDIATE: {
            DEBUGMSG_CFWD(DEBUG, "  is a get intermediates interest\n");
            return 0;
        }
        default: {
            DEBUGMSG_CFWD(DEBUG, "  Unknown request type.\n");
            return 0;
        }
    }
    return 0;
}



#endif

// eof
