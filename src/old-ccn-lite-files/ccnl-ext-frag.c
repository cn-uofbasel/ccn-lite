/*
 * @f ccnl-ext-frag.c
 * @b CCN lite extension: fragmentation support (including scheduling interface)
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
 * 2011-10-05 created
 * 2013-05-02 prototyped a new fragment format CCNL_FRAG_TYPE_CCNx2013
 * 2015-06-06 marked all except BeginEnd2015 frag scheme as obsolete
 */

// ----------------------------------------------------------------------

#ifdef USE_FRAG

/* see ccnl-core.h for available fragmentation protocols.
 *
 * CCNL_FRAG_NONE
 *  passthrough, i.e. no header is added at all
 *
 * CCNL_FRAG_SEQUENCED2012
 *  - a ccnb encoded header is prepended,
 *  - the driver is configurable for arbitrary MTU
 *  - packets have sequence numbers (can detect lost packets)
 *
 * CCNL_FRAG_CCNx2013
 *  - a ccnb encoded wire format as currently discussed with PARC.
 *    It serves as a container for various wire format types,
 *    including carrying fragments of bigger CCNX objects
 *  - all attributes from SEQUENCED2012 are retained
 *
 * CCNL_FRAG_SEQUENCED2015
 *  - CCNTLV and CISTLV define a new fixed header: we use this
 *    for a simple fragmentation protocol, also implemented for IOTTLV
 *
 * CCNL_FRAG_BEGINEND2015 ("Begin-End fragmentation")
 *  - cleanup (simplification) of old "sequenced" protocol
 *
 */

// ----------------------------------------------------------------------

#include <string.h>

#ifdef RIOT_VERSION
#include "ccn-lite-riot.h"
#endif
#include "ccnl-headers.h"
#include "ccnl-ext.h"

#define DEBUGMSG_EFRA(...) DEBUGMSG(__VA_ARGS__)

struct ccnl_frag_s*
ccnl_frag_new(int protocol, int mtu)
{
    struct ccnl_frag_s *e = NULL;

    DEBUGMSG_EFRA(TRACE, "ccnl_frag_new proto=%d mtu=%d\n", protocol, mtu);

    switch(protocol) {
#ifdef OBSOLETE_BY_2015_06
    case CCNL_FRAG_SEQUENCED2012:
    case CCNL_FRAG_CCNx2013:
    case CCNL_FRAG_SEQUENCED2015:
        e = (struct ccnl_frag_s*) ccnl_calloc(1, sizeof(struct ccnl_frag_s));
        if (e) {
            e->protocol = protocol;
            e->mtu = mtu;
            e->flagwidth = 1;
            e->sendseqwidth =   4;
            e->losscountwidth = 2;
            e->recvseqwidth =   4;
        }
        break;
#endif
    case CCNL_FRAG_BEGINEND2015:
        e = (struct ccnl_frag_s*) ccnl_calloc(1, sizeof(struct ccnl_frag_s));
        if (e) {
            e->protocol = protocol;
            e->mtu = mtu;
        }
        break;
    case CCNL_FRAG_NONE:
    default:
        break;
    }
    return e;
}

void
ccnl_frag_reset(struct ccnl_frag_s *e, struct ccnl_buf_s *buf,
                  int ifndx, sockunion *dst)
{
    DEBUGMSG_EFRA(VERBOSE, "ccnl_frag_reset if=%d (%zd bytes) dst=%s\n", ifndx,
             buf ? buf->datalen : -1, ccnl_addr2ascii(dst));
    if (!e)
        return;
    e->ifndx = ifndx;
    memcpy(&e->dest, dst, sizeof(*dst));
    ccnl_free(e->bigpkt);
    e->bigpkt = buf;
    if (buf)
        e->outsuite = ccnl_pkt2suite(buf->data, buf->datalen, 0);
    e->sendoffs = 0;
}

