/*
 * @f fwd-ccntlv.c
 * @b CCN lite, CCNx1.0 forwarding (using the new TLV format of Nov 2013)
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
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
 * 2014-03-20 created
 */

#ifdef USE_SUITE_CCNTLV

#include "pkt-ccntlv-dec.c"

// work on a message (buffer passed without the fixed header)
int
ccnl_ccntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      struct ccnx_tlvhdr_ccnx201409_s *hdrptr, int hoplimit,
                      unsigned char **data, int *datalen)
{
    int rc = -1;
    int keyidlen, contlen;
    struct ccnl_buf_s *buf = 0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0, *keyid = 0;


    // if (ccnl_ccntlv_dehead(data, datalen, &typ, &len))
    //     return -1;

    unsigned char typ = hdrptr->packettype;
    unsigned char hdrlen = *data - (unsigned char*)hdrptr;
    DEBUGMSG(99, "ccnl_ccntlv_forwarder (%d bytes left)\n", *datalen);
    DEBUGMSG(99, "hdrlen=%d", hdrlen);
    buf = ccnl_ccntlv_extract(hdrlen, data, datalen,
                              &p, &keyid, &keyidlen,
                              &content, &contlen);
    if (!buf) {
            DEBUGMSG(6, "  parsing error or no prefix\n"); goto Done;
    }

    if (typ == CCNX_PT_Interest) {
        DEBUGMSG(6, "  interest=<%s>\n", ccnl_prefix_to_path(p));
        ccnl_print_stats(relay, STAT_RCV_I); //log count recv_interest
        // CONFORM: Step 1: search for matching local content
        for (c = relay->contents; c; c = c->next) {
            if (c->suite != CCNL_SUITE_CCNTLV)
                continue;
            // TODO: check keyid
            // TODO: check freshness, kind-of-reply
            if (ccnl_prefix_cmp(c->name, NULL, p, CMP_EXACT))
                continue;
            DEBUGMSG(7, "  matching content for interest, content %p\n", (void *) c);
            ccnl_print_stats(relay, STAT_SND_C); //log sent_c
            if (from->ifndx >= 0){
                ccnl_nfn_monitor(relay, from, c->name, c->content, c->contentlen);
                ccnl_face_enqueue(relay, from, buf_dup(c->pkt));
            } else {
                ccnl_app_RX(relay, c);
            }
            goto Skip;
        }
        DEBUGMSG(7, "  no matching content for interest\n");
        // CONFORM: Step 2: check whether interest is already known
        for (i = relay->pit; i; i = i->next) {
            if (i->suite != CCNL_SUITE_CCNTLV)
                continue;
            if (!ccnl_prefix_cmp(i->prefix, NULL, p, CMP_EXACT))
                // TODO check hoplimit
                // TODO check keyid
                break;
        }
        // this is a new/unknown I request: create and propagate
#ifdef USE_NFN
        if (!i && ccnl_nfnprefix_isNFN(p)) { // NFN PLUGIN CALL
            if (ccnl_nfn_RX_request(relay, from, CCNL_SUITE_CCNTLV, &buf,
                                    &p, 0, 0))
                goto Done;
        }
#endif
        if (!i) {
            // TODO keyID restriction
            // TODO hoplimit
            i = ccnl_interest_new(relay, from, CCNL_SUITE_CCNTLV,
                                  &buf, &p, 0, 0 /* hoplimit */);
            if (i) { // CONFORM: Step 3 (and 4)
                DEBUGMSG(7, "  created new interest entry %p\n", (void *) i);
                ccnl_interest_propagate(relay, i);
            }
        } else if ((from->flags & CCNL_FACE_FLAGS_FWDALLI)) {
            DEBUGMSG(7, "  old interest, nevertheless propagated %p\n", (void *) i);
            ccnl_interest_propagate(relay, i);
        }
        if (i) { // store the I request, for the incoming face (Step 3)
            DEBUGMSG(7, "  appending interest entry %p\n", (void *) i);
            ccnl_interest_append_pending(i, from);
        }

    } else if (typ == CCNX_PT_ContentObject) { // data packet with content
        DEBUGMSG(6, "  data=<%s>\n", ccnl_prefix_to_path(p));
        ccnl_print_stats(relay, STAT_RCV_C); //log count recv_content

        // CONFORM: Step 1:
        for (c = relay->contents; c; c = c->next)
            if (buf_equal(c->pkt, buf)) goto Skip; // content is dup
        c = ccnl_content_new(relay, CCNL_SUITE_CCNTLV,
                             &buf, &p, NULL, content, contlen);
        if (c) { // CONFORM: Step 2 (and 3)
#ifdef USE_NFN
            if (ccnl_nfnprefix_isNFN(c->name)) {
                if (ccnl_nfn_RX_result(relay, from, c))
                    goto Done;
                DEBUGMSG(99, "no running computation found \n");
            }
#endif
            if (!ccnl_content_serve_pending(relay, c)) { // unsolicited content
                // CONFORM: "A node MUST NOT forward unsolicited data [...]"
                DEBUGMSG(7, "  removed because no matching interest\n");
                free_content(c);
                goto Skip;
            }
            if (relay->max_cache_entries != 0) { // it's set to -1 or a limit
                DEBUGMSG(7, "  adding content to cache\n");
                ccnl_content_add2cache(relay, c);
            } else {
                DEBUGMSG(7, "  content not added to cache\n");
                free_content(c);
            }
        }
    }

Skip:
    rc = 0;
Done:
    free_prefix(p);
//    free_3ptr_list(buf, nonce, ppkl);
    ccnl_free(buf);
    return rc;
}

int
ccnl_RX_ccntlv(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
               unsigned char **data, int *datalen)
{
    int rc = 0, endlen;
    unsigned char *end;
    DEBUGMSG(6, "ccnl_RX_ccntlv: %d bytes from face=%p (id=%d.%d)\n",
             *datalen, (void*)from, relay->id, from ? from->faceid : -1);

next:
    while (rc >= 0 && **data == CCNX_TLV_V0 &&
                        *datalen >= sizeof(struct ccnx_tlvhdr_ccnx201409_s)) {
        struct ccnx_tlvhdr_ccnx201409_s *hp = (struct ccnx_tlvhdr_ccnx201409_s*) *data;
        int hoplimit = -1;

        unsigned short hdrlen = ntohs(hp->hdrlen);
        unsigned short payloadlen = ntohs(hp->payloadlen);

        // check if info data to construct packet header
        if (hdrlen > *datalen)
            return -1;

        *data += hdrlen;
        *datalen -= hdrlen;

        // check if enough data to reconstruct message
        if (payloadlen > *datalen)
            return -1;

        end = *data + payloadlen;
        endlen = *datalen - payloadlen;

        hoplimit = hp->hoplimit - 1;
        if (hp->packettype == CCNX_PT_Interest && hoplimit <= 0) { // drop this interest
            *data = end;
            *datalen = endlen;
            goto next;
        }
        rc = ccnl_ccntlv_forwarder(relay, from, hp, hoplimit, data, datalen);
    }
    return rc;
}

#endif // USE_SUITE_CCNTLV

// eof
