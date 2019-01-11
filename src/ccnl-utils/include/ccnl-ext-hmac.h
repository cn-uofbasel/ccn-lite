/**
 * @addtogroup CCNL-utils
 * @{
 *
 * @file ccnl-ext-hmac.h
 * @brief HMAC-256 signing support based on RFC 2104
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

/**
 * @brief Generates an HMAC key
 * 
 * @note The \p keyval must be of at least 64 bytes (block length). 
 *
 * The function either copies \p klen bytes of \p key or generates a new
 * key.
 *
 * @param[in] key The value of the key (to be copied or generated)
 * @param[in] klen The length of the key
 * @param[in] keyval The (final) key (to be copied or generated)
 */
void
ccnl_hmac256_keyval(uint8_t *key, size_t klen,
                    uint8_t *keyval);

/**
 * @brief TODO
 * 
 * @note The \p keyid must be of at least 32 bytes (digest length). 
 *
 * @param[in] key The actual key
 * @param[in] klen The length of the key
 * @param[in] keyid TODO
 */
void
ccnl_hmac256_keyid(uint8_t *key, size_t klen,
                   uint8_t *keyid);

/**
 * @brief Adds padding bytes to a key 
 * 
 * @param[in] ctx Context 'object' of the underlying SHA256 implementation
 * @param[in] keyval The key to pad
 * @param[in] kvlen The length of the key
 * @param[in] pad The padding byte
 */
void
ccnl_hmac256_keysetup(SHA256_CTX_t *ctx, uint8_t *keyval, size_t kvlen,
                      uint8_t pad);

/**
 * @brief Generates an HMAC signature
 * 
 * @param[in]  keyval The key
 * @param[in]  kvlen The lengthof the key
 * @param[in]  data The data to sign
 * @param[in]  dlen The length of the sign
 * @param[in]  md The message digest
 * @param[out] mlen The length of the message digest
 */
void
ccnl_hmac256_sign(uint8_t *keyval, size_t kvlen,
                  uint8_t *data, size_t dlen,
                  uint8_t *md, size_t *mlen);


#ifdef NEEDS_PACKET_CRAFTING
#ifdef USE_SUITE_CCNTLV
/**
 * @brief Signs CCNx content and prepends signature with the header
 *
 * @note The content is before the \p offset in \p buf. The function adjusts the
 * \p offset.
 *
 * @param[in]  name The prefix of the content to sign
 * @param[in]  payload The actual content
 * @param[in]  paylen The length of \p payload
 * @param[in]  lastchunknum Position of the last chunk in the \p buf
 * @param[in]  contentpos Position of the content in the \p buf
 * @param[in]  keyval The key to use for signing the content (>= 64 bytes)
 * @param[in]  keydigest The digest (>= 32 bytes)
 * @param[out] offset TODO
 * @param[out] buf A byte representation of the actual packet
 *
 * @return Upon success, the function returns the number of used bytes
 */
int8_t
ccnl_ccntlv_prependSignedContentWithHdr(struct ccnl_prefix_s *name,
                                        uint8_t *payload, size_t paylen,
                                        uint32_t *lastchunknum,
                                        size_t *contentpos,
                                        uint8_t *keyval, // 64B
                                        uint8_t *keydigest, // 32B
                                        size_t *offset, uint8_t *buf, size_t *retlen);
#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_NDNTLV
/**
 * @brief Signs an NDO and prepends signature
 *
 * @param[in]  name The prefix of the content to sign
 * @param[in]  payload The actual content
 * @param[in]  paylen The length of \p payload
 * @param[in]  final_block_id Denotes position of optional MetaInfo fields
 * @param[in]  contentpos Position of the content in the \p buf
 * @param[in]  keyval The key to use for signing the content (>= 64 bytes)
 * @param[in]  keydigest The digest (>= 32 bytes)
 * @param[out] offset TODO
 * @param[out] buf A byte representation of the actual packet
 *
 * @return 0 upon success, nonzero upon failure
 */
int8_t
ccnl_ndntlv_prependSignedContent(struct ccnl_prefix_s *name,
                                 uint8_t *payload, size_t paylen,
                                 uint32_t *final_block_id, size_t *contentpos,
                                 uint8_t *keyval, // 64B
                                 uint8_t *keydigest, // 32B
                                 size_t *offset, uint8_t *buf, size_t *reslen);
#endif // USE_SUITE_NDNTLV
#endif // NEEDS_PACKET_CRAFTING

#endif 
/** @} */
