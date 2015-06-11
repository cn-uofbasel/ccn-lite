/*
 * @f pkt-ccntlv.h
 * @b header file for CCN lite, PARC's definitions for TLV (Sep 22, 2014)
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

#ifndef CCN_UDP_PORT
# define CCN_UDP_PORT                    9695
#endif

// ----------------------------------------------------------------------
// Header

struct ccnx_tlvhdr_ccnx2015_s {
    unsigned char  version;
    unsigned char  pkttype;
    uint16_t       pktlen;
    unsigned char  hoplimit;  // not used for data
    unsigned char  fill[2];   // fill[0] is errcode, Frag: flagsAndSeqNr
    unsigned char  hdrlen;
} __attribute__((packed));

/*
struct ccnx_tlvhdr_ccnx2015nack_s { // "interest return" extension
    unsigned int  errorc;
    unsigned char fill[2];
} __attribute__((packed));
*/

/*
struct ccnx_tlvhdr_ccnx201411_s {
    unsigned char version;
    unsigned char packettype;
    uint16_t payloadlen;
    unsigned char hoplimit;
  //    uint16_t reserved;
    unsigned char reserved;
    unsigned char reserved2;
    unsigned char hdrlen;
} __attribute__((packed));
*/

/*
struct ccnx_tlvhdr_ccnx201409_s {
    unsigned char version;
    unsigned char packettype;
    uint16_t payloadlen;
    unsigned char hoplimit;
    unsigned char reserved;
    uint16_t hdrlen;
} __attribute__((packed));
*/

// Non-TLV values for the fields in the header

// #define CCNX_TLV_V0                             0
#define CCNX_TLV_V1                             1

#define CCNX_PT_Interest                        0
#define CCNX_PT_Data                            1
#define CCNX_PT_NACK                            2 // "Interest Return"
#define CCNX_PT_Fragment                        3 // fragment

// NACK/Interest Return codes (fill[0]):
#define CCNX_TLV_NACK_NOROUTE      1
#define CCNX_TLV_NACK_HOPLIMIT     2
#define CCNX_TLV_NACK_NORESOURCES  3
#define CCNX_TLV_NACK_PATHERROR    4
#define CCNX_TLV_NACK_PROHIBITED   5
#define CCNX_TLV_NACK_CONGESTED    6
#define CCNX_TLV_NACK_MTUTOOLARGE  7

// ----------------------------------------------------------------------
// TLV message

// optional hop-by-hop (Sect 3.3)
// #define CCNX_TLV_O_IntLife                      1
// #define CCNX_TLV_O_CacheTime                    2

// top level(Sect 3.4)
#define CCNX_TLV_TL_Interest                    0x0001
#define CCNX_TLV_TL_Object                      0x0002
#define CCNX_TLV_TL_ValidationAlgo              0x0003
#define CCNX_TLV_TL_ValidationPayload           0x0004
#define CCNX_TLV_TL_Fragment                    0x0005

// global (Sect 3.5.1)
#define CCNX_TLV_G_Pad                          0x007F // TODO: correcty type?

// per msg (Sect 3.6)
// 3.6.1
#define CCNX_TLV_M_Name                         0x0000
#define CCNX_TLV_M_Payload                      0x0001
#define CCNX_TLV_M_ENDChunk                     0x0019 // chunking document

// per name (Sect 3.6.1)

#define CCNX_TLV_N_NameSegment                  0x0001
#define CCNX_TLV_N_IPID                         0x0002
#define CCNX_TLV_N_Chunk                        0x0010 // chunking document
#define CCNX_TLV_N_Meta                         0x0011
// #define CCNX_TLV_N_App                       0x1000 - 0x1FFF

// meta
// ...

// (opt) message TLVs (Sect 3.6.2)

#define CCNX_TLV_M_KeyIDRestriction             0x0002
#define CCNX_TLV_M_ObjHashRestriction           0x0003
#define CCNX_TLV_M_IPIDM                        0x0004

// (opt) content msg TLVs (Sect 3.6.2.2)

#define CCNX_TLV_C_PayloadType                  0x0005
#define CCNX_TLV_C_ExpiryTime                   0x0006

// content payload type (Sect 3.6.2.2.1)
#define CCNX_PAYLDTYPE_Data                     0
#define CCNX_PAYLDTYPE_Key                      1
#define CCNX_PAYLDTYPE_Link                     2
#define CCNX_PAYLDTYPE_Manifest                 3

// validation algorithms (Sect 3.6.4.1)
#define CCNX_VALIDALGO_CRC32C                   2
#define CCNX_VALIDALGO_HMAC_SHA256              4
#define CCNX_VALIDALGO_VMAC_128                 5
#define CCNX_VALIDALGO_RSA_SHA256               6
#define CCNX_VALIDALGO_EC_SECP_256K1            7
#define CCNX_VALIDALGO_EC_SECP_384R1            8

// validation dependent data (Sect 3.6.4.1.4)
#define CCNX_VALIDALGO_KEYID                    9
#define CCNX_VALIDALGO_PUBLICKEY                0x000b
#define CCNX_VALIDALGO_CERT                     0x000c
#define CCNX_VALIDALGO_KEYNAME                  0x000e
#define CCNX_VALIDALGO_SIGTIME                  0x000f

// #define CCNX_TLV_IntFrag                        0x0001 // TODO: correct type value?
// #define CCNX_TLV_ObjFrag                        0x0002 // TODO: correct type value?

// eof
