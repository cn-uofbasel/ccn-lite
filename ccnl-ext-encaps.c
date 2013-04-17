/*
 * @f ccnl-ext-encaps.c
 * @b CCN lite extension: encapsulation details (fragment, schedule interface)
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
 * 2011-10-05 created
 */

// ----------------------------------------------------------------------

#ifdef USE_ENCAPS

/* see ccnl-core.h for available fragmentation protocols.
 *
 * CCNL_ENCAPS_NONE
 *  passthrough, i.e. no header is added at all
 *
 * CCNL_ENCAPS_SEQUENCED2012
 *  - a ccnb encoded header is prepended,
 *  - the driver is configurable for arbitrary MTU
 *  - packets have sequence numbers (can detect lost packets)
 *
 */

int
ccnl_is_fragment(unsigned char *data, int datalen)
{
    int num, typ;

    return dehead(&data, &datalen, &num, &typ) >= 0 &&
	typ == CCN_TT_DTAG &&
	num == CCNL_DTAG_FRAGMENT;
}

struct ccnl_buf_s*
ccnl_encaps_handle_fragment(struct ccnl_relay_s *r,
			    struct ccnl_face_s *f,
			    unsigned char *data, int datalen)
{
    struct ccnl_encaps_s *e;
    struct ccnl_buf_s *buf = NULL;
    int datalen2, num, typ, contlen = -1;
    unsigned char *data2, *content = 0;
    unsigned int flags, ourseq, ourloss, yourseq, HAS = 0;
    unsigned char flagbytes, ourseqbytes, ourlossbytes, yourseqbytes;
#define HAS_FLAGS  0x01
#define HAS_OSEQ   0x02
#define HAS_OLOS   0x04
#define HAS_YSEQ   0x08

    DEBUGMSG(8, "ccnl_encaps_handle_fragment face=%p len=%d encaps=%p\n",
	     (void*)f, datalen, f->encaps);

    data2 = data;
    datalen2 = datalen;
    if (dehead(&data2, &datalen2, &num, &typ) < 0 ||
				typ != CCN_TT_DTAG || num != CCNL_DTAG_FRAGMENT)
	return ccnl_buf_new(data, datalen);
    if (!f->encaps && f->ifndx >= 0)
	f->encaps = ccnl_encaps_new(CCNL_ENCAPS_SEQUENCED2012,
				    r->ifs[f->ifndx].mtu);
    if (!f->encaps)
	return ccnl_buf_new(data, datalen);
    data = data2;
    datalen = datalen2;

    while (dehead(&data, &datalen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	if (typ == CCN_TT_DTAG) {
	    switch(num) {
	    case CCN_DTAG_CONTENT:
		DEBUGMSG(8, "  encaps content\n");
		if (dehead(&data, &datalen, &num, &typ) != 0) return NULL;
		if (typ != CCN_TT_BLOB) return NULL;
		content = data;
		contlen = num;
		if (consume(typ, num, &data, &datalen, 0, 0) < 0) return NULL;
		break;
	    case CCNL_DTAG_FRAG_FLAGS:
		DEBUGMSG(8, "  encaps flags\n");
		if (unmkBinaryInt(&data, &datalen, &flags, &flagbytes) != 0)
		    return NULL;
		HAS |= HAS_FLAGS;
		break;
	    case CCNL_DTAG_FRAG_OSEQN:
		DEBUGMSG(8, "  encaps ourseq\n");
		if (unmkBinaryInt(&data, &datalen, &ourseq, &ourseqbytes) != 0)
		    return NULL;
		HAS |= HAS_OSEQ;
		break;
	    case CCNL_DTAG_FRAG_OLOSS:
		DEBUGMSG(8, "  encaps outloss\n");
		if (unmkBinaryInt(&data, &datalen, &ourloss, &ourlossbytes) != 0)
		    return NULL;
		HAS |= HAS_OLOS;
		break;
	    case CCNL_DTAG_FRAG_YSEQN:
		DEBUGMSG(8, "  encaps yourseq\n");
		if (unmkBinaryInt(&data, &datalen, &yourseq, &yourseqbytes) != 0)
		    return NULL;
		HAS |= HAS_YSEQ;
		break;
	    default:
		break;
	    }
	}
	if (consume(typ, num, &data, &datalen, 0, 0) < 0) return NULL;
    }

    if (!content || contlen <= 0 || !(HAS & HAS_FLAGS) || !(HAS & HAS_OSEQ)) {
	DEBUGMSG(8, "  encaps problem: %p, %d, 0x%04x\n",
		 (void*)content, contlen, HAS);
	return NULL;
    }

    e = f->encaps;
    DEBUGMSG(8, "  encaps %p protocol=%d, flags=%04x, seq=%d (%d)\n",
	     (void*)e, e->protocol, flags, ourseq, e->recvseq);
    if (e->recvseq != ourseq) {
	// should increase error counter here
	if (e->defrag) {
	    DEBUGMSG(8, ">> %p seqnum mismatch (%d/%d), dropped old defrag buf\n",
		     (void*)e, ourseq, e->recvseq);
	    ccnl_free(e->defrag);
	    e->defrag = NULL;
	}
    }
    switch(flags & (CCNL_DTAG_FRAG_FLAG_FIRST|CCNL_DTAG_FRAG_FLAG_LAST)) {
    case CCNL_DTAG_FRAG_FLAG_FIRST|CCNL_DTAG_FRAG_FLAG_LAST: // single packet
	DEBUGMSG(8, ">> %p single\n", (void*)e);
	if (e->defrag) {
	    DEBUGMSG(8, ">> %p single drop\n", (void*)e);
	    ccnl_free(e->defrag);
	    e->defrag = NULL;
	}
	buf = ccnl_buf_new(content, contlen);
	break;
    case CCNL_DTAG_FRAG_FLAG_FIRST: // start of fragment sequence
	DEBUGMSG(8, ">> %p start\n", (void*)e);
	if (e->defrag) {
	    DEBUGMSG(8, ">> %p start drop\n", (void*)e);
	    ccnl_free(e->defrag);
	}
	e->defrag = ccnl_buf_new(content, contlen);
	break;
    case CCNL_DTAG_FRAG_FLAG_LAST: // end of fragment sequence
	DEBUGMSG(8, ">> %p end\n", (void*)e);
	if (!e->defrag) break;
	buf = ccnl_buf_new(NULL, e->defrag->datalen + contlen);
	if (buf) {
	    memcpy(buf->data, e->defrag->data, e->defrag->datalen);
	    memcpy(buf->data + e->defrag->datalen, content, contlen);
	}
	ccnl_free(e->defrag);
	e->defrag = NULL;
	break;
    case 0x00:  // fragment in the middle of a squence
    default:
	DEBUGMSG(8, ">> %p middle\n", (void*)e);
	if (!e->defrag) break;
	buf = ccnl_buf_new(NULL, e->defrag->datalen + contlen);
	if (buf) {
	    memcpy(buf->data, e->defrag->data, e->defrag->datalen);
	    memcpy(buf->data + e->defrag->datalen, content, contlen);
	    ccnl_free(e->defrag);
	    e->defrag = buf;
	    buf = NULL;
	} else {
	    ccnl_free(e->defrag);
	    e->defrag = NULL;
	}
	break;
    }
    if (ourseqbytes <= sizeof(int) && ourseqbytes > 1)
	e->recvseqbytes = ourseqbytes;
    // next expected seq number:
    e->recvseq = (ourseq + 1) & ((1<<(8*ourseqbytes)) - 1);

    if (buf) {
	DEBUGMSG(8, ">> %p defrag buffer len is %d bytes\n",
		 (void*)e, buf->datalen);
    }

    return buf;
}

// ----------------------------------------------------------------------

struct ccnl_encaps_s*
ccnl_encaps_new(int protocol, int mtu)
{
    struct ccnl_encaps_s *e = NULL;

    DEBUGMSG(8, "ccnl_encaps_new proto=%d mtu=%d\n", protocol, mtu);

    switch(protocol) {
    case CCNL_ENCAPS_SEQUENCED2012:
      e = (struct ccnl_encaps_s*) ccnl_calloc(1, sizeof(struct ccnl_encaps_s));
	if (e) {
	    e->protocol = protocol;
	    e->mtu = mtu;
	    e->flagbytes = 1;
	    e->sendseqbytes =   2;
	    e->losscountbytes = 2;
	    e->recvseqbytes =   2;
	}
	break;
    case CCNL_ENCAPS_NONE:
    default:
	break;
    }
    return e;
}

void
ccnl_encaps_start(struct ccnl_encaps_s *e, struct ccnl_buf_s *buf,
		  int ifndx, sockunion *dst)
{
    if (!e) return;
    e->ifndx = ifndx;
    memcpy(&e->dest, dst, sizeof(*dst));
    if (e->bigpkt)
	ccnl_free(e->bigpkt);
    e->bigpkt = buf;
    e->sendoffs = 0;
}

int
ccnl_encaps_getfragcount(struct ccnl_encaps_s *e, int origlen, int *totallen)
{
    int cnt = 0, len = 0;
    unsigned char dummy[256];
    int hdrlen, blobtaglen, datalen;
    int offs = 0;

    if (!e)
      cnt = 1;
    else
      while (offs < origlen) {
	hdrlen = mkHeader(dummy, CCNL_DTAG_FRAGMENT, CCN_TT_DTAG);
	hdrlen += mkBinaryInt(dummy, CCNL_DTAG_FRAG_FLAGS, CCN_TT_DTAG,
			      0, e->flagbytes);
	hdrlen += mkBinaryInt(dummy, CCNL_DTAG_FRAG_OSEQN, CCN_TT_DTAG,
			      0, e->sendseqbytes);
	hdrlen += mkBinaryInt(dummy, CCNL_DTAG_FRAG_OLOSS, CCN_TT_DTAG,
			      0, e->losscountbytes);
	hdrlen += mkBinaryInt(dummy, CCNL_DTAG_FRAG_YSEQN, CCN_TT_DTAG,
			      0, e->recvseqbytes);

	hdrlen += mkHeader(dummy, CCN_DTAG_CONTENT, CCN_TT_DTAG);
	blobtaglen = mkHeader(dummy, e->mtu - hdrlen - 1, CCN_TT_BLOB);
	datalen = e->mtu - hdrlen - blobtaglen - 1;
	if (datalen > (origlen - offs))
	    datalen = origlen - offs;
	hdrlen += mkHeader(dummy, datalen, CCN_TT_BLOB);
	len += hdrlen + datalen + 1;
	offs += datalen;
	cnt++;
    }
    if (totallen)
	*totallen = len;
    return cnt;
}

struct ccnl_buf_s*
ccnl_encaps_getnextfragment(struct ccnl_encaps_s *e, int *ifndx,
			    sockunion *su)
{
    struct ccnl_buf_s *buf = 0;
    unsigned char header[256];
    int hdrlen, blobtaglen, datalen, flagoffs;

    DEBUGMSG(2, "ccnl_encaps_getnextfragment e=%p\n", (void*)e);

    if (!e->bigpkt) {
	DEBUGMSG(2, "  no packet to fragment yet\n");
	return NULL;
    }
    DEBUGMSG(2, "  %d bytes to fragment, offset=%d\n",
	     e->bigpkt->datalen, e->sendoffs);

    hdrlen = mkHeader(header, CCNL_DTAG_FRAGMENT, CCN_TT_DTAG);   // fragment
    hdrlen += mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG_FLAGS, CCN_TT_DTAG,
			  0, e->flagbytes);
    flagoffs = hdrlen - 2;
    hdrlen += mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG_OSEQN, CCN_TT_DTAG,
			  e->sendseq, e->sendseqbytes);
    hdrlen += mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG_YSEQN, CCN_TT_DTAG,
			  e->recvseq, e->recvseqbytes);
    hdrlen += mkBinaryInt(header+hdrlen, CCNL_DTAG_FRAG_OLOSS, CCN_TT_DTAG,
			  e->losscount, e->losscountbytes);
    hdrlen += mkHeader(header+hdrlen, CCN_DTAG_CONTENT, CCN_TT_DTAG);

    blobtaglen = mkHeader(header + hdrlen, e->mtu - hdrlen - 2, CCN_TT_BLOB);
    datalen = e->mtu - hdrlen - blobtaglen - 2;
    if (datalen > (e->bigpkt->datalen - e->sendoffs))
	datalen = e->bigpkt->datalen - e->sendoffs;
    hdrlen += mkHeader(header + hdrlen, datalen, CCN_TT_BLOB);

    buf = ccnl_buf_new(NULL, hdrlen + datalen + 2);
    if (!buf)
	return NULL;
    memcpy(buf->data, header, hdrlen);
    memcpy(buf->data + hdrlen, e->bigpkt->data + e->sendoffs, datalen);
    buf->data[hdrlen + datalen] = '\0'; // end of content
    buf->data[hdrlen + datalen + 1] = '\0'; // end of fragment

    if (datalen >= e->bigpkt->datalen) { // single
	buf->data[flagoffs + e->flagbytes - 1] =
			CCNL_DTAG_FRAG_FLAG_FIRST | CCNL_DTAG_FRAG_FLAG_LAST;
	ccnl_free(e->bigpkt);
	e->bigpkt = NULL;
    } else if (e->sendoffs == 0) // start
	buf->data[flagoffs + e->flagbytes - 1] = CCNL_DTAG_FRAG_FLAG_FIRST;
    else if(datalen >= (e->bigpkt->datalen - e->sendoffs)) { // end
	buf->data[flagoffs + e->flagbytes - 1] = CCNL_DTAG_FRAG_FLAG_LAST;
	ccnl_free(e->bigpkt);
	e->bigpkt = NULL;
    } else
	buf->data[flagoffs + e->flagbytes - 1] = 0x00; // middle

    e->sendoffs += datalen;
    e->sendseq++;

    if (ifndx)
	*ifndx = e->ifndx;
    if (su)
	memcpy(su, &e->dest, sizeof(*su));
    return buf;
}

int
ccnl_encaps_nomorefragments(struct ccnl_encaps_s *e)
{
    if (!e || !e->bigpkt)
	return 1;
    return e->bigpkt->datalen <= e->sendoffs;
}

void
ccnl_encaps_destroy(struct ccnl_encaps_s *e)
{
    if (e) {
	if (e->bigpkt)
	    ccnl_free(e->bigpkt);
	if (e->defrag)
	    ccnl_free(e->defrag);
	ccnl_free(e);
    }
}

#endif // USE_ENCAPS

// eof