int
ccnl_frag_getfragcount(struct ccnl_frag_s *e, int origlen, int *totallen)
{
    (void) origlen;
    int cnt = 0, len = 0;

    if (!e)
      cnt = 1;
#ifdef OBSOLETE_BY_2015_06
#ifdef USE_SUITE_CCNB
    else if (e->protocol == CCNL_FRAG_SEQUENCED2012) {
      unsigned char dummy[256];
      int hdrlen, blobtaglen, datalen;
      int offs = 0;
      while (offs < origlen) { // we could do better than to simulate this:
        hdrlen = ccnl_ccnb_mkHeader(dummy, CCNL_DTAG_FRAGMENT2012, CCN_TT_DTAG);
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2012_FLAGS, CCN_TT_DTAG,
                              0, e->flagwidth);
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2012_SEQNR, CCN_TT_DTAG,
                              0, e->sendseqwidth);
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2012_OLOSS, CCN_TT_DTAG,
                              0, e->losscountwidth);
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2012_YSEQN, CCN_TT_DTAG,
                              0, e->recvseqwidth);

        hdrlen += ccnl_ccnb_mkHeader(dummy, CCN_DTAG_CONTENT, CCN_TT_DTAG);
        blobtaglen = ccnl_ccnb_mkHeader(dummy, e->mtu - hdrlen - 1, CCN_TT_BLOB);
        datalen = e->mtu - hdrlen - blobtaglen - 1;
        if (datalen > (origlen - offs))
            datalen = origlen - offs;
        hdrlen += ccnl_ccnb_mkHeader(dummy, datalen, CCN_TT_BLOB);
        len += hdrlen + datalen + 1;
        offs += datalen;
        cnt++;
      }
    } else if (e->protocol == CCNL_FRAG_CCNx2013) {
      unsigned char dummy[256];
      int hdrlen, blobtaglen, datalen;
      int offs = 0;
      while (offs < origlen) { // we could do better than to simulate this:
        hdrlen = ccnl_ccnb_mkHeader(dummy, CCNL_DTAG_FRAGMENT2013, CCN_TT_DTAG);
        hdrlen += ccnl_ccnb_mkHeader(dummy, CCNL_DTAG_FRAG2013_TYPE, CCN_TT_DTAG);
        hdrlen += 4; // three BLOB bytes plus end-of-entry
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2013_SEQNR, CCN_TT_DTAG,
                              0, e->sendseqwidth);
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2013_FLAGS, CCN_TT_DTAG,
                              0, e->flagwidth);
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2013_OLOSS, CCN_TT_DTAG,
                              0, e->losscountwidth);
        hdrlen += ccnl_ccnb_mkBinaryInt(dummy, CCNL_DTAG_FRAG2013_YSEQN, CCN_TT_DTAG,
                              0, e->recvseqwidth);

        hdrlen += ccnl_ccnb_mkHeader(dummy, CCN_DTAG_CONTENT, CCN_TT_DTAG);
        blobtaglen = ccnl_ccnb_mkHeader(dummy, e->mtu - hdrlen - 1, CCN_TT_BLOB);
        datalen = e->mtu - hdrlen - blobtaglen - 1;
        if (datalen > (origlen - offs))
            datalen = origlen - offs;
        hdrlen += ccnl_ccnb_mkHeader(dummy, datalen, CCN_TT_BLOB);
        len += hdrlen + datalen + 1;
        offs += datalen;
        cnt++;
      }
    }
#endif
    else if (e->protocol == CCNL_FRAG_SEQUENCED2015) {
        // ...
        DEBUGMSG_EFRA(FATAL, "frag counting not implemented yet for SEQD2015!\n");
    }
#endif // OBSOLETE
    else {
        DEBUGMSG_EFRA(FATAL, "frag protocol %d not supported?\n", e->protocol);
    }

    if (totallen)
        *totallen = len;
    return cnt;
}

