/*
 * @f ccnl-ext-encaps.c
 * @b CCN lite extension: encapsulation details (fragment, schedule interface)
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
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

/*
    e = f->encaps;
    if (!e || dehead(&data, &datalen, &num, &typ) < 0) {
	DEBUGMSG(8, "  no encaps/fragmentation found %p\n", (void*)e);
    }
    if () return NULL;
*/
    data2 = data;
    datalen2 = datalen;
    if (dehead(&data2, &datalen2, &num, &typ) < 0 ||
				typ != CCN_TT_DTAG || num != CCNL_DTAG_FRAGMENT)
	return ccnl_buf_new(data, datalen);
    if (!f->encaps && f->ifndx >= 0)
	f->encaps = ccnl_encaps_new(CCNL_ENCAPS_SEQUENCED2012, r->ifs[f->ifndx].mtu);
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
/*		data += num;
		datalen -= num;
*/
		if (consume(typ, num, &data, &datalen, 0, 0) < 0) return NULL;
		break;
	    case CCNL_DTAG_FRAG_FLAGS:
		DEBUGMSG(8, "  encaps flags\n");
		if (unmkBinaryInt(&data, &datalen, &flags, &flagbytes) != 0)
		    return NULL;
		HAS |= HAS_FLAGS;
//		continue;
		break;
	    case CCNL_DTAG_FRAG_OSEQN:
		DEBUGMSG(8, "  encaps ourseq\n");
		if (unmkBinaryInt(&data, &datalen, &ourseq, &ourseqbytes) != 0)
		    return NULL;
		HAS |= HAS_OSEQ;
//		continue;
		break;
	    case CCNL_DTAG_FRAG_OLOSS:
		DEBUGMSG(8, "  encaps outloss\n");
		if (unmkBinaryInt(&data, &datalen, &ourloss, &ourlossbytes) != 0)
		    return NULL;
		HAS |= HAS_OLOS;
//		continue;
		break;
	    case CCNL_DTAG_FRAG_YSEQN:
		DEBUGMSG(8, "  encaps yourseq\n");
		if (unmkBinaryInt(&data, &datalen, &yourseq, &yourseqbytes) != 0)
		    return NULL;
		HAS |= HAS_YSEQ;
//		continue;
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
/*
    {
	int i;
	unsigned char *cp;
	for (i = 0, cp = buf->data; i < buf->datalen; i++, cp++) {
	    if (i && !(i % 8)) printf(",\n");
	    if ((i % 8)) printf(", ");
	    printf("0x%02x", *cp);
	}
	if ((i%8))
	    printf("\n");
    }
*/
    }

    return buf;
}


#ifdef XXX
// dummy procedure turning a datagram into fragments (currently only one)
struct ccnl_buf_s*
ccnl_encaps_fragment(struct ccnl_relay_s *ccnl, struct ccnl_encaps_s *encaps,
		     struct ccnl_buf_s *buf)
{
    buf->next = NULL;
    return buf;
}
#endif

// ----------------------------------------------------------------------

// return 0 if no fragment header will be added
int
ccnl_if_fraghdrlen(int encaps, int mtu, int *datalen, int offs)
{
#ifdef XXX
    struct eth2011hdr {
#if __BYTE_ORDER == __BIG_ENDIAN
        unsigned char  version:4; // version is currently 0
        unsigned char  type:4;    // type is 0 for short counter fields
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN
        unsigned char  type:4;
        unsigned char  version:4;
#endif
        unsigned char  flags;
        unsigned short hdrlen;    // number of 32 bit words
        unsigned short ours;
        unsigned short theirs;
    } *h;

    switch(encaps) {
    case CCNL_ENCAPS_SEQUENCED2012:
	memset(h, 0, sizeof(*h));
	h->hdrlen = 2;
	if (*datalen > (mtu - sizeof(*h))) {
	    if (offs == 0)
		h->flags = SHIM_FRAGMENT_START << 6;
	    else
		h->flags = SHIM_FRAGMENT_MIDDLE << 6;
	    *datalen = mtu - sizeof(*h);
	} else {
	    if (offs == 0)
		h->flags = SHIM_FRAGMENT_SINGLE << 6;
	    else
		h->flags = SHIM_FRAGMENT_END << 6;
	}
	return fraghdr;
    case CCNL_ENCAPS_NONE:
    default:
	break;
    }
#endif
    return 0;
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


/*
struct ccnl_buf_s*
ccnl_encaps_RX(struct ccnl_relay_s *ccnl,
	       struct ccnl_face_s *face,
	       unsigned char *data, int len,
	       struct sockaddr *src, int salen)
{
    return NULL;
}
*/

#ifdef XXX
int
ccnl_encaps_predict_fragcnt(struct ccnl_encaps_s *e,
			    struct ccnl_buf_s *pkt, int *size)
{
    int sz = pkt->datalen, cnt = 1;

/*
    if (e)
	switch(e->protocol) {
	case CCNL_ENCAPS_SEQUENCED2012:
	    cnt = 1;
	    sz = 
	    break;
	case CCNL_ENCAPS_NONE:
	default:
	    break;
    }
*/

    if (size)
	*size = sz;
    return cnt;
}
#endif

/*
struct ccnl_buf_s*
ccnl_encaps_create_fragments(struct ccnl_encaps_s *e, struct ccnl_buf_s *pkt)
{
    if (!e)
	return pkt;
    
}
*/

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

/*
    int cnt = 1;

    if (!e)
	*totallen = origlen;
    else {
	cnt = (origlen + e->mtu - 1) / e->mtu;
	*totallen = origlen + cnt * sizeof(struct sequenced2011hdr);
    }
    return cnt;
*/
}

