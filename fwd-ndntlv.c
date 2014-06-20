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

#include "pkt-ndntlv.h"
#include "pkt-ndntlv-dec.c"

#ifdef CCNL_NFN
#include "krivine-common.h"
#endif

// we use one extraction routine for both interest and data pkts
struct ccnl_buf_s*
ccnl_ndntlv_extract(int hdrlen, unsigned char **data, int *datalen,
		    int *scope, int *mbf, int *min, int *max,
		    struct ccnl_prefix_s **prefix,
		    struct ccnl_buf_s **nonce,
		    struct ccnl_buf_s **ppkl,
		    unsigned char **content, int *contlen)
{
    unsigned char *start = *data - hdrlen;
    int i, len, typ;
    struct ccnl_prefix_s *p;
    struct ccnl_buf_s *buf, *n = 0, *pub = 0;
    DEBUGMSG(99, "ccnl_ndntlv_extract\n");

    if (content)
	*content = NULL;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p)
	return NULL;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
					   sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    while (ccnl_ndntlv_dehead(data, datalen, &typ, &len) == 0) {
	unsigned char *cp = *data;
	int len2 = len;
	switch (typ) {
	case NDN_TLV_Name:
	    while (len2 > 0) {
		if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i))
		    goto Bail;

		if (typ == NDN_TLV_NameComponent &&
					p->compcnt < CCNL_MAX_NAME_COMP) {
		    p->comp[p->compcnt] = cp;
		    p->complen[p->compcnt] = i;
		    p->compcnt++;
		}  // else unknown type: skip
		cp += i;
		len2 -= i;
	    }
	    break;
	case NDN_TLV_Selectors:
	    while (len2 > 0) {
		if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i))
		    goto Bail;

		if (typ == NDN_TLV_MinSuffixComponents && min)
		    *min = ccnl_ndntlv_nonNegInt(cp, i);
		if (typ == NDN_TLV_MinSuffixComponents && max)
		    *max = ccnl_ndntlv_nonNegInt(cp, i);
		if (typ == NDN_TLV_MustBeFresh && mbf)
		    *mbf = 1;
		if (typ == NDN_TLV_Exclude) {
		    DEBUGMSG(49, "warning: 'exclude' field ignored\n");
		}
		cp += i;
		len2 -= i;
	    }
	    break;
	case NDN_TLV_Nonce:
	    if (!n)
		n = ccnl_buf_new(*data, len);
	    break;
	case NDN_TLV_Scope:
	    if (scope)
		*scope = ccnl_ndntlv_nonNegInt(*data, len);
	    break;
	case NDN_TLV_Content:
	    if (content) {
		*content = *data;
		*contlen = len;
	    }
	    break;
	default:
	    break;
	}
	*data += len;
	*datalen -= len;
    }
    if (*datalen > 0)
	goto Bail;

    if (prefix)    *prefix = p;    else free_prefix(p);
    if (nonce)     *nonce = n;     else ccnl_free(n);
    if (ppkl)      *ppkl = pub;    else ccnl_free(pub);

    buf = ccnl_buf_new(start, *data - start);
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (content && *content)
	*content = buf->data + (*content - start);
    for (i = 0; i < p->compcnt; i++)
	    p->comp[i] = buf->data + (p->comp[i] - start);
    return buf;
