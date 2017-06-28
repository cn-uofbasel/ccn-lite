/*
 * @f pkt-cistlv.h
 * @b header file for CCN lite, Mark Stapp's (Cisco) defs for TLV (Jan 6, 2015)
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-01-06 created
 */

// ----------------------------------------------------------------------
// Header

struct cisco_tlvhdr_201501_s {
    unsigned char version;
    unsigned char pkttype;
    unsigned short pktlen;
    unsigned char hoplim;  // not used for data
    unsigned char flags;
    unsigned char resvd;
    unsigned char hlen;
} __attribute__((packed));


// Non-TLV values for the packettype in the header
#define CISCO_PT_Interest                        1
#define CISCO_PT_Content                         2
#define CISCO_PT_Nack                            3

// ----------------------------------------------------------------------
// TLV message

// Version
#define CISCO_TLV_V1                             1

// T values

#define CISCO_TLV_Interest                       1
#define CISCO_TLV_Content                        2

#define CISCO_TLV_Name                           1
#define CISCO_TLV_NameComponent                  1
#define CISCO_TLV_NameSegment                    2
#define CISCO_TLV_ContentData                    4
#define CISCO_TLV_Certificate                    5
#define CISCO_TLV_SignedInfo                     6
#define CISCO_TLV_ContentDigest                  7
#define CISCO_TLV_PublicKey                      8
#define CISCO_TLV_KeyName                        9
#define CISCO_TLV_KeyNameComponent               1
#define CISCO_TLV_Signature                      10
#define CISCO_TLV_Timestamp                      11
#define CISCO_TLV_Witness                        12
#define CISCO_TLV_SignatureBits                  13
#define CISCO_TLV_DigestAlgorithm                14
#define CISCO_TLV_FinalSegmentID                 15
#define CISCO_TLV_PublisherPublicKeyDigest       16
#define CISCO_TLV_PublisherPublicKeyName         17
#define CISCO_TLV_ContentExpiration              18
#define CISCO_TLV_CacheTTL                       19
#define CISCO_TLV_VendorSpecific                 20
#define CISCO_TLV_VendorId                       21

#define CISCO_ERR_NOROUTE                        1
#define CISCO_ERR_CONGESTED                      2
#define CISCO_ERR_NORESOURCE                     3
#define CISCO_ERR_POLICY                         4

// eof