struct ccnl_buf_s*
ccnl_encaps_getnextfragment(struct ccnl_encaps_s *e, int *ifndx,
			    sockunion *su)
{
    struct ccnl_buf_s *buf = 0;
    unsigned char header[256];
    int hdrlen, blobtaglen, datalen, flagoffs;
/*
    struct sequenced2011hdr *h;
    unsigned int flags;
*/

    DEBUGMSG(2, "ccnl_encaps_getnextfragment e=%p\n", (void*)e);

    if (!e->bigpkt) {
	DEBUGMSG(2, "  no packet to fragment yet\n");
	return NULL;
    }
    DEBUGMSG(2, "  %d bytes to fragment, offset=%d\n", e->bigpkt->datalen, e->sendoffs);

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

    /*
    buf = ccnl_buf_new(NULL, len);
    memcpy(buf->data + sizeof(*h), e->bigpkt->data + e->sendoffs,
	   len - sizeof(*h));
    h = (struct sequenced2011hdr*) buf->data;
    h->magic[0] = 0x10;
    h->magic[1] = 0x80;
    h->our_seq = htonl(e->sendseq);
    if ((len - sizeof(*h)) >= e->bigpkt->datalen) { // single
	h->flags = htons(0x03);
	ccnl_free(e->bigpkt);
	e->bigpkt = NULL;
    } else if (e->sendoffs == 0) // start
	h->flags = htons(0x01);
    else if((len - sizeof(*h)) >= (e->bigpkt->datalen - e->sendoffs)) { // end
	h->flags = htons(0x02);
	ccnl_free(e->bigpkt);
	e->bigpkt = NULL;
    } else // middle
	h->flags = htons(0x00);
    DEBUGMSG(2, "  ]] %p %p %04x seq=%d biglen=%d len=%d offs=%d\n",
	   (void*)e, (void*)e->bigpkt, ntohs(h->flags), ntohl(h->our_seq),
	   e->bigpkt ? e->bigpkt->datalen : -1, len, e->sendoffs);
    */

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

/*
    {
	int i;
	unsigned char *cp;
	for (i = 0, cp = buf->data; i < buf->datalen; i++, cp++) {
	    if (i && !(i % 8)) printf(",\n");
	    if ((i % 8)) printf(", ");
	    printf("0x%02x", *cp);
	}
	if ((i%8))
	    printf("\n");
    }
*/
    return buf;
}

/*
void
ccnl_encaps_start(struct ccnl_encaps_s *e, struct ccnl_buf_s *buf,
		  int ifndx, sockunion *su)
{
    if (e->bigpkt)
      ccnl_free(e->bigpkt);
    e->bigpkt = buf;
    e->sendoffs = 0;
}
*/

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

#ifdef XXX
void
ccnl_relay_encaps_TX_done(struct ccnl_relay_s *ccnl, int ifndx)
{
    DEBUGMSG(10, "ccnl_relay_encaps_TX_done ifndx=%d qlen=%d\n",
	     ifndx, ccnl->ifs[ifndx].qlen);

		    /*
		      struct ccnl_buf_s *b;
		    b = f->fraglist->next;
		    ccnl_free(f->fraglist);
		    f->fraglist = b;
		    */
    if (!ccnl->ifs[ifndx].sendface)
	return;
    if (ccnl_encaps_nomorefragments(ccnl->ifs[ifndx].sendface->encaps)) {
    }
    ccnl->ifs[ifndx].sendface = 0;

    // remove chemflow tokens
}
#endif

