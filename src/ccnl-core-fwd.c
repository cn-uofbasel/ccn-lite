/*
 * @f ccnl-core-fwd.c
 * @b CCN lite, the collection of suite specific forwarding logics
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
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

// content/data handling (common across all suites)
void
ccnl_fwd_handleContent(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                       struct ccnl_content_s *c)
{
    if (!c)
        return;
     // CONFORM: Step 2 (and 3)
#ifdef USE_NFN
    if (ccnl_nfnprefix_isNFN(c->name)) {
        if (ccnl_nfn_RX_result(relay, from, c))
            return;
        DEBUGMSG_CFWD(VERBOSE, "no running computation found \n");
    }
#endif
    if (!ccnl_content_serve_pending(relay, c)) { // unsolicited content
        // CONFORM: "A node MUST NOT forward unsolicited data [...]"
        DEBUGMSG_CFWD(DEBUG, "  removed because no matching interest\n");
        free_content(c);
        return;
    }
    if (relay->max_cache_entries != 0) { // it's set to -1 or a limit
        DEBUGMSG_CFWD(DEBUG, "  adding content to cache\n");
        ccnl_content_add2cache(relay, c);
    } else {
        DEBUGMSG_CFWD(DEBUG, "  content not added to cache\n");
        free_content(c);
    }
}

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNB

// helper proc: work on a message, top level type is already stripped
int
ccnl_ccnb_fwd(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
              unsigned char **data, int *datalen)
{
    int rc= -1, scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkd=0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0;
    DEBUGMSG_CFWD(DEBUG, "ccnb fwd (%d bytes left)\n", *datalen);

    buf = ccnl_ccnb_extract(data, datalen, &scope, &aok, &minsfx,
                            &maxsfx, &p, &nonce, &ppkd, &content, &contlen);
    if (!buf) {
        DEBUGMSG_CFWD(DEBUG, "  parsing error or no prefix\n");
        goto Done;
    }
    if (nonce && ccnl_nonce_find_or_append(ccnl, nonce)) {
        DEBUGMSG_CFWD(DEBUG, "  dropped because of duplicate nonce\n");
        goto Skip;
    }
    if (buf->data[0] == 0x01 && buf->data[1] == 0xd2) { // interest
        DEBUGMSG_CFWD(DEBUG, "  interest=<%s>\n", ccnl_prefix_to_path(p));
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
                DEBUGMSG_CFWD(DEBUG, "  matching content for interest, content %p\n",
                         (void *) c);
                if (from->ifndx >= 0) {
                    ccnl_nfn_monitor(ccnl, from, c->name, c->content,
                                     c->contentlen);
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
              // Since the interest msg may be required in future, it is not
              // possible to delete the interest/prefix here
              return rc;
        }
#endif
        if (!i) {
            i = ccnl_interest_new(ccnl, from, CCNL_SUITE_CCNB,
                                  &buf, &p, minsfx, maxsfx);
            if (ppkd)
                i->details.ccnb.ppkd = ppkd, ppkd = NULL;
            if (i) { // CONFORM: Step 3 (and 4)
                DEBUGMSG_CFWD(DEBUG, "  created new interest entry %p\n", (void *)i);
                if (scope > 2)
                    ccnl_interest_propagate(ccnl, i);
            }
        } else if (scope > 2 && (from->flags & CCNL_FACE_FLAGS_FWDALLI)) {
            DEBUGMSG_CFWD(DEBUG, "  old interest, nevertheless propagated %p\n",
                     (void *) i);
            ccnl_interest_propagate(ccnl, i);
        }
        if (i) { // store the I request, for the incoming face (Step 3)
            DEBUGMSG_CFWD(DEBUG, "  appending interest entry %p\n", (void *) i);
            ccnl_interest_append_pending(i, from);
        }
    } else { // content
        DEBUGMSG_CFWD(DEBUG, "  content=<%s>\n", ccnl_prefix_to_path(p));
        
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
            if (buf_equal(c->pkt, buf))
                goto Skip; // content is dup
        c = ccnl_content_new(ccnl, CCNL_SUITE_CCNB,
                             &buf, &p, &ppkd, content, contlen);
        ccnl_fwd_handleContent(ccnl, from, c);
    }

Skip:
    rc = 0;
Done:
    free_prefix(p);
    free_3ptr_list(buf, nonce, ppkd);
    return rc;
}

// loops over a frame until empty or error
int
ccnl_ccnb_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                    unsigned char **data, int *datalen)
{
    int rc = 0, num, typ;
    DEBUGMSG_CFWD(DEBUG, "ccnl_RX_ccnb: %d bytes from face=%p (id=%d.%d)\n",
             *datalen, (void*)from, relay->id, from ? from->faceid : -1);

    while (rc >= 0 && *datalen > 0) {
        if (ccnl_ccnb_dehead(data, datalen, &num, &typ) || typ != CCN_TT_DTAG)
            return -1;
        switch (num) {
        case CCN_DTAG_INTEREST:
        case CCN_DTAG_CONTENTOBJ:
            rc = ccnl_ccnb_fwd(relay, from, data, datalen);
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
            DEBUGMSG_CFWD(DEBUG, "  unknown datagram type %d\n", num);
            return -1;
        }
    }
    return rc;
}

#endif // USE_SUITE_CCNB

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNTLV

// helper proc: work on a message, buffer is passed without the fixed header
int
ccnl_ccntlv_fwd(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                struct ccnx_tlvhdr_ccnx2015_s *hdrptr,
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
    unsigned char typ = hdrptr->pkttype;
    unsigned char hdrlen = *data - (unsigned char*)hdrptr;
    DEBUGMSG_CFWD(DEBUG, "ccnl_ccntlv_forwarder (%d bytes left, hdrlen=%d)\n",
             *datalen, hdrlen);

    buf = ccnl_ccntlv_extract(hdrlen, data, datalen, &p, &keyid, &keyidlen,
                              &lastchunknum, &content, &contlen);
    if (!buf) {
            DEBUGMSG_CFWD(DEBUG, "  parsing error or no prefix\n");
            goto Done;
    }

    if (typ == CCNX_PT_Interest) {
        DEBUGMSG_CFWD(DEBUG, "  interest=<%s>\n", ccnl_prefix_to_path(p));
        // CONFORM: Step 1: search for matching local content
        for (c = relay->contents; c; c = c->next) {
            if (c->suite != CCNL_SUITE_CCNTLV)
                continue;
            // TODO: check keyid
            // TODO: check freshness, kind-of-reply
            if (ccnl_prefix_cmp(c->name, NULL, p, CMP_EXACT))
                continue;
            DEBUGMSG_CFWD(DEBUG, "  matching content for interest, content %p\n",
                     (void *) c);
            if (from->ifndx >= 0){
                ccnl_nfn_monitor(relay, from, c->name, c->content,
                                 c->contentlen);
                ccnl_face_enqueue(relay, from, buf_dup(c->pkt));
            } else {
                ccnl_app_RX(relay, c);
            }
            goto Skip;
        }
        DEBUGMSG_CFWD(DEBUG, "  no matching content for interest\n");
        // CONFORM: Step 2: check whether interest is already known
        for (i = relay->pit; i; i = i->next) {
            if (i->suite != CCNL_SUITE_CCNTLV)
                continue;
            if (!ccnl_prefix_cmp(i->prefix, NULL, p, CMP_EXACT))
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
            i = ccnl_interest_new(relay, from, CCNL_SUITE_CCNTLV,
                                  &buf, &p, 0, hdrptr->hoplimit - 1);
            if (i) { // CONFORM: Step 3 (and 4)
                // TODO keyID restriction
                DEBUGMSG_CFWD(DEBUG, "  created new interest entry %p\n",
                         (void *) i);
                ccnl_interest_propagate(relay, i);
            }
        } else if ((from->flags & CCNL_FACE_FLAGS_FWDALLI)) {
            DEBUGMSG_CFWD(DEBUG, "  old interest, nevertheless propagated %p\n",
                     (void *) i);
            ccnl_interest_propagate(relay, i);
        }
        if (i) { // store the I request, for the incoming face (Step 3)
            DEBUGMSG_CFWD(DEBUG, "  appending interest entry %p\n", (void *) i);
            ccnl_interest_append_pending(i, from);
        }
    } else if (typ == CCNX_PT_NACK) {
        DEBUGMSG_CFWD(DEBUG, "  NACK=<%s>\n", ccnl_prefix_to_path(p));
    } else if (typ == CCNX_PT_Data) { // data packet with content
        DEBUGMSG_CFWD(DEBUG, "  data=<%s>\n", ccnl_prefix_to_path(p));

        // CONFORM: Step 1:
        for (c = relay->contents; c; c = c->next)
            if (buf_equal(c->pkt, buf))
                goto Skip; // content is dup
        c = ccnl_content_new(relay, CCNL_SUITE_CCNTLV,
                             &buf, &p, NULL, content, contlen);
        ccnl_fwd_handleContent(relay, from, c);
    }

Skip:
    rc = 0;
Done:
    free_prefix(p);
    ccnl_free(buf);

    return rc;
}

// process one CCNTLV packet, return <0 if no bytes consumed or error
int
ccnl_ccntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen)
{
    int payloadlen, hoplimit, endlen;
    unsigned short hdrlen;
    unsigned char *end;
    struct ccnx_tlvhdr_ccnx2015_s *hp;

    DEBUGMSG_CFWD(DEBUG, "ccnl_RX_ccntlv: %d bytes from face=%p (id=%d.%d)\n",
             *datalen, (void*)from, relay->id, from ? from->faceid : -1);

    if (**data != CCNX_TLV_V1 ||
                        *datalen < sizeof(struct ccnx_tlvhdr_ccnx2015_s))
        return -1;

    hp = (struct ccnx_tlvhdr_ccnx2015_s*) *data;
    hdrlen = hp->hdrlen; // ntohs(hp->hdrlen);
    if (hdrlen > *datalen) // not enough bytes for a full header
        return -1;

    payloadlen = ntohs(hp->pktlen) - hdrlen;
    *data += hdrlen;
    *datalen -= hdrlen;
    if (payloadlen <= 0 ||
              payloadlen > *datalen) // not enough data to reconstruct message
            return -1;

    end = *data + payloadlen;
    endlen = *datalen - payloadlen;

    hoplimit = hp->hoplimit - 1;
    if ((hp->pkttype == CCNX_PT_Interest || hp->pkttype == CCNX_PT_NACK)
                                           && hoplimit <= 0) { // drop it
        *data = end;
        *datalen = endlen;
        return 0;
    } else
        hp->hoplimit = hoplimit;

    return ccnl_ccntlv_fwd(relay, from, hp, data, datalen);
}

#endif // USE_SUITE_CCNTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_IOTTLV

// process one IOTTLV packet, return <0 if no bytes consumed or error
int
ccnl_iottlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen)
{
    int rc=-1, contlen, len, typ;
    struct ccnl_buf_s *buf = 0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *start = *data, *content = 0;
    DEBUGMSG_CFWD(TRACE, "ccnl_iottlv_forwarder (%d bytes left)\n", *datalen);

    if (ccnl_iottlv_dehead(data, datalen, &typ, &len))
        return -1;

    // typ must be Request or Reply

    buf = ccnl_iottlv_extract(start, data, datalen, &p, NULL,
                              &content, &contlen);
    if (!buf) {
        DEBUGMSG_CFWD(WARNING, "  parsing error or no prefix\n");
        goto Done;
    }

    if (typ == IOT_TLV_Request) {
        DEBUGMSG_CFWD(DEBUG, "  request=<%s>\n", ccnl_prefix_to_path(p));
        if (local_producer(relay, p, from, content, contlen, buf))
            return 0;
        for (c = relay->contents; c; c = c->next) {
            if (c->suite != CCNL_SUITE_IOTTLV)
                continue;
            if (ccnl_prefix_cmp(c->name, NULL, p, CMP_EXACT))
                continue;
            DEBUGMSG_CFWD(DEBUG, "  matching content for interest, content %p\n", (void *) c);
            if (from->ifndx >= 0) {
                ccnl_nfn_monitor(relay, from, c->name, c->content, c->contentlen);
                ccnl_face_enqueue(relay, from, buf_dup(c->pkt));
            } else {
                ccnl_app_RX(relay, c);
            }
            goto Skip;
        }
        DEBUGMSG_CFWD(DEBUG, "  no matching content for interest\n");
        // CONFORM: Step 2: check whether interest is already known
        for (i = relay->pit; i; i = i->next) {
            if (i->suite == CCNL_SUITE_IOTTLV &&
                !ccnl_prefix_cmp(i->prefix, NULL, p, CMP_EXACT))
                break;
        }
        // this is a new/unknown I request: create and propagate
#ifdef USE_NFN
        if (!i && ccnl_nfnprefix_isNFN(p)) { // NFN PLUGIN CALL
            if (ccnl_nfn_RX_request(relay, from, CCNL_SUITE_IOTTLV,
                                    &buf, &p, 0, 0))
                // Since the interest msg may be required in future it is
                // not possible to delete the interest/prefix here
                // return rc;
                goto Done;
        }
#endif
        if (!i) {
            i = ccnl_interest_new(relay, from, CCNL_SUITE_IOTTLV,
                                  &buf, &p, 0, 0);
            if (i) { // CONFORM: Step 3 (and 4)
                DEBUGMSG_CFWD(DEBUG, "  created new interest entry %p\n", (void *) i);
                ccnl_interest_propagate(relay, i);
            }
        } else if (from->flags & CCNL_FACE_FLAGS_FWDALLI) {
            DEBUGMSG_CFWD(DEBUG, "  old interest, nevertheless propagated %p\n", (void *) i);
            ccnl_interest_propagate(relay, i);
        }
        if (i) { // store the I request, for the incoming face (Step 3)
            DEBUGMSG_CFWD(DEBUG, "  appending interest entry %p\n", (void *) i);
            ccnl_interest_append_pending(i, from);
        }
    } else { // data packet with content -------------------------------------
        DEBUGMSG_CFWD(DEBUG, "  reply=<%s>\n", ccnl_prefix_to_path(p));

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
            if (buf_equal(c->pkt, buf))
                goto Skip; // content is dup
        c = ccnl_content_new(relay, CCNL_SUITE_IOTTLV,
                             &buf, &p, NULL /* ppkd */ , content, contlen);
        ccnl_fwd_handleContent(relay, from, c);
    }