#ifdef OBSOLTE_BY_2015_06
#ifdef USE_SUITE_CCNB
struct ccnl_buf_s*
ccnl_frag_getnextSEQD2012(struct ccnl_frag_s *e, int *ifndx, sockunion *su)
{
    struct ccnl_buf_s *buf = 0;
    unsigned char header[256];
    int hdrlen = 0, blobtaglen, flagoffs;
    unsigned int datalen;

    DEBUGMSG_EFRA(TRACE, "ccnl_frag_getnextSEQD2012 e=%p, mtu=%d\n", (void*)e, e->mtu);
    DEBUGMSG_EFRA(TRACE, "  %d bytes to fragment, offset=%d\n",
             e->bigpkt->datalen, e->sendoffs);

    hdrlen = ccnl_ccnb_mkHeader(header, CCNL_DTAG_FRAGMENT2012, CCN_TT_DTAG);
    hdrlen += ccnl_ccnb_mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG2012_FLAGS,
                          CCN_TT_DTAG, 0, e->flagwidth);
    flagoffs = hdrlen - 2;
    hdrlen += ccnl_ccnb_mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG2012_YSEQN,
                          CCN_TT_DTAG, e->recvseq, e->recvseqwidth);
    hdrlen += ccnl_ccnb_mkBinaryInt(header+hdrlen, CCNL_DTAG_FRAG2012_OLOSS, CCN_TT_DTAG,
                          e->losscount, e->losscountwidth);

    hdrlen += ccnl_ccnb_mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG2012_SEQNR,
                          CCN_TT_DTAG, e->sendseq, e->sendseqwidth);
    hdrlen += ccnl_ccnb_mkHeader(header+hdrlen, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    blobtaglen = ccnl_ccnb_mkHeader(header+hdrlen, e->mtu - hdrlen - 2, CCN_TT_BLOB);

    datalen = e->mtu - hdrlen - blobtaglen - 2;
    if (datalen > (e->bigpkt->datalen - e->sendoffs))
        datalen = e->bigpkt->datalen - e->sendoffs;
    hdrlen += ccnl_ccnb_mkHeader(header + hdrlen, datalen, CCN_TT_BLOB);

    buf = ccnl_buf_new(NULL, hdrlen + datalen + 2);
    if (!buf)
        return NULL;
    memcpy(buf->data, header, hdrlen);
    memcpy(buf->data + hdrlen, e->bigpkt->data + e->sendoffs, datalen);
    buf->data[hdrlen + datalen] = '\0'; // end of content/any field
    buf->data[hdrlen + datalen + 1] = '\0'; // end of fragment/pdu

    if (datalen >= e->bigpkt->datalen) { // fits in a single fragment
        buf->data[flagoffs + e->flagwidth - 1] =
            CCNL_DTAG_FRAG_FLAG_FIRST | CCNL_DTAG_FRAG_FLAG_LAST;
        ccnl_free(e->bigpkt);
        e->bigpkt = NULL;
    } else if (e->sendoffs == 0) // this is the start fragment
        buf->data[flagoffs + e->flagwidth - 1] = CCNL_DTAG_FRAG_FLAG_FIRST;
    else if(datalen >= (e->bigpkt->datalen - e->sendoffs)) { // the end
        buf->data[flagoffs + e->flagwidth - 1] = CCNL_DTAG_FRAG_FLAG_LAST;
        ccnl_free(e->bigpkt);
        e->bigpkt = NULL;
    } else // in the middle
        buf->data[flagoffs + e->flagwidth - 1] = 0x00;

    e->sendoffs += datalen;
    e->sendseq++;

    DEBUGMSG_EFRA(TRACE, "  e->offset now %d\n", e->sendoffs);

    if (ifndx)
        *ifndx = e->ifndx;
    if (su)
        memcpy(su, &e->dest, sizeof(*su));
    return buf;
}

struct ccnl_buf_s*
ccnl_frag_getnextCCNx2013(struct ccnl_frag_s *fr, int *ifndx, sockunion *su)
{
    struct ccnl_buf_s *buf = 0;
    unsigned char header[256];
    int hdrlen, blobtaglen, flagoffs;
    unsigned int datalen;

    // switch among encodings of fragments here (ccnb, TLV, etc)

    hdrlen = ccnl_ccnb_mkHeader(header, CCNL_DTAG_FRAGMENT2013, CCN_TT_DTAG); // fragment

    hdrlen += ccnl_ccnb_mkHeader(header + hdrlen, CCNL_DTAG_FRAG2013_TYPE, CCN_TT_DTAG);
    hdrlen += ccnl_ccnb_mkHeader(header + hdrlen, 3, CCN_TT_BLOB);
    memcpy(header + hdrlen, CCNL_FRAG_TYPE_CCNx2013_VAL, 3); // "FHBH"
    header[hdrlen + 3] = '\0';
    hdrlen += 4;

    hdrlen += ccnl_ccnb_mkBinaryInt(header+hdrlen, CCNL_DTAG_FRAG2013_SEQNR, CCN_TT_DTAG,
                          fr->sendseq, fr->sendseqwidth);

    hdrlen += ccnl_ccnb_mkBinaryInt(header+hdrlen, CCNL_DTAG_FRAG2013_FLAGS, CCN_TT_DTAG,
                          0, fr->flagwidth);
    flagoffs = hdrlen - 2; // most significant byte of flag element

    // other optional fields would go here

    hdrlen += ccnl_ccnb_mkHeader(header+hdrlen, CCN_DTAG_CONTENT, CCN_TT_DTAG);

    blobtaglen = ccnl_ccnb_mkHeader(header + hdrlen, fr->mtu - hdrlen - 2, CCN_TT_BLOB);
    datalen = fr->mtu - hdrlen - blobtaglen - 2;
    if (datalen > (fr->bigpkt->datalen - fr->sendoffs))
        datalen = fr->bigpkt->datalen - fr->sendoffs;
    hdrlen += ccnl_ccnb_mkHeader(header + hdrlen, datalen, CCN_TT_BLOB);

    buf = ccnl_buf_new(NULL, hdrlen + datalen + 2);
    if (!buf)
        return NULL;
    memcpy(buf->data, header, hdrlen);
    memcpy(buf->data + hdrlen, fr->bigpkt->data + fr->sendoffs, datalen);
    buf->data[hdrlen + datalen] = '\0'; // end of content field
    buf->data[hdrlen + datalen + 1] = '\0'; // end of fragment

    // patch flag field:
    if (datalen >= fr->bigpkt->datalen) { // single
        buf->data[flagoffs] = CCNL_DTAG_FRAG_FLAG_SINGLE;
        ccnl_free(fr->bigpkt);
        fr->bigpkt = NULL;
    } else if (fr->sendoffs == 0) // start
        buf->data[flagoffs] = CCNL_DTAG_FRAG_FLAG_FIRST;
    else if(datalen >= (fr->bigpkt->datalen - fr->sendoffs)) { // end
        buf->data[flagoffs] = CCNL_DTAG_FRAG_FLAG_LAST;
        ccnl_free(fr->bigpkt);
        fr->bigpkt = NULL;
    } else
        buf->data[flagoffs] = CCNL_DTAG_FRAG_FLAG_MID;

    fr->sendoffs += datalen;
    fr->sendseq++;

    if (ifndx)
        *ifndx = fr->ifndx;
    if (su)
        memcpy(su, &fr->dest, sizeof(*su));

    return buf;
}
#endif // USE_SUITE_CCNB

