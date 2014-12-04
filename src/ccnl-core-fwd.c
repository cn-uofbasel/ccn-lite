/*
 * @f ccnl-core-fwd.c
 * @b CCN lite, the collection of suite specific forwarding logics
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
 * 2014-11-05 collected from the various fwd-XXX.c files
 */

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNB

int
ccnl_ccnb_forwarder(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
                    unsigned char **data, int *datalen)
{
    int rc= -1, scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkd=0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0;
    DEBUGMSG(99, "ccnl/ccnb forwarder (%d bytes left)\n", *datalen);

    buf = ccnl_ccnb_extract(data, datalen, &scope, &aok, &minsfx,
                            &maxsfx, &p, &nonce, &ppkd, &content, &contlen);
    if (!buf) {
        DEBUGMSG(6, "  parsing error or no prefix\n");
        goto Done;
    }
    if (nonce && ccnl_nonce_find_or_append(ccnl, nonce)) {
        DEBUGMSG(6, "  dropped because of duplicate nonce\n");
        goto Skip;
    }
    if (buf->data[0] == 0x01 && buf->data[1] == 0xd2) { // interest
        DEBUGMSG(6, "  interest=<%s>\n", ccnl_prefix_to_path(p));
        ccnl_print_stats(ccnl, STAT_RCV_I); //log count recv_interest
        if (p->compcnt > 0 && p->comp[0][0] == (unsigned char) 0xc1)
            goto Skip;
        if (p->compcnt == 4 && !memcmp(p->comp[0], "ccnx", 4)) {
            rc = ccnl_mgmt(ccnl, buf, p, from);
            goto Done;
        }
        // CONFORM: Step 1:
        if ( aok & 0x01 ) { // honor "answer-from-existing-content-store" flag
            for (c = ccnl->contents; c; c = c->next) {
                if (c->suite != CCNL_SUITE_CCNB) continue;
                if (!ccnl_i_prefixof_c(p, minsfx, maxsfx, c)) continue;
                if (ppkd && !buf_equal(ppkd, c->details.ccnb.ppkd)) continue;
                // FIXME: should check stale bit in aok here
                DEBUGMSG(7, "  matching content for interest, content %p\n", (void *) c);
                ccnl_print_stats(ccnl, STAT_SND_C); //log sent_c
                if (from->ifndx >= 0) {
                    ccnl_nfn_monitor(ccnl, from, c->name, c->content, c->contentlen);
                    ccnl_face_enqueue(ccnl, from, buf_dup(c->pkt));
                } else {
                    ccnl_app_RX(ccnl, c);
                }
                goto Skip;
            }
        }
        // CONFORM: Step 2: check whether interest is already known
        for (i = ccnl->pit; i; i = i->next) {
            if (i->suite == CCNL_SUITE_CCNB &&
                !ccnl_prefix_cmp(i->prefix, NULL, p, CMP_EXACT) &&
                i->details.ccnb.minsuffix == minsfx &&
                i->details.ccnb.maxsuffix == maxsfx && 
                ((!ppkd && !i->details.ccnb.ppkd) ||
                   buf_equal(ppkd, i->details.ccnb.ppkd)) )
                break;
        }
        // this is a new/unknown I request: create and propagate
#ifdef USE_NFN
        if (!i && ccnl_nfnprefix_isNFN(p)) { // NFN PLUGIN CALL
            if (ccnl_nfn_RX_request(ccnl, from, CCNL_SUITE_CCNB,
                                    &buf, &p, minsfx, maxsfx))
              //Since the interest msg may be required in future it is not possible
              //to delete the interest/prefix here
              return rc;
        }
#endif
        if (!i) {
            i = ccnl_interest_new(ccnl, from, CCNL_SUITE_CCNB,
                                  &buf, &p, minsfx, maxsfx);
            if (ppkd)
                i->details.ccnb.ppkd = ppkd, ppkd = NULL;
            if (i) { // CONFORM: Step 3 (and 4)
                DEBUGMSG(7, "  created new interest entry %p\n", (void *) i);
                if (scope > 2)
                    ccnl_interest_propagate(ccnl, i);
            }
        } else if (scope > 2 && (from->flags & CCNL_FACE_FLAGS_FWDALLI)) {
            DEBUGMSG(7, "  old interest, nevertheless propagated %p\n", (void *) i);
            ccnl_interest_propagate(ccnl, i);
        }
        if (i) { // store the I request, for the incoming face (Step 3)
            DEBUGMSG(7, "  appending interest entry %p\n", (void *) i);
            ccnl_interest_append_pending(i, from);
        }
    } else { // content
        DEBUGMSG(6, "  content=<%s>\n", ccnl_prefix_to_path(p));
        ccnl_print_stats(ccnl, STAT_RCV_C); //log count recv_content
        
#ifdef USE_SIGNATURES
        if (p->compcnt == 2 && !memcmp(p->comp[0], "ccnx", 4)
                && !memcmp(p->comp[1], "crypto", 6) &&
                from == ccnl->crypto_face) {
            rc = ccnl_crypto(ccnl, buf, p, from);
            goto Done;
        }
#endif /*USE_SIGNATURES*/
        
        // CONFORM: Step 1:
        for (c = ccnl->contents; c; c = c->next)
            if (buf_equal(c->pkt, buf)) goto Skip; // content is dup
        c = ccnl_content_new(ccnl, CCNL_SUITE_CCNB,
                             &buf, &p, &ppkd, content, contlen);
        if (c) { // CONFORM: Step 2 (and 3)
#ifdef USE_NFN
            if (ccnl_nfnprefix_isNFN(c->name)) {
                if (ccnl_nfn_RX_result(ccnl, from, c))
                    goto Done;
                DEBUGMSG(99, "no running computation found \n");
            }
#endif
            if (!ccnl_content_serve_pending(ccnl, c)) { // unsolicited content
                // CONFORM: "A node MUST NOT forward unsolicited data [...]"
                DEBUGMSG(7, "  removed because no matching interest\n");
                free_content(c);
                goto Skip;
            }
        if (ccnl->max_cache_entries != 0) { // it's set to -1 or a limit
                DEBUGMSG(7, "  adding content to cache\n");
                ccnl_content_add2cache(ccnl, c);
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
    free_3ptr_list(buf, nonce, ppkd);
    return rc;
}

int
ccnl_RX_ccnb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
              unsigned char **data, int *datalen)
{
    int rc = 0, num, typ;
    DEBUGMSG(6, "ccnl_RX_ccnb: %d bytes from face=%p (id=%d.%d)\n",
             *datalen, (void*)from, relay->id, from ? from->faceid : -1);

    while (rc >= 0 && *datalen > 0) {
        if (ccnl_ccnb_dehead(data, datalen, &num, &typ) || typ != CCN_TT_DTAG)
            return -1;
        switch (num) {
        case CCN_DTAG_INTEREST:
        case CCN_DTAG_CONTENTOBJ:
            rc = ccnl_ccnb_forwarder(relay, from, data, datalen);
            continue;
#ifdef USE_FRAG
        case CCNL_DTAG_FRAGMENT2012:
            rc = ccnl_frag_RX_frag2012(ccnl_RX_ccnb, relay, from, data, datalen);
            continue;
        case CCNL_DTAG_FRAGMENT2013:
            rc = ccnl_frag_RX_CCNx2013(ccnl_RX_ccnb, relay, from, data, datalen);
            continue;
#endif
        default:
            DEBUGMSG(15, "  unknown datagram type %d\n", num);
            return -1;
        }
    }
    return rc;
}

#endif // USE_SUITE_CCNB


// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNTLV

// work on a message (buffer passed without the fixed header)
int
ccnl_ccntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      struct ccnx_tlvhdr_ccnx201411_s *hdrptr, int hoplimit,
                      unsigned char **data, int *datalen)
{
    int rc = -1;
    int keyidlen, contlen;
    unsigned int lastchunknum;
    struct ccnl_buf_s *buf = 0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0, *keyid = 0;


    // if (ccnl_ccntlv_dehead(data, datalen, &typ, &len))
    //     return -1;

    unsigned char typ = hdrptr->packettype;
    unsigned char hdrlen = *data - (unsigned char*)hdrptr;
    DEBUGMSG(99, "ccnl_ccntlv_forwarder (%d bytes left, hdrlen=%d)\n",
             *datalen, hdrlen);
    buf = ccnl_ccntlv_extract(hdrlen, 
                              data, datalen,
                              &p, 
                              &keyid, &keyidlen,
                              &lastchunknum,
                              &content, &contlen);
    if (!buf) {
            DEBUGMSG(6, "  parsing error or no prefix\n");
            goto Done;
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
                        *datalen >= sizeof(struct ccnx_tlvhdr_ccnx201411_s)) {
        struct ccnx_tlvhdr_ccnx201411_s *hp = (struct ccnx_tlvhdr_ccnx201411_s*) *data;
        int hoplimit = -1;
        unsigned short hdrlen = hp->hdrlen; // ntohs(hp->hdrlen);
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

// ----------------------------------------------------------------------

#ifdef USE_SUITE_NDNTLV

int
ccnl_ndntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen)
{
    int len, rc=-1, typ;
    int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0, *tracing = 0;
    unsigned char *content = 0, *cp = *data;
    DEBUGMSG(99, "ccnl_ndntlv_forwarder (%d bytes left)\n", *datalen);

    if (ccnl_ndntlv_dehead(data, datalen, &typ, &len))
        return -1;
    buf = ccnl_ndntlv_extract(*data - cp,
                              data, datalen,
                              &scope, &mbf, &minsfx, &maxsfx, 0, 
                              &p, &tracing, &nonce, &ppkl, &content, &contlen);
    if (!buf) {
        DEBUGMSG(6, "  parsing error or no prefix\n");
        goto Done;
    }

    if (typ == NDN_TLV_Interest) {
        if (nonce && ccnl_nonce_find_or_append(relay, nonce)) {
            DEBUGMSG(6, "  dropped because of duplicate nonce\n");
            goto Skip;
        }
        DEBUGMSG(6, "  interest=<%s>\n", ccnl_prefix_to_path(p));
        ccnl_print_stats(relay, STAT_RCV_I); //log count recv_interest
    /*
        filter here for link level management messages ...
        if (p->compcnt == 4 && !memcmp(p->comp[0], "ccnx", 4)) {
            rc = ccnl_mgmt(relay, buf, p, from); goto Done;
        }
    */
        // CONFORM: Step 1: search for matching local content
        for (c = relay->contents; c; c = c->next) {
            if (c->suite != CCNL_SUITE_NDNTLV) continue;
            if (!ccnl_i_prefixof_c(p, minsfx, maxsfx, c)) continue;
            if (ppkl && !buf_equal(ppkl, c->details.ndntlv.ppkl)) continue;
            // FIXME: should check freshness (mbf) here
            // if (mbf) // honor "answer-from-existing-content-store" flag
            DEBUGMSG(7, "  matching content for interest, content %p\n", (void *) c);
            ccnl_print_stats(relay, STAT_SND_C); //log sent_c
            if (from->ifndx >= 0) {
                ccnl_nfn_monitor(relay, from, c->name, c->content, c->contentlen);
                ccnl_face_enqueue(relay, from, buf_dup(c->pkt));
            } else {
                ccnl_app_RX(relay, c);
            }
            goto Skip;
        }
        // CONFORM: Step 2: check whether interest is already known
#ifdef USE_KITE
        if (tracing) { // is a tracing interest
            for (i = relay->pit; i; i = i->next) {
            }
        }
#endif
        for (i = relay->pit; i; i = i->next) {
            if (i->suite == CCNL_SUITE_NDNTLV &&
                !ccnl_prefix_cmp(i->prefix, NULL, p, CMP_EXACT) &&
                i->details.ndntlv.minsuffix == minsfx &&
                i->details.ndntlv.maxsuffix == maxsfx &&
                ((!ppkl && !i->details.ndntlv.ppkl) ||
                 buf_equal(ppkl, i->details.ndntlv.ppkl)) )
                break;
        }
        // this is a new/unknown I request: create and propagate
#ifdef USE_NFN
        if (!i && ccnl_nfnprefix_isNFN(p)) { // NFN PLUGIN CALL
            if (ccnl_nfn_RX_request(relay, from, CCNL_SUITE_NDNTLV,
                                    &buf, &p, minsfx, maxsfx))
                // Since the interest msg may be required in future it is
                // not possible to delete the interest/prefix here
                // return rc;
                goto Done;
        }
#endif
        if (!i) {
            i = ccnl_interest_new(relay, from, CCNL_SUITE_NDNTLV,
                      &buf, &p, minsfx, maxsfx);
            if (ppkl)
                i->details.ndntlv.ppkl = ppkl, ppkl = NULL;
            if (i) { // CONFORM: Step 3 (and 4)
                DEBUGMSG(7, "  created new interest entry %p\n", (void *) i);
                if (scope > 2)
                    ccnl_interest_propagate(relay, i);
            }
        } else if (scope > 2 && (from->flags & CCNL_FACE_FLAGS_FWDALLI)) {
            DEBUGMSG(7, "  old interest, nevertheless propagated %p\n", (void *) i);
            ccnl_interest_propagate(relay, i);
        }
        if (i) { // store the I request, for the incoming face (Step 3)
            DEBUGMSG(7, "  appending interest entry %p\n", (void *) i);
            ccnl_interest_append_pending(i, from);
        }
    } else { // data packet with content -------------------------------------
        DEBUGMSG(6, "  data=<%s>\n", ccnl_prefix_to_path(p));
        ccnl_print_stats(relay, STAT_RCV_C); //log count recv_content

/*  mgmt messages for NDN?
#ifdef USE_SIGNATURES
        if (p->compcnt == 2 && !memcmp(p->comp[0], "ccnx", 4)
                && !memcmp(p->comp[1], "crypto", 6) &&
                from == relay->crypto_face) {
            rc = ccnl_crypto(relay, buf, p, from); goto Done;
        }
#endif // USE_SIGNATURES
*/
        
        // CONFORM: Step 1:
        for (c = relay->contents; c; c = c->next)
            if (buf_equal(c->pkt, buf)) goto Skip; // content is dup
        c = ccnl_content_new(relay, CCNL_SUITE_NDNTLV,
                             &buf, &p, NULL /* ppkd */ , content, contlen);
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
    free_3ptr_list(buf, nonce, ppkl);
    return rc;
}

int
ccnl_RX_ndntlv(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
               unsigned char **data, int *datalen)
{
    int rc = 0;
    DEBUGMSG(6, "ccnl_RX_ndntlv: %d bytes from face=%p (id=%d.%d)\n",
             *datalen, (void*)from, relay->id, from ? from->faceid : -1);

    while (rc >= 0 && *datalen > 0) {
        if (**data == NDN_TLV_Interest || **data == NDN_TLV_Data) {
            rc = ccnl_ndntlv_forwarder(relay, from, data, datalen);
            continue;
        }
/*
#ifdef USE_FRAG
        case CCNL_DTAG_FRAGMENT2012:
            rc = ccnl_frag_RX_frag2012(ccnl_RX_ccnb, relay, from, data, datalen);
            continue;
        case CCNL_DTAG_FRAGMENT2013:
            rc = ccnl_frag_RX_CCNx2013(ccnl_RX_ccnb, relay, from, data, datalen);
            continue;
#endif
*/
        DEBUGMSG(15, "  unknown datagram type 0x%02x\n", **data);
        return -1;
    }
    return rc;
}

#endif // USE_SUITE_NDNTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_LOCALRPC

int ccnl_rdr_dump(int lev, struct rdr_ds_s *x)
{
    int i, t;
    char *n, tmp[20];

    t = ccnl_rdr_getType(x);
    if (t < NDN_TLV_RPC_SERIALIZED)
        return t;
    if (t < NDN_TLV_RPC_APPLICATION) {
        sprintf(tmp, "v%02x", t);
        n = tmp;
    } else switch (t) {
    case NDN_TLV_RPC_APPLICATION:
        n = "APP"; break;
    case NDN_TLV_RPC_LAMBDA:
        n = "LBD"; break;
    case NDN_TLV_RPC_SEQUENCE:
        n = "SEQ"; break;
    case NDN_TLV_RPC_NAME:
        n = "VAR"; break;
    case NDN_TLV_RPC_NONNEGINT:
        n = "INT"; break;
    case NDN_TLV_RPC_BIN:
        n = "BIN"; break;
    case NDN_TLV_RPC_STR:
        n = "STR"; break;
    default:
        n = "???"; break;
    }
    for (i = 0; i < lev; i++)
        fprintf(stderr, "  ");
    if (t == NDN_TLV_RPC_NAME)
        fprintf(stderr, "%s (0x%x, len=%d)\n", n, t, x->u.namelen);
    else
        fprintf(stderr, "%s (0x%x)\n", n, t);

    switch (t) {
    case NDN_TLV_RPC_APPLICATION:
        ccnl_rdr_dump(lev+1, x->u.fct);
        break;
    case NDN_TLV_RPC_LAMBDA:
        ccnl_rdr_dump(lev+1, x->u.lambdavar);
        break;
    case NDN_TLV_RPC_SEQUENCE:
        break;
    case NDN_TLV_RPC_NAME:
    case NDN_TLV_RPC_NONNEGINT:
    case NDN_TLV_RPC_BIN:
    case NDN_TLV_RPC_STR:
    default:
        return 0;
    }
    x = x->aux;
    while (x) {
        ccnl_rdr_dump(lev+1, x);
        x = x->nextinseq;
    }
    return 0;
}

int
ccnl_emit_RpcReturn(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                    int rc, char *reason, struct rdr_ds_s *content)
{

    struct ccnl_buf_s *pkt;
    struct rdr_ds_s *seq, *element;
    int len;

    len = strlen(reason) + 50;
    if (content)
        len += ccnl_rdr_getFlatLen(content);
    pkt = ccnl_buf_new(NULL, len);
    if (!pkt)
        return 0;

    // we build a sequence, and later change the type in the flattened bytes
    seq = ccnl_rdr_mkSeq();
    element = ccnl_rdr_mkNonNegInt(rc);
    ccnl_rdr_seqAppend(seq, element);
    element = ccnl_rdr_mkStr(reason);
    ccnl_rdr_seqAppend(seq, element);
    if (content) {
        ccnl_rdr_seqAppend(seq, content);
    }
    len = ccnl_rdr_serialize(seq, pkt->data, pkt->datalen);
    ccnl_rdr_free(seq);
    if (len < 0) {
        ccnl_free(pkt);
        return 0;
    }
//    fprintf(stderr, "%d bytes to return face=%p\n", len, from);

    *(pkt->data) = NDN_TLV_RPC_APPLICATION;
    pkt->datalen = len;
    ccnl_face_enqueue(relay, from, pkt);

    return 0;
}

// ----------------------------------------------------------------------


struct rpc_exec_s* rpc_exec_new(void)
{
    return ccnl_calloc(1, sizeof(struct rpc_exec_s));
}

void rpc_exec_free(struct rpc_exec_s *exec)
{
    if (!exec) return;
    if (exec->ostack)
        ccnl_rdr_free(exec->ostack);
    ccnl_free(exec);
}

// ----------------------------------------------------------------------

int rpc_syslog(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
               struct rpc_exec_s *exec, struct rdr_ds_s *param)
{
    DEBUGMSG(11, "rpc_syslog\n");
    if (ccnl_rdr_getType(param) == NDN_TLV_RPC_STR) {
        char *cp = ccnl_malloc(param->u.strlen + 1);
        memcpy(cp, param->aux, param->u.strlen);
        cp[param->u.strlen] = '\0';
        DEBUGMSG(1, "rpc_syslog: \"%s\"\n", cp);
        ccnl_free(cp);
        ccnl_emit_RpcReturn(relay, from, 200, "ok", NULL);
    } else {
        DEBUGMSG(12, "rpc_syslog: unknown param type\n");
        ccnl_emit_RpcReturn(relay, from,
                            415, "rpc_syslog: unknown param type", NULL);
    }
    return 0;
}

int rpc_forward(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                struct rpc_exec_s *exec, struct rdr_ds_s *param)
{
    int encoding, len;
    char *cp;
    unsigned char *ucp;

    DEBUGMSG(11, "rpc_forward\n");

    if (ccnl_rdr_getType(param) != NDN_TLV_RPC_NAME) {
        ccnl_emit_RpcReturn(relay, from,
                            415, "rpc_forward: expected encoding name", NULL);
        return 0;
    }
        
    cp = ccnl_malloc(param->u.namelen + 1);
    memcpy(cp, param->aux, param->u.namelen);
    cp[param->u.namelen] = '\0';
    if (!strcmp(cp,      "/rpc/const/encoding/ndn2013"))
        encoding = CCNL_SUITE_NDNTLV;
#ifdef CCNL_SUITE_CCNB
    else if (!strcmp(cp, "/rpc/const/encoding/ccnb"))
        encoding = CCNL_SUITE_CCNB;
#endif
    else
        encoding = -1;
    ccnl_free(cp);

    if (encoding < 0) {
        ccnl_emit_RpcReturn(relay, from,
                            415, "rpc_forward: no such encoding", NULL);
        return 0;
    }
    while (param->nextinseq) {
        param = param->nextinseq;
        if (ccnl_rdr_getType(param) != NDN_TLV_RPC_BIN) {
            ccnl_emit_RpcReturn(relay, from,
                                415, "rpc_forward: invalid or no data", NULL);
            return 0;
        }

        ucp = (unsigned char*) param->aux;
        len = param->u.binlen;
        switch(encoding) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            ccnl_RX_ccnb(relay, from, &ucp, &len);
            break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            ccnl_RX_ndntlv(relay, from, &ucp, &len);
            break;
#endif
        default:
            break;
        }
    }
    return 0;
}

int rpc_lookup(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
               struct rpc_exec_s *exec, struct rdr_ds_s *param)
{
    DEBUGMSG(11, "rpc_lookup\n");

    if (ccnl_rdr_getType(param) == NDN_TLV_RPC_NAME) {
        char *cp = ccnl_malloc(param->u.namelen + 1);
        struct rdr_ds_s *val = 0;
        memcpy(cp, param->aux, param->u.namelen);
        cp[param->u.namelen] = '\0';
        if (!strcmp(cp, "/rpc/config/compileString")) {
            val = ccnl_rdr_mkStr((char*)compile_string());
        } else if (!strcmp(cp, "/rpc/config/localTime")) {
            time_t t = time(NULL);
            char *p = ctime(&t);
            p[strlen(p) - 1] = '\0';
            val = ccnl_rdr_mkStr(p);
        }
        ccnl_free(cp);
        if (val)
            ccnl_emit_RpcReturn(relay, from, 200, "ok", val);
        else
            ccnl_emit_RpcReturn(relay, from,
                            415, "rpc_lookup: no such variable", NULL);
    } else {
//      DEBUGMSG(1, "rpc_lookup: unknown param type\n");
        ccnl_emit_RpcReturn(relay, from,
                            415, "rpc_lookup: not a variable name", NULL);
    }
    return 0;
}


struct x_s {
    char *name;
    rpcBuiltinFct *fct;
} builtin[] = {
    {"/rpc/builtin/lookup",  rpc_lookup},
    {"/rpc/builtin/forward", rpc_forward},
    {"/rpc/builtin/syslog",  rpc_syslog},
    {NULL, NULL}
};

rpcBuiltinFct* rpc_getBuiltinFct(struct rdr_ds_s *var)
{
    struct x_s *x = builtin;

    if (var->type != NDN_TLV_RPC_NAME)
        return NULL;
    while (x->name) {
        if (strlen(x->name) == var->u.namelen &&
            !memcmp(x->name, var->aux, var->u.namelen))
            return x->fct;
        x++;
    }
    return NULL;
}

int
ccnl_localrpc_handleReturn(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                           struct rdr_ds_s *rc, struct rdr_ds_s *aux)
{
    DEBUGMSG(10, "ccnl_RX_handleReturn %d %d\n",
             ccnl_rdr_getType(rc), ccnl_rdr_getType(aux));
    
    return 0;
}

int
ccnl_localrpc_handleApplication(struct ccnl_relay_s *relay,
                                struct ccnl_face_s *from,
                                struct rdr_ds_s *fexpr, struct rdr_ds_s *args)
{
    int ftype, rc;
    rpcBuiltinFct *fct;
    struct rpc_exec_s *exec;

    DEBUGMSG(10, "ccnl_RX_handleApplication face=%p\n", (void*) from);

    ftype = ccnl_rdr_getType(fexpr);
    if (ftype != NDN_TLV_RPC_NAME) {
        DEBUGMSG(11, " (%02x) only constant fct names supported yet\n", ftype);
        ccnl_emit_RpcReturn(relay, from,
                            404, "only constant fct names supported yet", NULL);
        return -1;
    }
    fct = rpc_getBuiltinFct(fexpr);
    if (!fct) {
        DEBUGMSG(11, "  unknown RPC builtin function (type=0x%02x)\n", ftype);
        ccnl_emit_RpcReturn(relay, from, 501, "unknown function", NULL);
        return -1;
    }
    exec = rpc_exec_new();
    rc = fct(relay, from, exec, args);
    rpc_exec_free(exec);
    return rc;
}


int
ccnl_RX_localrpc(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                 unsigned char **buf, int *buflen)
{
    struct rdr_ds_s *a, *fct;
    int rc = 0, type;

    DEBUGMSG(6, "ccnl_RX_localrpc: %d bytes from face=%p (id=%d.%d)\n",
             *buflen, (void*)from, relay->id, from ? from->faceid : -1);

    while (rc == 0 && *buflen > 0) {
        if (**buf != NDN_TLV_RPC_APPLICATION) {
            DEBUGMSG(15, "  not an RPC packet\n");
            return -1;
        }
        a = ccnl_rdr_unserialize(*buf, *buflen);
        if (!a) {
            DEBUGMSG(15, "  unserialization error\n");
            return -1;
        }

//      ccnl_rdr_dump(0, a);

        type = ccnl_rdr_getType(a);
        if (type < 0 || type != NDN_TLV_RPC_APPLICATION) {
            DEBUGMSG(15, "  unserialization error %d\n", type);
            return -1;
        }
        fct = a->u.fct;
        if (ccnl_rdr_getType(fct) == NDN_TLV_RPC_NONNEGINT) // RPC return msg
            rc = ccnl_localrpc_handleReturn(relay, from, fct, a->aux);
        else
            rc = ccnl_localrpc_handleApplication(relay, from, fct, a->aux);
        if (rc < 0) {
//          DEBUGMSG(15, "  error processing RPC msg\n");
//          return rc;
        }
        ccnl_rdr_free(a);
        *buf += a->flatlen;
        *buflen -= a->flatlen;
    }

    return 0;
}

#endif // USE_SUITE_LOCALRPC

// eof
