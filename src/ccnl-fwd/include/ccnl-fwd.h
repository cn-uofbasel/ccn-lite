/*
 * @f ccnl-fwd.h
 * @b CCN lite (CCNL), fwd header file (internal data structures)
 *
 * Copyright (C) 2011-17, University of Basel
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
 * 2017-06-16 created
 */

#ifndef CCNL_FWD_H
#define CCNL_FWD_H

#include "ccnl-core.h"

typedef int (*dispatchFct)(struct ccnl_relay_s*, struct ccnl_face_s*, 
                           unsigned char**, int*);

typedef int (*cMatchFct)(struct ccnl_pkt_s *p, struct ccnl_content_s *c);

struct ccnl_suite_s {
    dispatchFct RX;
    cMatchFct cMatch;
};

#ifdef USE_SUITE_CCNB
int
ccnl_ccnb_fwd(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
              unsigned char **data, int *datalen, int typ);
int
ccnl_ccnb_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                    unsigned char **data, int *datalen);
#endif // USE_SUITE_CCNB

#ifdef USE_SUITE_CCNTLV
int
ccnl_ccntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_CISTLV
int
ccnl_cistlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
#endif // USE_SUITE_CISTLV

#ifdef USE_SUITE_IOTTLV
int
ccnl_iottlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
#endif // USE_SUITE_IOTTLV

#ifdef USE_SUITE_NDNTLV
int
ccnl_ndntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
#endif // USE_SUITE_NDNTLV

int
ccnl_fwd_handleInterest(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                        struct ccnl_pkt_s **pkt, cMatchFct cMatch);

#endif
