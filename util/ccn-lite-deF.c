/*
 * @f util/ccn-lite-deF.c
 * @b CLI deFragment: merge a fragment series into a sequence of ccnb objs
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
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h> // htonl()
#include <linux/types.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../ccnx.h"
#include "../ccnl.h"
#include "ccnl-common.c"
#include "ccnl-frag.h"

static char *fileprefix = "defrag";
static char noclobber = 0;
static int cnt;

// ----------------------------------------------------------------------

static int consume(int typ, int num, unsigned char **buf, int *len,
		   unsigned char **valptr, int *vallen);

static int
hunt_for_end(unsigned char **buf, int *len,
	     unsigned char **valptr, int *vallen)
{
    int typ, num;

    while (dehead(buf, len, &num, &typ) == 0) {
	if (num==0 && typ==0)					return 0;
	if (consume(typ, num, buf, len, valptr, vallen) < 0)	return -1;
    }
    return -1;
}

static int
consume(int typ, int num, unsigned char **buf, int *len,
	unsigned char **valptr, int *vallen)
{
    if (typ == CCN_TT_BLOB || typ == CCN_TT_UDATA) {
	if (valptr)  *valptr = *buf;
	if (vallen)  *vallen = num;
	*buf += num, *len -= num;
	return 0;
    }
    if (typ == CCN_TT_DTAG || typ == CCN_TT_DATTR)
	return hunt_for_end(buf, len, valptr, vallen);
//  case CCN_TT_TAG, CCN_TT_ATTR:
//  case DTAG, DATTR:
    return -1;
}

int
data2uint(unsigned char *cp, int len)
{
    int i, val;

    for (i = 0, val = 0; i < len; i++)
	if (isdigit(cp[i]))
	    val = 10*val + cp[i] - '0';
	else
	    return -1;
    return val;
}

// ----------------------------------------------------------------------

void
reassembly_done(unsigned char *data, int len)
{
    char fname[512];
    int f;

    sprintf(fname, "%s%03d.ccnb", fileprefix, cnt);
    if (noclobber && !access(fname, F_OK)) {
	printf("file %s already exists, exiting\n", fname);
	exit(-1);
    }

    printf("new file %s, %d bytes\n", fname, len);

    f = creat(fname, 0666);
    if (f < 0)
	perror("open");
    else {
	if (write(f, data, len) < 0)
	    perror("write");
	close(f);
    }
}

void
RX_hbh_frag(struct ccnl_frag_s *fr, int seqnr, int seqnrlen, char flags,
	    unsigned char *content, int contlen)
{
    struct ccnl_buf_s *buf = NULL;

    printf("  received fragment %d, flags=0x%02x, %d bytes\n",
	   seqnr, flags, contlen);

    if (!fr->protocol) {
	fr->protocol = 1;
	fr->recvseq = seqnr;
	fr->recvseqwidth = seqnrlen;
    } else { // verify seqnr
    }
    fr->recvseq = seqnr;
/*
    if (s->ourseqbytes <= sizeof(int) && s->ourseqbytes > 1)
	e->recvseqbytes = s->ourseqbytes;
    // next expected seq number:
    e->recvseq = (s->ourseq + 1) & ((1<<(8*s->ourseqbytes)) - 1);
*/

    switch (flags & CCNL_DTAG_FRAG_FLAG_MASK) {
    case CCNL_DTAG_FRAG_FLAG_SINGLE:
	// DEBUGMSG(17, "  >> single fragment\n");
	if (fr->defrag) {
	    fprintf(stderr, "  had to drop defrag buf\n");
	    ccnl_free(fr->defrag);
	    fr->defrag = NULL;
	}
	// no need to copy the buffer:
	reassembly_done(content, contlen);
	return;
    case CCNL_DTAG_FRAG_FLAG_FIRST: // start of fragment sequence
	// DEBUGMSG(17, "  >> start of fragment series\n");
	if (fr->defrag) {
	    fprintf(stderr, "  had to drop defrag buf\n");
	    ccnl_free(fr->defrag);
	    fr->defrag = NULL;
	}
	fr->defrag = ccnl_buf_new(content, contlen);
	break;
    case CCNL_DTAG_FRAG_FLAG_LAST: // end of fragment sequence
	// DEBUGMSG(17, "  >> last fragment of a series\n");
	if (!fr->defrag)
	    break;
	buf = ccnl_buf_new(NULL, fr->defrag->datalen + contlen);
	if (buf) {
	    memcpy(buf->data, fr->defrag->data, fr->defrag->datalen);
	    memcpy(buf->data + fr->defrag->datalen, content, contlen);
	    reassembly_done(buf->data, buf->datalen);
	    ccnl_free(buf);
	}
	ccnl_free(fr->defrag);
	fr->defrag = NULL;
	break;
    case CCNL_DTAG_FRAG_FLAG_MID:  // fragment in the middle of a squence
    default:
	// DEBUGMSG(17, "  >> fragment in the middle of a series\n");
	if (!fr->defrag)
	    break;
	buf = ccnl_buf_new(NULL, fr->defrag->datalen + contlen);
	if (buf) {
	    memcpy(buf->data, fr->defrag->data, fr->defrag->datalen);
	    memcpy(buf->data + fr->defrag->datalen, content, contlen);
	    ccnl_free(fr->defrag);
	    fr->defrag = buf;
	    buf = NULL;
	} else {
	    ccnl_free(fr->defrag);
	    fr->defrag = NULL;
	}
	break;
    }
}