struct ccnl_buf_s*
ccnl_frag_getnextSEQD2015(struct ccnl_frag_s *fr, int *ifndx, sockunion *su)
{
    struct ccnl_buf_s *buf = 0;
    unsigned int datalen = 0;

    DEBUGMSG_EFRA(DEBUG, "ccnl_frag_getnextSEQD2015: datalen=%d\n", datalen);

    switch(fr->outsuite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        buf = ccnl_ccntlv_mkFrag(fr, &datalen);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        buf = ccnl_iottlv_mkFrag(fr, &datalen);
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        buf = ccnl_ndntlv_mkFrag(fr, &datalen);
        break;
#endif
    default:
        break;
    }

    if (buf) {
        fr->sendseq++;

        fr->sendoffs += datalen;
        if (fr->sendoffs >= fr->bigpkt->datalen) {
            ccnl_free(fr->bigpkt);
            fr->bigpkt = NULL;
        }

        if (ifndx)
            *ifndx = fr->ifndx;
        if (su)
            memcpy(su, &fr->dest, sizeof(*su));

        DEBUGMSG_EFRA(VERBOSE, "  produced %d bytes fragment, seqnr=%d-1\n",
                 buf->datalen, fr->sendseq);
    } else
        DEBUGMSG_EFRA(VERBOSE, "  produced NO fragment, seqnr remains at =%d-1\n",
                 fr->sendseq);

    return buf;
}
#endif // OBSOLETE

struct ccnl_buf_s*
ccnl_frag_getnextBE2015(struct ccnl_frag_s *fr, int *ifndx, sockunion *su)
{
    struct ccnl_buf_s *buf = 0;
    unsigned int datalen = 0;

    DEBUGMSG_EFRA(VERBOSE, "ccnl_frag_getnextBE2015: remaining=%zd\n",
                  fr->bigpkt->datalen - fr->sendoffs);

    switch(fr->outsuite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        buf = ccnl_ccntlv_mkFrag(fr, &datalen);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        buf = ccnl_iottlv_mkFrag(fr, &datalen);
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        buf = ccnl_ndntlv_mkFrag(fr, &datalen);
        break;
#endif
    default:
        break;
    }

    if (buf) {
        fr->sendseq++;

        fr->sendoffs += datalen;
        if (fr->sendoffs >= (unsigned) fr->bigpkt->datalen) {
            ccnl_free(fr->bigpkt);
            fr->bigpkt = NULL;
        }

        if (ifndx)
            *ifndx = fr->ifndx;
        if (su)
            memcpy(su, &fr->dest, sizeof(*su));

        DEBUGMSG_EFRA(VERBOSE, "  produced %zd bytes fragment, seqnr=%d-1\n",
                 buf->datalen, fr->sendseq);
    } else
        DEBUGMSG_EFRA(VERBOSE, "  produced NO fragment, seqnr remains at =%d-1\n",
                 fr->sendseq);

    return buf;
}

