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


// we use one extraction routine for both interest and data pkts
struct ccnl_buf_s*
ccnl_ccntlv_extract(int hdrlen,
		    unsigned char **data, int *datalen, int pkttyp,
		    struct ccnl_prefix_s **prefix,
		    int *scope,
		    unsigned char **keyid, int *keyidlen,
		    unsigned char **content, int *contlen)
{
    unsigned char *start = *data - hdrlen;
    int i;
    unsigned int len, typ;
    struct ccnl_prefix_s *p;
    struct ccnl_buf_s *buf;
    DEBUGMSG(99, "ccnl_ccntlv_extract\n");

    if (content)
	*content = NULL;
    if (keyid)
	*keyid = NULL;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p)
	return NULL;
    p->suite = CCNL_SUITE_CCNTLV;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
					   sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    while (ccnl_ccntlv_dehead(data, datalen, &typ, &len) == 0) {
	unsigned char *cp = *data, *cp2;
	int len2 = len;

	switch (typ) {
	case CCNX_TLV_G_Name:
	    while (len2 > 0) {
		unsigned int len3;
		cp2 = cp;
		if (ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3))
		    goto Bail;

		if (p->compcnt < CCNL_MAX_NAME_COMP) {
		    p->comp[p->compcnt] = cp2;
		    p->complen[p->compcnt] = cp - cp2 + len3;
		    p->compcnt++;
		}  // else out of name component memory: skip
		cp += len3;
		len2 -= len3;
	    }
	    break;

	case CCNX_TLV_C_KeyID:   // same as CCNX_TLV_I_KeyID
	    if (keyid && keyidlen) {
		*keyid = *data;
		*keyidlen = len;
	    }
	    break;

	case CCNX_TLV_I_Scope:
	    if (scope)
		*scope = **data;
	    break;

	case CCNX_TLV_C_Contents:
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

    buf = ccnl_buf_new(start, *data - start);
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (content && *content)
	*content = buf->data + (*content - start);
    for (i = 0; i < p->compcnt; i++)
	    p->comp[i] = buf->data + (p->comp[i] - start);
    if (p->path)
	p->path = buf->data + (p->path - start);
    return buf;
Bail:
    free_prefix(p);
//    free_2ptr_list(n, pub);
    return NULL;
}

// work on a message (buffer passed without the fixed header)
int
ccnl_ccntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		      struct ccnx_tlvhdr_ccnx201311_s *hdrptr, int hoplimit,
		      unsigned char **data, int *datalen)
{
    int rc = -1;
    unsigned int typ, len;
    int scope = -1, keyidlen, contlen;
    struct ccnl_buf_s *buf = 0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0, *keyid = 0;
    DEBUGMSG(99, "ccnl_ccntlv_forwarder (%d bytes left)\n", *datalen);

    if (ccnl_ccntlv_dehead(data, datalen, &typ, &len))
	return -1;
    buf = ccnl_ccntlv_extract(*data - (unsigned char*)hdrptr, data, datalen,
			      typ, &p, &scope, &keyid, &keyidlen,
			      &content, &contlen);
    if (!buf) {
	    DEBUGMSG(6, "  parsing error or no prefix\n"); goto Done;
    }

    if (typ == CCNX_TLV_TL_Interest) {
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
	if (scope >= 0) // request for local content only, and none was found
	    goto Skip;
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
	if (!i && ccnl_isNFNrequest(p)) { // NFN PLUGIN CALL
	    if (ccnl_nfn_RX_request(relay, from, CCNL_SUITE_CCNTLV, buf,
				    p, 0, 0))
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

    } else if (typ == CCNX_TLV_TL_Object) { // data packet with content
	DEBUGMSG(6, "  data=<%s>\n", ccnl_prefix_to_path(p));
	ccnl_print_stats(relay, STAT_RCV_C); //log count recv_content

        // CONFORM: Step 1:
	for (c = relay->contents; c; c = c->next)
	    if (buf_equal(c->pkt, buf)) goto Skip; // content is dup
	c = ccnl_content_new(relay, CCNL_SUITE_CCNTLV,
			     &buf, &p, NULL, content, contlen);
	if (c) { // CONFORM: Step 2 (and 3)
#ifdef USE_NFN
	    if (ccnl_isNFNrequest(c->name)) {
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
    int rc = 0, opthdrlen, endlen;
    unsigned int typ, len;
    unsigned char *opthdr, *end;
    DEBUGMSG(6, "ccnl_RX_ccntlv: %d bytes from face=%p (id=%d.%d)\n",
	     *datalen, (void*)from, relay->id, from ? from->faceid : -1);

next:
    while (rc >= 0 && **data == CCNX_TLV_V0 &&
			*datalen > sizeof(struct ccnx_tlvhdr_ccnx201311_s)) {
	struct ccnx_tlvhdr_ccnx201311_s *hp =
				(struct ccnx_tlvhdr_ccnx201311_s*) *data;
	int hoplimit = -1;

	if ((sizeof(struct ccnx_tlvhdr_ccnx201311_s) + ntohs(hp->hdrlen)) > *datalen)
	    return -1;
	*data += sizeof(struct ccnx_tlvhdr_ccnx201311_s) + ntohs(hp->hdrlen);
	*datalen -= sizeof(struct ccnx_tlvhdr_ccnx201311_s) + ntohs(hp->hdrlen);
	if (*datalen < ntohs(hp->msglen)) 
	    return -1;
	end = *data + ntohs(hp->msglen);
	endlen = *datalen - ntohs(hp->msglen);

	opthdrlen = ntohs(hp->hdrlen);
	opthdr = (unsigned char*) (hp + 1);
	while (opthdrlen > 0) {
	    if (ccnl_ccntlv_dehead(&opthdr, &opthdrlen, &typ, &len))
		break;
	    if (opthdrlen > 2 && typ == CCNX_TLV_PH_Hoplimit &&
				ntohs(hp->msgtype) == CCNX_TLV_TL_Interest) {
		hoplimit = ntohs(*(unsigned short*)opthdr) - 1;
		if (hoplimit <= 0) { // drop this interest
		    *data = end;
		    *datalen = endlen;
		    goto next;
		}
		*(unsigned short*)opthdr = hoplimit;
		opthdr += 2;
		opthdrlen -= 2;
	    }
	}
	rc = ccnl_ccntlv_forwarder(relay, from, hp, hoplimit, data, datalen);
    }
    return rc;
}

#endif // USE_SUITE_CCNTLV

// eof
