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

// eof