struct ccnl_buf_s*
ccnl_frag_getnext(struct ccnl_frag_s *fr, int *ifndx, sockunion *su)
{
    if (!fr->bigpkt) return NULL;

    DEBUGMSG_EFRA(VERBOSE, "fragmenting %zd bytes (@ %d)\n",
                                fr->bigpkt->datalen, fr->sendoffs);

    switch (fr->protocol) {
#ifdef OBSOLETE_BY_2015_06
#ifdef USE_SUITE_CCNB
    case CCNL_FRAG_SEQUENCED2012:
        return ccnl_frag_getnextSEQD2012(fr, ifndx, su);
    case CCNL_FRAG_CCNx2013:
        return ccnl_frag_getnextCCNx2013(fr, ifndx, su);
#endif
    case CCNL_FRAG_SEQUENCED2015:
        return ccnl_frag_getnextSEQD2015(fr, ifndx, su);
#endif
    case CCNL_FRAG_BEGINEND2015:
        return ccnl_frag_getnextBE2015(fr, ifndx, su);
    default:
        DEBUGMSG_EFRA(VERBOSE, "  unknown protocol %d\n", fr->protocol);
        return NULL;
    }
}

int
ccnl_frag_nomorefragments(struct ccnl_frag_s *e)
{
    if (!e || !e->bigpkt)
        return 1;
    return (unsigned) e->bigpkt->datalen <= e->sendoffs;
}

void
ccnl_frag_destroy(struct ccnl_frag_s *e)
{
    if (e) {
        ccnl_free(e->bigpkt);
        ccnl_free(e->defrag);
        ccnl_free(e);
    }
}

// ----------------------------------------------------------------------

struct serialFragPDU_s { // collect all fields of a numbered HBH fragment
    int contlen;
    unsigned char *content;
    unsigned int flags, ourseq, ourloss, yourseq, HAS;
    unsigned char flagwidth, ourseqwidth, ourlosswidth, yourseqwidth;
};

void
serialFragPDU_init(struct serialFragPDU_s *s)
{
    memset(s, 0, sizeof(*s));
    s->contlen = -1;
    s->flagwidth = 1;
    s->ourseqwidth = s->ourlosswidth = s->yourseqwidth = sizeof(int);
}

#ifdef OBSOLETE_BY_2015_06
// processes a SEQENCED fragment. Once fully assembled we call the callback
void
ccnl_frag_RX_serialfragment(RX_datagram callback,
                            struct ccnl_relay_s *relay,
                            struct ccnl_face_s *from,
                            struct serialFragPDU_s *s)
{
    struct ccnl_buf_s *buf = NULL;
    struct ccnl_frag_s *e = from->frag;
    DEBUGMSG_EFRA(VERBOSE, "  frag %p protocol=%d, flags=%04x, seq=%d (%d)\n",
             (void*)e, e->protocol, s->flags, s->ourseq, e->recvseq);

    if (e->recvseq != s->ourseq) {
        // should increase error counter here
        if (e->defrag) {
            DEBUGMSG_EFRA(WARNING, "  >> seqnum mismatch (%d/%d), dropped defrag buf\n",
                     s->ourseq, e->recvseq);
            ccnl_free(e->defrag);
            e->defrag = NULL;
        }
        if (e->recvseq > s->ourseq) // old and dup fragment: ignore
            return;
    }
    switch(s->flags & (CCNL_DTAG_FRAG_FLAG_FIRST|CCNL_DTAG_FRAG_FLAG_LAST)) {
    case CCNL_DTAG_FRAG_FLAG_SINGLE: // single packet
        DEBUGMSG_EFRA(VERBOSE, "  >> single fragment (%d bytes)\n", s->contlen);
        if (e->defrag) {
            DEBUGMSG_EFRA(WARNING, "    had to drop defrag buf\n");
            ccnl_free(e->defrag);
            e->defrag = NULL;
        }
        // no need to copy the buffer:
        callback(relay, from, &s->content, &s->contlen);
        return;
    case CCNL_DTAG_FRAG_FLAG_FIRST: // start of fragment sequence
        DEBUGMSG_EFRA(VERBOSE, "  >> start of fragment series\n");
        if (e->defrag) {
            DEBUGMSG_EFRA(WARNING, "    had to drop defrag buf\n");
            ccnl_free(e->defrag);
        }
        e->defrag = ccnl_buf_new(s->content, s->contlen);
        s->content += s->contlen;
        s->contlen = 0;
        break;
    case CCNL_DTAG_FRAG_FLAG_LAST: // end of fragment sequence
        DEBUGMSG_EFRA(VERBOSE, "  >> last fragment of a series\n");
        if (!e->defrag) {
            DEBUGMSG_EFRA(VERBOSE, "  >> no e->defrag?\n");
            break;
        }
        buf = ccnl_buf_new(NULL, e->defrag->datalen + s->contlen);
        if (buf) {
            memcpy(buf->data, e->defrag->data, e->defrag->datalen);
            memcpy(buf->data + e->defrag->datalen, s->content, s->contlen);
        }
        ccnl_free(e->defrag);
        e->defrag = NULL;
        s->content += s->contlen;
        s->contlen = 0;
        break;
    case CCNL_DTAG_FRAG_FLAG_MID:  // fragment in the middle of a squence
    default:
        DEBUGMSG_EFRA(VERBOSE, "  >> fragment in the middle of a series\n");
        if (!e->defrag) break;
        buf = ccnl_buf_new(NULL, e->defrag->datalen + s->contlen);
        if (buf) {
            memcpy(buf->data, e->defrag->data, e->defrag->datalen);
            memcpy(buf->data + e->defrag->datalen, s->content, s->contlen);
            ccnl_free(e->defrag);
            e->defrag = buf;
            buf = NULL;
        } else {
            ccnl_free(e->defrag);
            e->defrag = NULL;
        }
        s->content += s->contlen;
        s->contlen = 0;
        break;
    }
    // FIXME: we should only bump recvseq if s->ourseq is ahead, or 0
    e->recvseq = s->ourseq + 1;
    DEBUGMSG_EFRA(VERBOSE, ">>> seq from %d to %d (w=%d)\n", s->ourseq, e->recvseq,
        s->ourseqwidth);

