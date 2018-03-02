/*
 * @f ccnl-pkt-iottlv.h
 * @b CCN-lite - header file for a dense IOT packet format
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
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
 * 2014-11-05 created
 */

#ifndef CCNL_PKT_IOTTLV_H
#define CCNL_PKT_IOTTLV_H

#include "ccnl-core.h"

// ----------------------------------------------------------------------
// Header

/* TLV:
  2 bits  T (4 types)
  6 bits  L (0..63 Bytes)
 */

/* assigned type values and packet hierarchy:

Top level:
  0 reserved
  1 Fragment
  2 Request (aka Interest)
  3 Reply (aka Data/Content)

Fragment:
  0 Optional Headers
  1 FlagsAndSeqNr (2 bytes: BEIXnnnn nnnnnnnn)
  2 Data

Request or Reply:
  0 Optional Headers
  1 Name
  2 Payload
  3 Validation

Optional Headers:
  0 TTL
  1 Exclusions

Exclusions:
  0 KeyID
  1 ContentObjectHash

Name:
  0 PathName
  1 FlatLabel
  2 NamedFunction

PathName:
  1 NameComponent // do not assign value 0: we need to encode empty components

Payload:
  0 opt Metadata
  1 Data

Validation: (covers Name and Payload entries of a Reply or Request msg)
  0 Validation Algo Type
  1 Validation Data

*/

// ----------------------------------------------------------------------
// TLV message

#define IOT_TLV_Fragment        0x1
#define IOT_TLV_Request         0x2
#define IOT_TLV_Reply           0x3

// inside fragment
#define IOT_TLV_F_OptFragHdr    0x0
#define IOT_TLV_F_FlagsAndSeq   0x1
#define IOT_TLV_F_Data          0x2

// inside request/reply packet
#define IOT_TLV_R_OptHeader     0x0
#define IOT_TLV_R_Name          0x1
#define IOT_TLV_R_Payload       0x2
#define IOT_TLV_R_Validation    0x3

// inside header (partially optional header fields)
#define IOT_TLV_H_HopLim        0x0
#define IOT_TLV_H_Exclusion     0x1
#define IOT_TLV_H_FwdTarget     0x2

// inside exclusion
#define IOT_TLV_E_KeyID         0x0
#define IOT_TLV_E_DataObjHash   0x1

// inside FwdTarget
#define IOT_TLV_FT_FLabel       0x0
#define IOT_TLV_FT_PathName     0x1

// inside name
#define IOT_TLV_N_PathName      0x0
#define IOT_TLV_N_FlatLabel     0x1
#define IOT_TLV_N_NamedFunction 0x2

// inside hierarchical name
#define IOT_TLV_PN_Component    0x1

// inside payload
#define IOT_TLV_PL_Metadata     0x0
#define IOT_TLV_PL_Data         0x1

// inside validation
#define IOT_TLV_V_AlgoType      0x0
#define IOT_TLV_V_Bits          0x1

/**
 * Opens a TLV and reads the Type and the Length Value
 * @param buf allocated buffer in which the tlv should be opened
 * @param len length of the buffer
 * @param typ return value via pointer: type value of the tlv
 * @param vallen return value via pointer: length value of the tlv
 * @return 0 on success, -1 on failure.
 */
int
ccnl_iottlv_dehead(unsigned char **buf, int *len,
                   unsigned int *typ, int *vallen);

struct ccnl_pkt_s*
ccnl_iottlv_bytes2pkt(int pkttype, unsigned char *start,
                      unsigned char **data, int *datalen);

int
ccnl_iottlv_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c);

int
ccnl_iottlv_prependRequest(struct ccnl_prefix_s *name, int *ttl,
                           int *offset, unsigned char *buf);

int
ccnl_iottlv_prependReply(struct ccnl_prefix_s *name,
                         unsigned char *payload, int paylen,
                         int *offset, int *contentpos,
                         unsigned int *final_block_id,
                         unsigned char *buf);

int
ccnl_iottlv_peekType(unsigned char *buf, int len);

#endif // eof
