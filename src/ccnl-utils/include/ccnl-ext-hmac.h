/**
 * @addtogroup CCNL-utils
 * @{
 *
 * @file ccnl-ext-hmac.h
 * @brief HMAC-256 signing support
 *
 * Copyright (C) 2015-18 <christian.tschudin@unibas.ch>
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
 */
#ifndef CCNL_EXT_MAC_H_
#define CCNL_EXT_MAC_H_

#include "lib-sha256.h"

#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-localrpc.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-switch.h"


void
ccnl_hmac256_keyval(unsigned char *key, int klen,
                    unsigned char *keyval); // MUST have 64 bytes (BLOCK_LENGTH)
void
ccnl_hmac256_keyid(unsigned char *key, int klen,
                   unsigned char *keyid); // MUST have 32 bytes (DIGEST_LENGTH)
// internal
void
ccnl_hmac256_keysetup(SHA256_CTX_t *ctx, unsigned char *keyval, int kvlen,
                      unsigned char pad);
void
ccnl_hmac256_sign(unsigned char *keyval, int kvlen,
                  unsigned char *data, int dlen,
                  unsigned char *md, int *mlen);
#ifdef NEEDS_PACKET_CRAFTING

#ifdef USE_SUITE_CCNTLV

// write Content packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_prependSignedContentWithHdr(struct ccnl_prefix_s *name,
                                        unsigned char *payload, int paylen,
                                        unsigned int *lastchunknum,
                                        int *contentpos,
                                        unsigned char *keyval, // 64B
                                        unsigned char *keydigest, // 32B
                                        int *offset, unsigned char *buf);
#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_NDNTLV

int
ccnl_ndntlv_prependSignedContent(struct ccnl_prefix_s *name,
                           unsigned char *payload, int paylen,
                           unsigned int *final_block_id, int *contentpos,
                           unsigned char *keyval, // 64B
                           unsigned char *keydigest, // 32B
                           int *offset, unsigned char *buf);

#endif // USE_SUITE_NDNTLV

#endif // NEEDS_PACKET_CRAFTING

#endif 
/** @} */