    if (buf) {
        unsigned char *frag = buf->data;
        int fraglen = buf->datalen;
        DEBUGMSG_EFRA(VERBOSE, "  >> reassembled fragment is %d bytes\n", buf->datalen);
        // FIXME: loop over multiple packets in this reassembled frame?
        callback(relay, from, &frag, &fraglen);
        ccnl_free(buf);
    }
}

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNB

#define getNumField(var,len,flag,rem) \
        DEBUGMSG_EFRA(TRACE, "  parsing " rem "\n"); \
        if (ccnl_ccnb_unmkBinaryInt(data, datalen, &var, &len) != 0) \
            goto Bail; \
        s.HAS |= flag
#define HAS_FLAGS  0x01
#define HAS_OSEQ   0x02
#define HAS_OLOS   0x04
#define HAS_YSEQ   0x08

int
ccnl_frag_RX_frag2012(RX_datagram callback,
                        struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                        unsigned char **data, int *datalen)
{
    int num, typ;
    struct serialFragPDU_s s;
    DEBUGMSG_EFRA(TRACE, "ccnl_frag_RX_frag2012 (%d bytes)\n", *datalen);

    serialFragPDU_init(&s);
    while (ccnl_ccnb_dehead(data, datalen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        if (typ == CCN_TT_DTAG) {
            switch(num) {
            case CCN_DTAG_CONTENT:
                DEBUGMSG_EFRA(TRACE, "  frag content\n");
//              if (s.content) // error: more than one content entry
                if (ccnl_ccnb_consume(typ, num, data, datalen, &s.content,&s.contlen) < 0)
                    goto Bail;
                continue;
            case CCNL_DTAG_FRAG2012_FLAGS:
                getNumField(s.flags, s.flagwidth, HAS_FLAGS, "flags");
                continue;
            case CCNL_DTAG_FRAG2012_SEQNR:
                getNumField(s.ourseq, s.ourseqwidth, HAS_OSEQ, "ourseq");
                continue;
            case CCNL_DTAG_FRAG2012_OLOSS:
                getNumField(s.ourloss, s.ourlosswidth, HAS_OLOS, "ourloss");
                continue;
            case CCNL_DTAG_FRAG2012_YSEQN:
                getNumField(s.yourseq, s.yourseqwidth, HAS_YSEQ, "yourseq");
                continue;
            default:
                break;
            }
        }
        if (ccnl_ccnb_consume(typ, num, data, datalen, 0, 0) < 0)
            goto Bail;
    }
    if (!s.content || s.HAS != 15) {
        DEBUGMSG_EFRA(WARNING, "* incomplete frag\n");
        return 0;
    }

    if (from) {
      if (!from->frag)
          from->frag = ccnl_frag_new(CCNL_FRAG_SEQUENCED2012,
                                       relay->ifs[from->ifndx].mtu);
      if (from->frag && from->frag->protocol == CCNL_FRAG_SEQUENCED2012)
          ccnl_frag_RX_serialfragment(callback, relay, from, &s);
      else
          DEBUGMSG_EFRA(WARNING, "WRONG FRAG PROTOCOL\n");
    } else
        ccnl_frag_RX_serialfragment(callback, relay, from, &s);

    return 0;
Bail:
    DEBUGMSG_EFRA(WARNING, "* frag bailing\n");
    return -1;
}

int
ccnl_frag_RX_CCNx2013(RX_datagram callback,
                       struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                       unsigned char **data, int *datalen)
{
    int rc, num, typ, pdutypelen;
    unsigned char *pdutype = 0;
    struct serialFragPDU_s s;

    DEBUGMSG_EFRA(TRACE, "ccnl_frag_RX_CCNx2013 (%d bytes)\n", *datalen);
    serialFragPDU_init(&s);
    while (ccnl_ccnb_dehead(data, datalen, &num, &typ) == 0) {
        if (num==0 && typ==0) break; // end
        if (typ == CCN_TT_DTAG) {
            switch (num) {
            case CCNL_DTAG_FRAG2013_TYPE:
                if (ccnl_ccnb_hunt_for_end(data, datalen, &pdutype, &pdutypelen) ||
                    pdutypelen != 3) goto Bail;
                continue;
            case CCNL_DTAG_FRAG2013_SEQNR:
                getNumField(s.ourseq, s.ourseqwidth, HAS_OSEQ, "ourseq");
                continue;
            case CCNL_DTAG_FRAG2013_FLAGS:
                getNumField(s.flags, s.flagwidth, HAS_FLAGS, "flags");
                continue;
            case CCN_DTAG_CONTENT:
//              if (frag) // error: more than one content entry
                if (ccnl_ccnb_consume(typ, num, data, datalen, &s.content,&s.contlen) < 0)
                    goto Bail;
                continue;

            // CCNL extensions:
            case CCN_DTAG_INTEREST:
            case CCN_DTAG_CONTENTOBJ:
//                rc = ccnl_ccnb_forwarder(relay, from, data, datalen);
                rc = callback(relay, from, data, datalen);
                if (rc < 0)
                    return rc;
                continue;
            case CCNL_DTAG_FRAG2013_OLOSS:
                getNumField(s.ourloss, s.ourlosswidth, HAS_OLOS, "ourloss");
                continue;
            case CCNL_DTAG_FRAG2013_YSEQN:
                getNumField(s.yourseq, s.yourseqwidth, HAS_YSEQ, "yourseq");
                continue;
            default:
                break;
            }
        }
        if (ccnl_ccnb_consume(typ, num, data, datalen, 0, 0) < 0)
            goto Bail;
    }
    if (!pdutype || !s.content ) {
/* ||
                    (s.HAS&(HAS_FLAGS|HAS_OSEQ)) != (HAS_FLAGS|HAS_OSEQ) ) {
*/
        DEBUGMSG_EFRA(WARNING, "* incomplete frag\n");
        return 0;
    }

    if (memcmp(pdutype, CCNL_FRAG_TYPE_CCNx2013_VAL, 3) == 0) { // hop-by-hop
        if (from) {
            if (!from->frag)
                from->frag = ccnl_frag_new(CCNL_FRAG_CCNx2013,
                                           relay->ifs[from->ifndx].mtu);
            if (from->frag && from->frag->protocol == CCNL_FRAG_CCNx2013)
                ccnl_frag_RX_serialfragment(callback, relay, from, &s);
            else
                DEBUGMSG_EFRA(WARNING, "WRONG FRAG PROTOCOL\n");
        } else
            ccnl_frag_RX_serialfragment(callback, relay, from, &s);
    }

    /*
     * echo "FMTE" | base64 -d | hexdump -v -e '/1 "@x%02x"'| tr @ '\\'; echo
     */
    if (memcmp(pdutype, "\x14\xc4\xc4", 3) == 0) { // mid-to-end fragment
        // not implemented yet
    }

    return 0;
Bail:
    DEBUGMSG_EFRA(WARNING, "* frag bailing\n");
    return -1;
}
#endif // USE_SUITE_CCNB

int
ccnl_frag_RX_Sequenced2015(RX_datagram callback, struct ccnl_relay_s *relay,
                           struct ccnl_face_s *from, int mtu,
                           unsigned int bits, unsigned int seqno,
                           unsigned char **data, int *datalen)
{
    struct serialFragPDU_s s;

    DEBUGMSG_EFRA(DEBUG, "ccnl_frag_RX_Sequenced2015 (%d bytes)\n", *datalen);
    serialFragPDU_init(&s);

    s.ourseq = seqno;
    s.flags = bits;
    s.content = *data;
    s.contlen = *datalen;

    if (from) {
        if (!from->frag)
            from->frag = ccnl_frag_new(CCNL_FRAG_SEQUENCED2015, mtu);
        if (from->frag && from->frag->protocol != CCNL_FRAG_SEQUENCED2015) {
            DEBUGMSG_EFRA(WARNING, "WRONG FRAG PROTOCOL\n");
            return 0;
        }
    }
    ccnl_frag_RX_serialfragment(callback, relay, from, &s);

    /*
    *data = s.content;
    *datalen = s.contlen;
    */

    return 1;
}
#endif // OBSOLETE

int
ccnl_frag_RX_BeginEnd2015(RX_datagram callback, struct ccnl_relay_s *relay,
                          struct ccnl_face_s *from, int mtu,
                          unsigned int bits, unsigned int seqno,
                          unsigned char **data, int *datalen)
{
    struct ccnl_buf_s *buf = NULL;
    struct ccnl_frag_s *e;

    DEBUGMSG_EFRA(DEBUG, "ccnl_frag_RX_BeginEnd2015 (%d bytes), seqno=%d\n",
                  *datalen, seqno);

    if (!from) {
        DEBUGMSG_EFRA(WARNING, "RX_BeginEnd2015: no from value\n");
        return 0;
    }
    if (!from->frag)
        from->frag = ccnl_frag_new(CCNL_FRAG_BEGINEND2015, mtu);
    if (!from->frag) {
        DEBUGMSG_EFRA(WARNING, "no frag alloc\n");
        return 0;
    }
    if (from->frag->protocol != CCNL_FRAG_BEGINEND2015) {
        DEBUGMSG_EFRA(WARNING, "WRONG FRAG PROTOCOL\n");
        return 0;
    }

    e = from->frag;
    if (e->recvseq != seqno) {
        // should increase error counter here
        DEBUGMSG_EFRA(WARNING, "  >> seqnum mismatch: rcvd %d, expected %d\n",
                      seqno, e->recvseq);
        if (e->defrag) {
            DEBUGMSG_EFRA(WARNING, "  >> had to drop defrag buf\n");
            ccnl_free(e->defrag);
            e->defrag = NULL;
        }
        e->recvseq = seqno;
    }

    switch(bits & (CCNL_BEFRAG_FLAG_MASK)) {
    case CCNL_BEFRAG_FLAG_SINGLE: // single packet
        DEBUGMSG_EFRA(VERBOSE, "  >> single fragment seqno=%d (%d bytes)\n",
                      seqno, *datalen);
        if (e->defrag) {
            DEBUGMSG_EFRA(WARNING, "    had to drop defrag buf\n");
            ccnl_free(e->defrag);
            e->defrag = NULL;
        }
        e->recvseq++;
        // no need to copy the buffer:
        callback(relay, from, data, datalen);
        return 1;
    case CCNL_BEFRAG_FLAG_FIRST: // start of fragment sequence
        DEBUGMSG_EFRA(VERBOSE, "  >> start of fragment series\n");
        if (e->defrag) {
            DEBUGMSG_EFRA(WARNING, "    had to drop defrag buf\n");
            ccnl_free(e->defrag);
        }
        e->defrag = ccnl_buf_new(*data, *datalen);
        break;
    case CCNL_BEFRAG_FLAG_LAST: // end of fragment sequence
        DEBUGMSG_EFRA(VERBOSE, "  >> last fragment of a series\n");
        if (!e->defrag) {
            DEBUGMSG_EFRA(WARNING, "  >> no e->defrag?\n");
            break;
        }
        buf = ccnl_buf_new(NULL, e->defrag->datalen + *datalen);
        if (buf) {
            memcpy(buf->data, e->defrag->data, e->defrag->datalen);
            memcpy(buf->data + e->defrag->datalen, *data, *datalen);
        }
        ccnl_free(e->defrag);
        e->defrag = NULL;
        break;
    case CCNL_BEFRAG_FLAG_MID:  // fragment in the middle of a squence
    default:
        DEBUGMSG_EFRA(VERBOSE, "  >> fragment in the middle of a series\n");
        if (!e->defrag)
            break;
        buf = ccnl_buf_new(NULL, e->defrag->datalen + *datalen);
        if (buf) {
            memcpy(buf->data, e->defrag->data, e->defrag->datalen);
            memcpy(buf->data + e->defrag->datalen, *data, *datalen);
            ccnl_free(e->defrag);
            e->defrag = buf;
            buf = NULL;
        } else {
            ccnl_free(e->defrag);
            e->defrag = NULL;
        }
        break;
    }
    *data += *datalen;
    *datalen = 0;
    e->recvseq++;

    if (buf) {
        unsigned char *frag = buf->data;
        int fraglen = buf->datalen;
        DEBUGMSG_EFRA(DEBUG, "  >> reassembled fragment is %zd bytes\n",
                      buf->datalen);
        // FIXME: loop over multiple packets in this reassembled frame?
        callback(relay, from, &frag, &fraglen);
        ccnl_free(buf);
    }

    return 1;
}

#endif // USE_FRAG

// eof
