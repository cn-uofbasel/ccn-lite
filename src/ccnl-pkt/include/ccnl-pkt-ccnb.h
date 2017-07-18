/*
 * @f ccnl-pkt-ccnb.h
 * @b CCN lite - CCNb wire format constants
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
 * 2011-03-13 created
 * 2014-03-17 renamed to prepare for a world with many wire formats
 */

#ifndef CCNL_PKT_CCNB_H
#define CCNL_PKT_CCNB_H

#include "ccnl-core.h"

// ----------------------------------------------------------------------

#define CCN_UDP_PORT            9695
#define CCN_DEFAULT_MTU         4096

// ----------------------------------------------------------------------

#define CCN_TT_TAG              1
#define CCN_TT_DTAG             2
#define CCN_TT_ATTR             3
#define CCN_TT_DATTR            4
#define CCN_TT_BLOB             5
#define CCN_TT_UDATA            6

#define CCN_DTAG_ANY            13
#define CCN_DTAG_NAME           14
#define CCN_DTAG_COMPONENT      15
#define CCN_DTAG_CERTIFICATE    16
#define CCN_DTAG_CONTENT        19
#define CCN_DTAG_SIGNEDINFO     20
#define CCN_DTAG_CONTENTDIGEST  21
#define CCN_DTAG_INTEREST       26
#define CCN_DTAG_KEY            27
#define CCN_DTAG_KEYLOCATOR     28
#define CCN_DTAG_KEYNAME        29
#define CCN_DTAG_SIGNATURE      37
#define CCN_DTAG_TIMESTAMP      39
#define CCN_DTAG_TYPE           40
#define CCN_DTAG_NONCE          41
#define CCN_DTAG_SCOPE          42
#define CCN_DTAG_EXCLUDE        43
#define CCN_DTAG_ANSWERORIGKIND 47
#define CCN_DTAG_WITNESS        53
#define CCN_DTAG_SIGNATUREBITS  54
#define CCN_DTAG_DIGESTALGO     55
#define CCN_DTAG_FRESHNESS      58
#define CCN_DTAG_FINALBLOCKID   59
#define CCN_DTAG_PUBPUBKDIGEST  60
#define CCN_DTAG_PUBCERTDIGEST  61
#define CCN_DTAG_CONTENTOBJ     64
#define CCN_DTAG_ACTION         73
#define CCN_DTAG_FACEID         74
#define CCN_DTAG_IPPROTO        75
#define CCN_DTAG_HOST           76
#define CCN_DTAG_PORT           77
#define CCN_DTAG_FWDINGFLAGS    79
#define CCN_DTAG_FACEINSTANCE   80
#define CCN_DTAG_FWDINGENTRY    81
#define CCN_DTAG_MINSUFFCOMP    83
#define CCN_DTAG_MAXSUFFCOMP    84
#define CCN_DTAG_SEQNO          256
#define CCN_DTAG_FragA          448
#define CCN_DTAG_FragB          449
#define CCN_DTAG_FragC          450
#define CCN_DTAG_FragD          451
#define CCN_DTAG_FragP          463
#define CCN_DTAG_CCNPDU         17702112

int
ccnl_ccnb_dehead(unsigned char **buf, int *len, int *num, int *typ);

struct ccnl_pkt_s*
ccnl_ccnb_bytes2pkt(unsigned char *start, unsigned char **data, int *datalen);

int
ccnl_ccnb_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c);

int
ccnl_ccnb_fillInterest(struct ccnl_prefix_s *name, int *nonce,
                       unsigned char *out, int outlen);

int
ccnl_ccnb_fillContent(struct ccnl_prefix_s *name, unsigned char *data,
                      int datalen, int *contentpos, unsigned char *out);

int
ccnl_ccnb_consume(int typ, int num, unsigned char **buf, int *len,
                  unsigned char **valptr, int *vallen);

#ifdef NEEDS_PACKET_CRAFTING
int
ccnl_ccnb_mkHeader(unsigned char *buf, unsigned int num, unsigned int tt);

int
ccnl_ccnb_mkStrBlob(unsigned char *out, unsigned int num, unsigned int tt,
                    char *str);

int
ccnl_ccnb_mkBlob(unsigned char *out, unsigned int num, unsigned int tt,
                 char *cp, int cnt);

int
ccnl_ccnb_mkField(unsigned char *out, unsigned int num, int typ,
                  unsigned char *data, int datalen);
#endif // NEEDS_PACKET_CRAFTING

#endif // eof