Skip:
    rc = 0;
Done:
    free_prefix(p);
    ccnl_free(buf);
    return rc;
}

#endif // USE_SUITE_IOTTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_NDNTLV

// process one NDNTLV packet, return <0 if no bytes consumed or error
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
    DEBUGMSG_CFWD(DEBUG, "ccnl_ndntlv_forwarder (%d bytes left)\n", *datalen);

    if (ccnl_ndntlv_dehead(data, datalen, &typ, &len))
        return -1;
    buf = ccnl_ndntlv_extract(*data - cp,
                              data, datalen,
                              &scope, &mbf, &minsfx, &maxsfx, 0, 
                              &p, &tracing, &nonce, &ppkl, &content, &contlen);
    if (!buf) {
        DEBUGMSG_CFWD(DEBUG, "  parsing error or no prefix\n");
        goto Done;
    }

    if (typ == NDN_TLV_Interest) {
        if (nonce && ccnl_nonce_find_or_append(relay, nonce)) {
            DEBUGMSG_CFWD(DEBUG, "  dropped because of duplicate nonce\n");
            goto Skip;
        }
        DEBUGMSG_CFWD(DEBUG, "  interest=<%s>\n", ccnl_prefix_to_path(p));
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
            DEBUGMSG_CFWD(DEBUG, "  matching content for interest, content %p\n",
                     (void *) c);
            if (from->ifndx >= 0) {
                ccnl_nfn_monitor(relay, from, c->name, c->content,
                                 c->contentlen);
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
                DEBUGMSG_CFWD(DEBUG,
                         "  created new interest entry %p\n", (void *) i);
                if (scope > 2)
                    ccnl_interest_propagate(relay, i);
            }
        } else if (scope > 2 && (from->flags & CCNL_FACE_FLAGS_FWDALLI)) {
            DEBUGMSG_CFWD(DEBUG, "  old interest, nevertheless propagated %p\n",
                     (void *) i);
            ccnl_interest_propagate(relay, i);
        }
        if (i) { // store the I request, for the incoming face (Step 3)
            DEBUGMSG_CFWD(DEBUG, "  appending interest entry %p\n", (void *) i);
            ccnl_interest_append_pending(i, from);
        }
    } else { // data packet with content -------------------------------------
        DEBUGMSG_CFWD(DEBUG, "  data=<%s>\n", ccnl_prefix_to_path(p));

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
            if (buf_equal(c->pkt, buf))
                goto Skip; // content is dup
        c = ccnl_content_new(relay, CCNL_SUITE_NDNTLV,
                             &buf, &p, NULL /* ppkd */ , content, contlen);
        ccnl_fwd_handleContent(relay, from, c);
    }

Skip:
    rc = 0;
Done:
    free_prefix(p);
    free_3ptr_list(buf, nonce, ppkl);

    return rc;
}

#endif // USE_SUITE_NDNTLV

// eof