Bail:
    free_prefix(p);
    free_2ptr_list(n, pub);
    return NULL;
}

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
    buf = ccnl_ndntlv_extract(*data - cp, data, datalen,
			      &scope, &mbf, &minsfx, &maxsfx,
			      &p, &nonce, &ppkl, &content, &contlen);
    if (!buf) {
	    DEBUGMSG(6, "  parsing error or no prefix\n"); goto Done;
    }

    if (typ == NDN_TLV_Interest) {
        if (nonce && ccnl_nonce_find_or_append(relay, nonce)) {
            DEBUGMSG(6, "  dropped because of duplicate nonce\n"); //goto Skip;
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
            if (from->ifndx >= 0){
#ifdef CCNL_NFN_MONITOR
                char monitorpacket[CCNL_MAX_PACKET_SIZE];
                int l = create_packet_log(inet_ntoa(from->peer.ip4.sin_addr),
                ntohs(from->peer.ip4.sin_port),
                c->name, (char*)c->content, c->contentlen, monitorpacket);
                sendtomonitor(relay, monitorpacket, l);
#endif
                ccnl_face_enqueue(relay, from, buf_dup(c->pkt));
            }
            else{
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
        if (!i) { // this is a new/unknown I request: create and propagate
#ifdef CCNL_NFN
        if((numOfRunningComputations < NFN_MAX_RUNNING_COMPUTATIONS) //full, do not compute but propagate
                && !memcmp(p->comp[p->compcnt-1], "NFN", 3)){
            struct ccnl_buf_s *buf2 = buf;
            //Create new prefix
            struct ccnl_prefix_s *p2 = p;
            i = ccnl_interest_new(relay, from, CCNL_SUITE_NDNTLV,
                                  &buf, &p, minsfx, maxsfx);

            i->propagate = 0; //do not forward interests for running computations
            ccnl_interest_append_pending(i, from);
            if(!i->propagate)ccnl_nfn(relay, buf2, p2, from, NULL, i, CCNL_SUITE_NDNTLV, 0);
            goto Done;
        }
#endif /*CCNL_NFN*/
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
//	c = ccnl_content_new(relay, &buf, &p, &ppkd, content, contlen);
	c = ccnl_content_new(relay, CCNL_SUITE_NDNTLV,
			     &buf, &p, NULL, content, contlen);
	if (c) { // CONFORM: Step 2 (and 3)
#ifdef CCNL_NFN
        if(debug_level >= 99){
            fprintf(stderr, "PIT Entries: \n");
            struct ccnl_interest_s *i_it;
            for(i_it = relay->pit; i_it; i_it = i_it->next){
                    int it;
                    fprintf(stderr, "    - ");
                    for(it = 0; it < i_it->prefix->compcnt; ++it){
                            fprintf(stderr, "/%s", i_it->prefix->comp[it]);
                    }
                    fprintf(stderr, " --- from-faceid: %d propagate: %d \n", i_it->from->faceid, i_it->propagate);
            }
            fprintf(stderr, "Content name: ");
            int it = 0;
            for(it = 0; it < c->name->compcnt; ++it){
                fprintf(stderr, "/%s",  c->name->comp[it]);
            }fprintf(stderr, "\n");
        }
        if(!memcmp(c->name->comp[c->name->compcnt-1], "NFN", 3)){
            struct ccnl_interest_s *i_it = NULL;
#ifdef CCNL_NACK
            if(!memcmp(c->content, ":NACK", 5)){
                DEBUGMSG(99, "Handle NACK packet: local compute!\n");
                struct ccnl_buf_s *buf2 = buf; // c->pkt
                struct ccnl_prefix_s *p2 =p; // c->name
                ccnl_nfn_nack_local_computation(relay, c->pkt, c->name, from, NULL, CCNL_SUITE_NDNTLV);
                goto Done;
            }
#endif // CCNL_NACK
            int found = 0;
            for(i_it = relay->pit; i_it;/* i_it = i_it->next*/){
                 //Check if prefix match and it is a nfn request
                 int cmp = ccnl_prefix_cmp(c->name, NULL, i_it->prefix, CMP_EXACT);
                 DEBUGMSG(99, "CMP: %d (match if zero), faceid: %d \n", cmp, i_it->from->faceid);
                 if( !ccnl_prefix_cmp(c->name, NULL, i_it->prefix, CMP_EXACT)
                         && i_it->from->faceid < 0){
                    ccnl_content_add2cache(relay, c);
                    int configid = -i_it->from->faceid;
                    DEBUGMSG(49, "Continue configuration for configid: %d\n", configid);

                    int faceid = -i_it->from->faceid;
                    i_it->propagate = 1;
                    i_it = ccnl_interest_remove(relay, i_it);
                    ccnl_nfn_continue_computation(relay, faceid, 0);
                    ++found;
                    //goto Done;
                 }
                 else{
                     i_it = i_it->next;
                 }
            }
            if(found) goto Done;
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

// eof
