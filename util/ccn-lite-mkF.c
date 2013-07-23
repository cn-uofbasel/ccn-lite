/*
 * @f util/ccn-lite-mkF.c
 * @b CLI mkFragment: split a large (ccnb) file into a fragment series
 *
 * Copyright (C) 2013, Christian Tschudin, University of Basel
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
 * 2013-07-06  created
 */

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h> // sockaddr

#include "../ccnx.h"
#include "../ccnl.h"

#include "ccnl-common.c"
#include "../ccnl-pdu.c"
#include "ccnl-frag.h"

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------

struct ccnl_buf_s*
ccnl_frag_getnext(struct ccnl_frag_s *fr)
{
    struct ccnl_buf_s *buf = 0;
    unsigned char header[256];
    int hdrlen, blobtaglen, datalen, flagoffs;

    if (!fr->bigpkt) return NULL;

    printf("fragmenting %d bytes (@ %d)\n", fr->bigpkt->datalen, fr->sendoffs);

    // switch among encodings of fragments here (ccnb, TLV, etc)

    hdrlen = mkHeader(header, CCNL_DTAG_FRAGMENT, CCN_TT_DTAG);   // fragment

    hdrlen += mkHeader(header + hdrlen, CCNL_DTAG_FRAG_TYPE, CCN_TT_DTAG);
    hdrlen += mkHeader(header + hdrlen, 3, CCN_TT_BLOB);
    memcpy(header + hdrlen, "\x14\x70\x47", 3); // "FHBH"
    header[hdrlen + 3] = '\0';
    hdrlen += 4;

    hdrlen += mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG_SEQNR, CCN_TT_DTAG,
			  fr->sendseq, fr->sendseqwidth);

    hdrlen += mkBinaryInt(header + hdrlen, CCNL_DTAG_FRAG_FLAGS, CCN_TT_DTAG,
			  0, fr->flagswidth);
    flagoffs = hdrlen - 2; // most significant byte of flag element

    // other optional fields would go here

    hdrlen += mkHeader(header+hdrlen, CCN_DTAG_CONTENT, CCN_TT_DTAG);

    blobtaglen = mkHeader(header + hdrlen, fr->mtu - hdrlen - 2, CCN_TT_BLOB);
    datalen = fr->mtu - hdrlen - blobtaglen - 2;
    if (datalen > (fr->bigpkt->datalen - fr->sendoffs))
	datalen = fr->bigpkt->datalen - fr->sendoffs;
    hdrlen += mkHeader(header + hdrlen, datalen, CCN_TT_BLOB);

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

    return buf;
}

void 
file2frags(unsigned char *data, int datalen, char *fileprefix, int bytelimit,
	   unsigned int *seqnr, unsigned int seqnrwidth, char noclobber)
{
    struct ccnl_buf_s *fragbuf;
    struct ccnl_frag_s fr;
    char fname[512];
    int cnt = 0, f;

    memset(&fr, 0, sizeof(fr));
    fr.bigpkt = ccnl_buf_new(data, datalen);
    fr.mtu = bytelimit;
    fr.sendseq = *seqnr;
    fr.sendseqwidth = seqnrwidth;
    fr.flagswidth = 1;

    fragbuf = ccnl_frag_getnext(&fr);
    while (fragbuf) {
	sprintf(fname, "%s%03d.ccnb", fileprefix, cnt);
	if (noclobber && !access(fname, F_OK)) {
	    printf("file %s already exists, skipping this name\n", fname);
	} else {
	    printf("new fragment, len=%d / %d --> %s\n",
		   fragbuf->datalen, fr.sendseq, fname);
	    f = creat(fname, 0666);
	    if (f < 0)
		perror("open");
	    else {
		if (write(f, fragbuf->data, fragbuf->datalen) < 0)
		    perror("write");
		close(f);
	    }
	    ccnl_free(fragbuf);
	    fragbuf = ccnl_frag_getnext(&fr);
	}
	cnt++;
    }
    *seqnr = fr.sendseq;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int opt, len, fd;
    unsigned int bytelimit = 1500, seqnr = 0, seqnrlen = 4;
    char *cmdname = argv[0], *cp, *fname, *fileprefix = "frag";
    char noclobber = 0;

    while ((opt = getopt(argc, argv, "b:f:hns:")) != -1) {
        switch (opt) {
        case 'b':
	    bytelimit = atoi((char*) optarg);
	    break;
        case 'f':
	    fileprefix = optarg;
	    break;
	case 'n':
	    noclobber = ! noclobber;
	    break;
        case 's':
	    seqnr = strtol(optarg, &cp, 0);
	    if (cp && cp[0]== '/' && isdigit(cp[1]))
		seqnrlen = atoi(cp+1);
	    break;
        case 'h':
	default:
	    fprintf(stderr, "usage: %s [options] FILE(S)\n"
	    "  -b LIMIT    MTU limit\n"
	    "  -f PREFIX   use PREFIX for fragment file names (default: frag)\n"
	    "  -n          no-clobber\n"
	    "  -s NUM[/SZ] start with seqnr NUM, SZ Bytes (default: 0/4)\n",
	    cmdname);
	    exit(1);
	}
    }

    fname = argv[optind] ? argv[optind++] : "-";
    do {
	unsigned char in[64*1024];
	fd = strcmp(fname, "-") ? open(fname, O_RDONLY) : 0;
	if (fd < 0) {
	    fprintf(stderr, "error opening file %s\n", fname);
	    exit(-1);
	}
	len = read(fd, in, sizeof(in));
	if (len < 0) {
	    fprintf(stderr, "error reading file %s\n", fname);
	    exit(-1);
	}
	if (len == sizeof(in)) {
	    char tmp;
	    len = read(fd, &tmp, 1);
	    if (len > 0) {
		fprintf(stderr, "error: input file %s larger than %d KB\n",
			fname, (int)(sizeof(in)/1024));
		exit(-1);
	    }
	}
	close(fd);

	file2frags(in, len, fileprefix, bytelimit, &seqnr, seqnrlen, noclobber);
	fname = argv[optind] ? argv[optind++] : NULL;
    } while (fname);

    return 0;
}

// eof
