/*
 * @f pkt-ccntlv.h
 * @b header file for CCN lite, PARC's definitions for TLV (Nov 2013)
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

struct ccnx_tlvhdr_ccnx201311_s {
    unsigned char version;
    unsigned char msgtype;
    uint16_t payloadlen;
    uint16_t reserved;
    uint16_t hdrlen;
};

// global
#define CCNX_TLV_G_Name				0x0000
#define CCNX_TLV_G_Pad				0x007F
// #define CCNX_TLV_G_Expiment 			0xE000 - 0xEFFF

// top level
#define CCNX_TLV_TL_Interest			0x0001
#define CCNX_TLV_TL_Object			0x0002

// per hop
#define CCNX_TLV_PH_Nonce			0x0001
#define CCNX_TLV_PH_Hoplimit			0x0002
#define CCNX_TLV_PH_Fragment			0x0004

// per name
#define CCNX_TLV_N_UTF8 			0x0001
#define CCNX_TLV_N_Binary 			0x0002
#define CCNX_TLV_N_NameNonce 			0x0003
#define CCNX_TLV_N_NameKey 			0x0004
#define CCNX_TLV_N_Meta 			0x0006
#define CCNX_TLV_N_ObjHash 			0x0007
#define CCNX_TLV_N_PayloadHash 			0x0008
// #define CCNX_TLV_N_App 			0xF000 - 0xF0FF

// per interest
#define CCNX_TLV_I_KeyID 			0x0001
#define CCNX_TLV_I_ObjHash 			0x0002
#define CCNX_TLV_I_Scope 			0x0003
#define CCNX_TLV_I_Art 				0x0004
#define CCNX_TLV_I_IntLife 			0x0005

// per content
#define CCNX_TLV_C_KeyID 			0x0001
#define CCNX_TLV_C_NameAuth 			0x0002
#define CCNX_TLV_C_ProtoInfo 			0x0003
#define CCNX_TLV_C_Contents			0x0004
#define CCNX_TLV_C_SigBlock 			0x0005
#define CCNX_TLV_C_Suite 			0x0006
#define CCNX_TLV_C_PubKeyLoc 			0x0007
#define CCNX_TLV_C_Key	 			0x0008
#define CCNX_TLV_C_Cert	 			0x0009
#define CCNX_TLV_C_KeyNameKeyID			0x000A
#define CCNX_TLV_C_ObjInfo 			0x000B
#define CCNX_TLV_C_ObjType			0x000C
#define CCNX_TLV_C_Create 			0x000D
#define CCNX_TLV_C_Sigbits			0x000E
#define CCNX_TLV_C_KeyLocator			0x000F

// eof
