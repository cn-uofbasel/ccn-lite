/*
 * @f ccnl-ext-echo.c
 * @b CCN lite extension: echo/ping service - send back run-time generated data
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-01-12 created
 */

#ifdef USE_ECHO

void
ccnl_echo_request(struct ccnl_relay_s *relay, struct ccnl_face_s *inface,
                  struct ccnl_prefix_s *pfx, struct ccnl_buf_s *buf)
{
    time_t t;
    char *s, *cp;
    struct ccnl_buf_s *reply;
    unsigned char *ucp;
    int len, enc;
    struct ccnl_prefix_s *pfx2 = NULL;

    DEBUGMSG(DEBUG, "echo request for <%s>\n", ccnl_prefix_to_path(pfx));

//    if (pfx->chunknum) {
        // mkSimpleContent adds the chunk number, so remove it here
      /*
        ccnl_free(pfx->chunknum);
        pfx->chunknum = NULL;
      */
#ifdef USE_SUITE_CCNTLV
    if (pfx->complen[pfx->compcnt-1] > 1 &&
        pfx->comp[pfx->compcnt-1][1] == CCNX_TLV_N_Chunk) {
        struct ccnl_prefix_s *pfx2 = ccnl_prefix_dup(pfx);
        pfx2->compcnt--;
        pfx2->chunknum = (int*) ccnl_malloc(sizeof(unsigned int));
        *(pfx2->chunknum) = 0;
        pfx = pfx2;
    }
#endif

    t = time(NULL);
    s = ccnl_prefix_to_path(pfx);

    cp = ccnl_malloc(strlen(s) + 60);
    sprintf(cp, "%s\n%suptime %s\n", s, ctime(&t), timestamp());

    reply = ccnl_mkSimpleContent(pfx, (unsigned char*) cp, strlen(cp), 0);
    ccnl_free(cp);
    if (pfx2) {
        free_prefix(pfx2);
    }

    ucp = reply->data;
    len = reply->datalen;

    while (!ccnl_switch_dehead(&ucp, &len, &enc)); // for iot2014 encoding

    ccnl_core_suites[(int)pfx->suite].RX(relay, NULL, &ucp, &len);
    ccnl_free(reply);
}

// insert forwarding entry with a tap - the prefix arg is consumed
int
ccnl_echo_add(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx)
{
    return ccnl_set_tap(relay, pfx, ccnl_echo_request);
}

void
ccnl_echo_cleanup(struct ccnl_relay_s *relay)
{
    struct ccnl_forward_s *fwd;

    DEBUGMSG(DEBUG, "removing all echo servers\n");

    for (fwd = relay->fib; fwd; fwd = fwd->next) {
        if (fwd->tap == ccnl_echo_request) {
            fwd->tap = NULL;
/*
            if (fwd->face == NULL) { // remove this entry
                free_prefix(fwd->prefix);
                fwd->prefix = 0;
            }
*/
        }
    }
}

#endif // USE_ECHO

// eof
