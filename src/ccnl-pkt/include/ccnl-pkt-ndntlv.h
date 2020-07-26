/*
 * @f pkt-ndntlv.h
 * @b CCN lite - header file for NDN (TLV pkt format March 2014)
 *
 * Copyright (C) 2014-15, Christian Tschudin, University of Basel
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
 * 2014-03-05 created
 */

#ifndef CCNL_PKT_NDNTLV_H
#define CCNL_PKT_NDNTLV_H

#ifndef CCNL_LINUXKERNEL
#include <stdint.h>
#else
#include <linux/types.h>
# define UINT8_MAX		(255)
#endif
#include <stddef.h>
#ifndef CCNL_LINUXKERNEL
#include "ccnl-content.h"
#else
#include "../../ccnl-core/include/ccnl-content.h"
#endif


/**
 * Default interest lifetime in milliseconds. If the element is omitted by a user, a default
 * value of 4 seconds is used.
 */
#ifndef NDN_DEFAULT_INTEREST_LIFETIME
#define NDN_DEFAULT_INTEREST_LIFETIME (4000u)
#endif

#define NDN_UDP_PORT                    6363
#define NDN_DEFAULT_MTU                 4096

// Packet types:
#define NDN_TLV_Interest                0x05
#define NDN_TLV_Data                    0x06
#define NDN_TLV_NDNLP                   0x64
#define NDN_TLV_Fragment                NDN_TLV_NDNLP

// Common fields:
#define NDN_TLV_Name                    0x07
#define NDN_TLV_NameComponent           0x08

// Interest packet:
#define NDN_TLV_Selectors               0x09
#define NDN_TLV_Nonce                   0x0a
#define NDN_TLV_Scope                   0x0b
#define NDN_TLV_InterestLifetime        0x0c

// Interest/Selectors:
#define NDN_TLV_MinSuffixComponents     0x0d
#define NDN_TLV_MaxSuffixComponents     0x0e
#define NDN_TLV_PublisherPublicKeyLocator 0x0f
#define NDN_TLV_Exclude                 0x10
#define NDN_TLV_ChildSelector           0x11
#define NDN_TLV_MustBeFresh             0x12
#define NDN_TLV_Any                     0x13

// Data packet:
#define NDN_TLV_MetaInfo                0x14
#define NDN_TLV_Content                 0x15
#define NDN_TLV_SignatureInfo           0x16
#define NDN_TLV_SignatureValue          0x17

// Data/MetaInfo:
#define NDN_TLV_ContentType             0x18
#define NDN_TLV_FreshnessPeriod         0x19
#define NDN_TLV_FinalBlockId            0x1a

// Data/Signature:
#define NDN_TLV_SignatureType           0x1b
#define NDN_TLV_KeyLocator              0x1c
#define NDN_TLV_KeyLocatorDigest        0x1d
#define NDN_VAL_SIGTYPE_DIGESTSHA256    0x00
#define NDN_VAL_SIGTYPE_SHA256WITHRSA   0x01
#define NDN_VAL_SIGTYPE_HMAC256         0x04

// Fragment
#define NDN_TLV_NdnlpHeader             0x50
#define NDN_TLV_NdnlpFragment           0x52
#define NDN_TLV_Frag_BeginEndFields     0x5c

// reserved values:
/*
Values          Designation

0-4, 30-79      Reserved for future assignments (1-byte encoding)
80-100          Reserved for assignments related to local link data
                processing (NDNLP header, LocalControlHeader, etc.)
101-127         Reserved for assignments related to forwarding daemon
128-252         For application use (1-byte encoding)
253-32767       Reserved for future assignments (3-byte encoding)
>32767          For application use (3-byte encoding)
*/

// Signature types (not TLV values)
#define NDN_SigTypeVal_DigestSha256             0x00
#define NDN_SigTypeVal_SignatureSha256WithRsa   0x01
#define NDN_SigTypeVal_SignatureSha256WithEcdsa 0x02
#define NDN_SigTypeVal_SignatureHmacWithSha256  0x04


// Markers (not TLV values)
// For details see: http://named-data.net/wp-content/uploads/2014/08/ndn-tr-22-ndn-memo-naming-conventions.pdf

// Segmenting markers
#define NDN_Marker_SegmentNumber 		0x00
#define NDN_Marker_ByteOffset 			0xFB

#define NDN_Marker_Version 				0xFD
#define NDN_Marker_Timestamp			0xFC
#define NDN_Marker_SequenceNumber		0xFE

/**
 * @brief NDN Interest options
 */
struct ccnl_ndntlv_interest_opts_s {
    int32_t nonce;              /**< Nonce value */
    /* Selectors */
    uint8_t mustbefresh;           /**< MustBeFresh Selector */
    /* Guiders */
    uint32_t interestlifetime;  /**< Interest Lifetime Guider */
};

/**
 * @brief NDN Data options
 */
struct ccnl_ndntlv_data_opts_s {
    /* MetaInfo */
    uint32_t freshnessperiod;       /**< freshness period */
    /* FinalBlockID is actually from type NameComponent.
     * Use integer for simplicity for now */
    uint32_t finalblockid;          /**< final block ID */
};

#ifdef USE_SUITE_NDNTLV
int8_t
ccnl_ndntlv_varlenint(uint8_t **buf, size_t *len, uint64_t *val);

uint64_t
ccnl_ndntlv_nonNegInt(uint8_t *cp, size_t len);
#endif // USE_SUITE_NDNTLV

/**
 * Opens a TLV and reads the Type and the Length Value
 * @param buf allocated buffer in which the tlv should be opened
 * @param len length of the buffer
 * @param typ return value via pointer: type value of the tlv
 * @param vallen return value via pointer: length value of the tlv
 * @return 0 on success, -1 on failure.
 */
int8_t
ccnl_ndntlv_dehead(uint8_t **buf, size_t *len,
                   uint64_t *typ, size_t *vallen);

struct ccnl_pkt_s*
ccnl_ndntlv_bytes2pkt(uint64_t pkttype, uint8_t *start,
                      uint8_t **data, size_t *datalen);

int8_t
ccnl_ndntlv_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c);

int8_t
ccnl_ndntlv_prependInterest(struct ccnl_prefix_s *name, int scope, struct ccnl_ndntlv_interest_opts_s *opts,
                            size_t *offset, uint8_t *buf, size_t *reslen);

int8_t
ccnl_ndntlv_prependContent(struct ccnl_prefix_s *name,
                           uint8_t *payload, size_t paylen,
                           size_t *contentpos, struct ccnl_ndntlv_data_opts_s *opts,
                           size_t *offset, uint8_t *buf, size_t *reslen);

int8_t
ccnl_ndntlv_prependTL(uint64_t type, uint64_t len,
                      size_t *offset, uint8_t *buf);

int8_t
ccnl_ndntlv_prependTLval(uint64_t val, size_t *offset, uint8_t *buf);

int8_t
ccnl_ndntlv_prependBlob(uint64_t type, uint8_t *blob, size_t len,
                        size_t *offset, uint8_t *buf);

int8_t
ccnl_ndntlv_prependIncludedNonNegInt(uint64_t type, uint64_t val,
                                     uint8_t marker,
                                     size_t *offset, uint8_t *buf);

int8_t
ccnl_ndntlv_prependName(struct ccnl_prefix_s *name,
                        size_t *offset, uint8_t *buf);

#endif // EOF
