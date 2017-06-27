//
// Created by Balázs Faludi on 27.06.17.
//

#ifndef CCNL_NFNUTIL_H
#define CCNL_NFNUTIL_H

#include "ccnl-prefix.h"

#ifdef NEEDS_PACKET_CRAFTING
struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen, int *payoffset);

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce);
#endif

#endif //CCNL_NFNUTIL_H