void
parseFrag(char *fname, unsigned char *data, int datalen, struct ccnl_frag_s *fr)
{
    int num, typ, dummy;
    unsigned char *type = NULL, *content = NULL, *seq = NULL, *flag = NULL;
    int typelen, contlen, seqlen;
    int seqnr;

    if (dehead(&data, &datalen, &num, &typ)
			|| typ != CCN_TT_DTAG || num != CCNL_DTAG_FRAGMENT) {
	fprintf(stderr, "** file %s not a fragment, ignored\n", fname);
	return;
    }

    while (dehead(&data, &datalen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	if (typ != CCN_TT_DTAG) {
parseError:
	    fprintf(stderr, "error parsing file %s, ignored\n", fname);
	    return;
	}
	switch (num) {
	case CCNL_DTAG_FRAG_TYPE:
	    if (consume(typ, num, &data, &datalen, &type, &typelen) < 0)
		goto parseError;
	    break;
	case CCNL_DTAG_FRAG_FLAGS:
	    if (consume(typ, num, &data, &datalen, &flag, &dummy) < 0)
		goto parseError;
	    break;
	case CCNL_DTAG_FRAG_SEQNR:
	    if (consume(typ, num, &data, &datalen, &seq, &seqlen) < 0)
		goto parseError;
	    break;
	case CCN_DTAG_CONTENT:
	    if (consume(typ, num, &data, &datalen, &content, &contlen) < 0)
		goto parseError;
	    break;
	default:
	    goto parseError;
	}
    }
    if (!type || typelen != 3)
	goto parseError;
    if (memcmp(type, "\x14\x70\x47", 3)) { // "FHBH"
	fprintf(stderr, "** unknown fragment type, ignored\n");
	return;
    }
    if (!seq || !flag) {
	fprintf(stderr, "** flag or seq number field missing, fragment ignored\n");
	return;
    }

    seqnr = ntohl(*(int*)seq);

    RX_hbh_frag(fr, seqnr, seqlen, *flag, content, contlen);
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int opt;
    char *cmdname = argv[0], *fname;
    struct ccnl_frag_s fr;

    while ((opt = getopt(argc, argv, "f:hn:")) != -1) {
        switch (opt) {
        case 'f':
	    fileprefix = optarg;
	    break;
	case 'n':
	    noclobber = ! noclobber;
	    break;
        case 'h':
	default:
usage:
	    fprintf(stderr, "usage: %s [options] FILE(S)\n"
	    "  -f PREFIX   use PREFIX for output file names (default: defrag)\n"
	    "  -n          no-clobber\n",
	    cmdname);
	    exit(1);
	}
    }

    if (!argv[optind])
	goto usage;

    memset(&fr, 0, sizeof(fr));
    fname = argv[optind++];
    do {
	unsigned char in[64*1024];
	int len, fd = open(fname, O_RDONLY);

	printf("file %s:\n", fname);

	if (fd < 0) {
	    fprintf(stderr, "error opening file %s\n", fname);
	    exit(-1);
	}
	len = read(fd, in, sizeof(in));
	if (len < 0) {
	    fprintf(stderr, "error reading file %s\n", fname);
	    exit(-1);
	}
	close(fd);

	parseFrag(fname, in, len, &fr);

	fname = argv[optind] ? argv[optind++] : NULL;
    } while (fname);

    return 0;
}

// eof
