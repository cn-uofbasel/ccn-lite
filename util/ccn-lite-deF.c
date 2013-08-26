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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define USE_FRAG

#include "../ccnx.h"
#include "../ccnl.h"
#include "../ccnl-core.h"

#include "ccnl-common.c"
#include "../ccnl-pdu.c"
// #include "ccnl-frag.h"

// ----------------------------------------------------------------------

#ifdef SILENT
#  define DEBUGMSG(LVL, ...) do {} while (0)
#else
#  define DEBUGMSG(LVL, ...) do {       \
        fprintf(stderr, __VA_ARGS__);   \
    } while (0)
#endif

#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)

struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b;

    b = (struct ccnl_buf_s *) malloc(sizeof(*b) + len);
    if (!b)
	return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
	memcpy(b->data, data, len);
    return b;
}

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
ccnl_core_RX_i_or_c(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                    unsigned char **data, int *datalen)
{
    return 0;
}

// ----------------------------------------------------------------------

#include "../ccnl-ext.h"
#include "../ccnl-ext-frag.c"

// ----------------------------------------------------------------------

static char *fileprefix = "defrag";
static char noclobber = 0;
static int cnt;

int
reassembly_done(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		unsigned char **data, int *len)
{
    char fname[512];
    int f;

    sprintf(fname, "%s%03d.ccnb", fileprefix, cnt);
    if (noclobber && !access(fname, F_OK)) {
	printf("file %s already exists, exiting\n", fname);
	exit(-1);
    }

    printf("new file %s, %d bytes\n", fname, *len);

    f = creat(fname, 0666);
    if (f < 0)
	perror("open");
    else {
	if (write(f, *data, *len) < 0)
	    perror("write");
	close(f);
    }

    return 0;
}


void
parseFrag(char *fname, unsigned char *data, int datalen, struct ccnl_face_s *f)
{
    int num, typ;

    if (dehead(&data, &datalen, &num, &typ)
		|| typ != CCN_TT_DTAG || num != CCNL_DTAG_FRAGMENT2013) {
	fprintf(stderr, "** file %s not a CCNx2013 fragment, ignored\n", fname);
	return;
    }

    ccnl_frag_RX_CCNx2013(reassembly_done, NULL, f, &data, &datalen);
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int opt;
    char *cmdname = argv[0], *fname;
    struct ccnl_face_s f;

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


    memset(&f, 0, sizeof(f));
    f.frag = ccnl_frag_new(CCNL_FRAG_CCNx2013, 1200);

    fname = argv[optind++];
    do {
	unsigned char in[64*1024];
	int len, fd = open(fname, O_RDONLY);

	printf("** file %s\n", fname);

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

	parseFrag(fname, in, len, &f);

	fname = argv[optind] ? argv[optind++] : NULL;
    } while (fname);

    return 0;
}

// eof