#ifdef XXX

void
ccnl_relay_encaps_CTS(struct ccnl_relay_s *ccnl, int ifndx)
{
    DEBUGMSG(10, "ccnl_relay_encaps_CTS ifndx=%d qlen=%d\n",
	     ifndx, ccnl->ifs[ifndx].qlen);
}

int
ccnl_relay_encaps_RTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to, int datalen)
{
    DEBUGMSG(99, "ccnl_relay_encaps_RTS face=%p, len=%d\n",
	     (void *) to, datalen);

/*
    // we should store the requesting face in a list
    if (ccnl->ifs[to->ifndx].sendbuf)
	return 0;
*/
/*
    if (!ccnl->ifs[to->ifndx].sendface)
	ccnl->ifs[to->ifndx].sendface = (struct ccnl_face_s **)
	    ccnl_malloc(CCNL_MAX_IF_QLEN * sizeof(struct ccnl_face_s*));
    if (ccnl->ifs[to->ifndx].sendface &&
			ccnl->ifs[to->ifndx].outcnt < CCNL_MAX_IF_QLEN) {
	ccnl->ifs[to->ifndx].sendface[ccnl->ifs[to->ifndx].outcnt] = to;
	ccnl->ifs[to->ifndx].outcnt++;
	ccnl_sched_RTS(ccnl, to->ifndx, 1);
    }
*/
    return 0;
}


void
ccnl_relay_encaps_TX(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to,
		    struct ccnl_buf_s *buf) // , sockunion *peer)
{
    sockunion *peer = &to->peer;
    //    struct ccnl_if_s *iface = ccnl->ifs + to->ifndx;

    DEBUGMSG(10, "ccnl_relay_encaps_TX face=%p buf=%p %p\n",
	     (void *) to, (void *) buf, (void *) peer);

    /*
    switch(iface->encaps) {
    case CCNL_DGRAM_ENCAPS_NONE:
	ccnl->ifs[to->ifndx].fraglist = 0;
	break;
    case CCNL_DGRAM_ENCAPS_ETH2011:
	// not implemented yet
	// populate the iface->fraglist if necessary
    case CCNL_DGRAM_ENCAPS_UNKNOWN:
    default:
	ccnl_free(buf);
	return;
    }
    */
    // send out first fragment
    ccnl_ll_TX(ccnl, to->ifndx, peer, buf);
}

void
ccnl_relay_encaps_CTS(struct ccnl_relay_s *ccnl, int ifndx)
{
    DEBUGMSG(10, "ccnl_relay_encaps_CTS ifndx=%d qlen=%d\n",
	     ifndx, ccnl->ifs[ifndx].qlen);

    /*
    if (ccnl->ifs[ifndx].fraglist) { // more fragments?
	struct ccnl_buf_s *b = ccnl->ifs[ifndx].fraglist;
	ccnl->ifs[ifndx].fraglist = ccnl->ifs[ifndx].fraglist->next;
	ccnl_ll_TX(ccnl, ifndx, NULL, b);
	return;
    }

    // this is where the face-to-interface scheduler would do its work ...

    if (ccnl->ifs[ifndx].outcnt) {
	struct ccnl_face_s *f = *ccnl->ifs[ifndx].sendface;
	ccnl->ifs[ifndx].outcnt--;
	memmove(ccnl->ifs[ifndx].sendface, ccnl->ifs[ifndx].sendface+1,
		ccnl->ifs[ifndx].outcnt * sizeof(struct ccnl_face_s*));
	ccnl_relay_CTS(ccnl, f);
    }
    */
}

/*
void
ccnl_relay_encaps_RX(struct ccnl_relay_s *ccnl, struct ccnl_face_s *face,
		     unsigned char *data, int len,
		     struct sockaddr *src, int salen)
{
    struct ccnl_buf_s *buf;

    DEBUGMSG(10, "ccnl_relay_encaps_RX if=%d len=%d\n", ifndx, len);

    ccnl_sched_RX_ok(ccnl, ifndx, 1);

    buf = ccnl_buf_new(data, len);
    ccnl_relay_RX(ccnl, ifndx, src, salen,  buf);
}
*/

#endif

#endif // USE_ENCAPS

// eof
