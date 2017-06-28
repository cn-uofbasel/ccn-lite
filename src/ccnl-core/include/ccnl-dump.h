/*
 * @f ccnl-dump.h
 * @b CCN lite extension
 *
 * Copyright (C) 2012-17, University of Basel
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
 * 2012-05-06 created
 * 2013-10-21 extended for crypto <christopher.scherb@unibas.ch>
 * 2017-26-06 adapted to ccnl v2
 */

#ifndef CCNL_DUMP_H
#define CCNL_DUMP_H

//#ifdef USE_DEBUG

enum { // numbers for each data type
    CCNL_BUF = 1,
    CCNL_PREFIX,
    CCNL_RELAY,
    CCNL_FACE,
    CCNL_FRAG,
    CCNL_FWD,
    CCNL_INTEREST,
    CCNL_PENDINT,
    CCNL_PACKET,
    CCNL_CONTENT
};

//char* ccnl_addr2ascii(sockunion *su);

char *frag_protocol(int e);
void ccnl_dump(int lev, int typ, void *p);
int get_buf_dump(int lev, void *p, long *outbuf, int *len, long *next);
int get_prefix_dump(int lev, void *p, int *len, char **val);
int get_num_faces(void *p);
int get_faces_dump(int lev, void *p, int *faceid, long *next, long *prev, int *ifndx, int *flags, char **peer, int *type, char **frag);
int get_num_fwds(void *p);
int get_fwd_dump(int lev, void *p, long *outfwd, long *next, long *face, int *faceid, int *suite, int *prefixlen, char **prefix);
int get_num_interface(void *p);
int get_interface_dump(int lev, void *p, int *ifndx, char **addr, long *dev, int *devtype, int *reflect);
int get_num_interests(void *p);
int get_interest_dump(int lev, void *p, long *interest, long *next, long *prev, int *last, int *min, int *max, int *retries, long *publisher, int *prefixlen, char **prefix);
int get_pendint_dump(int lev, void *p, char **out);
int get_num_contents(void *p);
int get_content_dump(int lev, void *p, long *content, long *next, long *prev, int *last_use, int *served_cnt, int *prefixlen, char **prefix);

//#endif

#endif // CCNL_DUMP_H
