/*
 * @f ccnl-ext-echo.c
 * @b CCN lite extension: echo service
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

    DEBUGMSG(DEBUG, "echo request for <%s>\n", ccnl_prefix_to_path(pfx));

    t = time(NULL);
    s = ccnl_prefix_to_path(pfx);

    cp = ccnl_malloc(strlen(s) + 60);
    strcpy(cp, ctime(&t));
    strcat(cp, s);

    reply = ccnl_mkSimpleContent(pfx, (unsigned char*) cp, strlen(cp), 0);
    ccnl_free(cp);
//    ccnl_face_enqueue(relay, inface, reply);

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
    struct ccnl_forward_s *fwd, **fwd2;

    DEBUGMSG(DEBUG, "adding echo server for <%s>, suite %s\n",
             ccnl_prefix_to_path(pfx), ccnl_suite2str(pfx->suite));

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
    fwd->tap = ccnl_echo_request;

    return 0;
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
