/*
 * @f fwd-ndntlv.c
 * @b CCN lite, NDN forwarding (using the new TLV format of Mar 2014)
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

#ifdef USE_SUITE_NDNTLV

#include "pkt-ndntlv-dec.c"

int
ccnl_ndntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		      unsigned char **data, int *datalen)
{
    int len, rc=-1, typ;
    int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0, *cp = *data;
    DEBUGMSG(99, "ccnl_ndntlv_forwarder (%d bytes left)\n", *datalen);

    if (ccnl_ndntlv_dehead(data, datalen, &typ, &len))
	return -1;
    buf = ccnl_ndntlv_extract(*data - cp,
			      data, datalen,
			      &scope, &mbf, &minsfx, &maxsfx, 0, 0,
			      &p, &nonce, &ppkl, &content, &contlen);
    if (!buf) {
	DEBUGMSG(6, "  parsing error or no prefix\n");
	goto Done;
    }

    if (typ == NDN_TLV_Interest) {
	if (nonce) {
	    if (ccnl_nonce_find_or_append(relay, nonce)) {
		DEBUGMSG(6, "  dropped because of duplicate nonce\n");
		//goto Skip;
	    }
	    ccnl_free(nonce);
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
		    //Since the interest msg may be required in future it is not possible
		    //to delete the interest/prefix here
                return rc;
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

// eof
