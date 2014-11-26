/*
 * @f pkt-ccntlv.h
 * @b header file for CCN lite, PARC's definitions for TLV (Sep 22, 2014)
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
 * 2014-03-05 created
 */

#ifndef PKT_CCNTLV_H
#define PKT_CCNTLV_H

// ----------------------------------------------------------------------
// Header

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

// Non-TLV values for the packettype in the header
#define CCNX_PT_Interest                        1
#define CCNX_PT_ContentObject                   2

// ----------------------------------------------------------------------
// TLV message

// Version
#define CCNX_TLV_V0                             0

// optional (3.2)
// #define CCNX_TLV_O_IntLife

// top level(3.3)
#define CCNX_TLV_TL_Interest                    0x0001
#define CCNX_TLV_TL_Object                      0x0002
#define CCNX_TLV_TL_ValidationAlg               0x0003
#define CCNX_TLV_TL_ValidationPayload           0x0004

// global (3.4)
#define CCNX_TLV_G_Pad                          0x007F // TODO: correcty type?

// per msg (3.5)
// 3.5.1
#define CCNX_TLV_M_Name                         0x0000
// 3.5.2
#define CCNX_TLV_M_MetaData                     0x0001
// 3.5.3
#define CCNX_TLV_M_Payload                      0x0002

// per name (3.5.1)
#define CCNX_TLV_N_NameSegment                  0x0001
#define CCNX_TLV_N_NameNonce                    0x0002
#define CCNX_TLV_N_NameKey                      0x0003
#define CCNX_TLV_N_ObjHash                      0x0004
#define CCNX_TLV_N_Chunk                        0x0010      // var int
#define CCNX_TLV_N_Meta                         0x0011
// #define CCNX_TLV_N_App                       0xF000 - 0xF0FF

// per metadata (3.5.2)

// per interest metadata (3.5.2.1)
#define CCNX_TLV_M_KeyID                        0x0001
#define CCNX_TLV_M_ObjHash                      0x0002

// per content metadta(3.5.2.2)
#define CCNX_TLV_M_PayldType                    0x0003
#define CCNX_TLV_M_Create                       0x0004
#define CCNX_TLV_M_ENDChunk                     0x0019      // var int

// content payload type (3.5.2.2.1)
#define CCNX_PAYLDTYPE_Data                     0
#define CCNX_PAYLDTYPE_EncrypteData             1
#define CCNX_PAYLDTYPE_Key                      2
#define CCNX_PAYLDTYPE_Link                     3
#define CCNX_PAYLDTYPE_Certificate              4
#define CCNX_PAYLDTYPE_Gone                     5
#define CCNX_PAYLDTYPE_NACK                     6
#define CCNX_PAYLDTYPE_Manifest                 7

// #define CCNX_TLV_IntFrag                        0x0001 // TODO: correct type value?
// #define CCNX_TLV_ObjFrag                        0x0002 // TODO: correct type value?

// Validation stuff ignored

// per chunk number

#endif
// eof
