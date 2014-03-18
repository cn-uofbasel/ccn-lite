/*
 * @f util/ccn-lite-ccntlv2hex.c
 * @b CCN lite - parse a CCNx-TLV encoded packet (encoding as of Nov 2013)
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
 * 2014-03-16 created: based on ccn-lite-ndntlv2hex.c
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "../pkt-ccntlv.c"

enum {
    CTX_GLOBAL = 1,
    CTX_TOPLEVEL,
    CTX_HOP,
    CTX_NAME,
    CTX_INTEREST,
    CTX_CONTENT,
    CTX_NAMEAUTH // ?
};

static char recurse[][3] = {
    {CTX_TOPLEVEL, CCNX_TLV_TL_Interest, CTX_INTEREST},
    {CTX_TOPLEVEL, CCNX_TLV_TL_Object, CTX_CONTENT},
    {CTX_INTEREST, CCNX_TLV_G_Name, CTX_NAME},
    {CTX_CONTENT, CCNX_TLV_G_Name, CTX_NAME},

    {CTX_CONTENT, CCNX_TLV_C_NameAuth, CTX_CONTENT},
    {CTX_CONTENT, CCNX_TLV_C_KeyLocator, CTX_CONTENT},
    {CTX_CONTENT, CCNX_TLV_C_SigBlock, CTX_CONTENT},


    {0,0,0}
};

static char
must_recurse(char ctx, char typ)
{
    int i;
    for (i = 0; recurse[i][0]; i++) {
	if (recurse[i][0] == ctx && recurse[i][1] == typ)
	    return recurse[i][2];
    }
    return 0;
}


// ----------------------------------------------------------------------

static char*
ccnl_ccntlv_type2name(unsigned char ctx, unsigned int type)
{
    char *cn = "globalCtx", *tn = NULL;
    static char tmp[50];

//    fprintf(stderr, "t2n %d %d\n", ctx, type);

    if (type == CCNX_TLV_G_Name)
	tn = "Name";
    else if (type == CCNX_TLV_G_Pad)
	tn = "Pad";
    else {
	switch (ctx) {
/*
	case CTX_GLOBAL:
	    cn = "global";
	    switch (type) {
	    case CCNX_TLV_G_Name:		tn = "Name"; break;
	    case CCNX_TLV_G_Pad:		tn = "Pad"; break;
	    default: break;
	    }
	    break;
*/
	case CTX_TOPLEVEL:
	    cn = "toplevelCtx";
	    switch (type) {
	    case CCNX_TLV_TL_Interest:	tn = "Interest"; break;
	    case CCNX_TLV_TL_Object:	tn = "Object"; break;
	    default: break;
	    }
	    break;
	case CTX_HOP:
	    cn = "hopCtx";
	    switch (type) {
	    case CCNX_TLV_PH_Nonce:	tn = "Nonce"; break;
	    case CCNX_TLV_PH_Hoplimit:	tn = "Hoplimit"; break;
	    default: break;
	    }
	    break;
	case CTX_NAME:
	    cn = "nameCtx";
	    switch (type) {
	    case CCNX_TLV_N_UTF8:	tn = "UTF8"; break;
	    case CCNX_TLV_N_Binary:	tn = "Binary"; break;
	    case CCNX_TLV_N_NameNonce:	tn = "NameNonce"; break;
	    case CCNX_TLV_N_NameKey:	tn = "NameKey"; break;
	    case CCNX_TLV_N_Meta:	tn = "Meta"; break;
	    case CCNX_TLV_N_ObjHash:	tn = "ObjHash"; break;
	    case CCNX_TLV_N_PayloadHash: tn = "PayloadHash"; break;
	    default: break;
	    }
	    break;
	case CTX_INTEREST:
	    cn = "interestCtx";
	    switch (type) {
	    case CCNX_TLV_I_KeyID:	tn = "KeyID"; break;
	    case CCNX_TLV_I_ObjHash:	tn = "ObjHash"; break;
	    case CCNX_TLV_I_Scope:	tn = "Scope"; break;
	    case CCNX_TLV_I_Art:	tn = "Art"; break;
	    case CCNX_TLV_I_IntLife:	tn = "IntLife"; break;
	    default: break;
	    }
	    break;
	case CTX_CONTENT:
	    cn = "contentCtx";
	    switch (type) {
	    case CCNX_TLV_C_KeyID:	tn = "KeyID"; break;
	    case CCNX_TLV_C_NameAuth:	tn = "NameAuth"; break;
	    case CCNX_TLV_C_ProtoInfo:	tn = "ProtoInfo"; break;
	    case CCNX_TLV_C_Contents:	tn = "Contents"; break;
	    case CCNX_TLV_C_SigBlock:	tn = "SigBlock"; break;
	    case CCNX_TLV_C_Suite:	tn = "Suite"; break;
	    case CCNX_TLV_C_PubKeyLoc:	tn = "PubKeyLoc"; break;
	    case CCNX_TLV_C_Key:	tn = "Key"; break;
	    case CCNX_TLV_C_Cert:	tn = "Cert"; break;
	    case CCNX_TLV_C_KeyNameKeyID: tn = "KeyNameKeyID"; break;
	    case CCNX_TLV_C_ObjInfo:	tn = "ObjInfo"; break;
	    case CCNX_TLV_C_ObjType:	tn = "ObjType"; break;
	    case CCNX_TLV_C_Create:	tn = "Create"; break;
	    case CCNX_TLV_C_Sigbits:	tn = "Sigbits"; break;
	    case CCNX_TLV_C_KeyLocator:	tn="KeyLocator"; break;
	    default: break;
	    }
	    break;
	case CTX_NAMEAUTH:
	    cn = "nameauthCtx";
	    break;
	default:
	    cn = NULL;
	    break;
	}
    }
    if (tn)
	sprintf(tmp, "%s\\%s", tn, cn);
    else if (cn)
	sprintf(tmp, "type=0x%04x\\%s", type, cn);
    else
	sprintf(tmp, "type=0x%04x\\ctx=%d", type, ctx);
    return tmp;
}


static int
parse_ccntlv_sequence(int lev, unsigned char ctx, unsigned char *base,
		      unsigned char **buf, unsigned int *len, char *here)
{
    unsigned int vallen, typ, i, maxi;
    unsigned char ctx2, *cp;
    char *n, tmp[100];

    while (*len > 0) {
//	fprintf(stderr, "# >> parsing sequence of %d bytes (offset=%d)\n", *len, (int)(*buf - base));

	cp = *buf;
	if (ccnl_ccntlv_dehead(lev, base, buf, len, &typ, &vallen) < 0)
	    return -1;

	if (vallen > *len) {
	  fprintf(stderr, "\n%04zx ** CCNTLV length problem:\n"
		  "  type=%hu, len=%hu larger than %d available bytes\n",
		  *buf - base, typ, vallen, *len);
	  exit(-1);
	}

	n = ccnl_ccntlv_type2name(ctx, typ);
	if (!n) {
	    sprintf(tmp, "type=%hu", typ);
	    n = tmp;
	}

	printf("%04zx  ", cp - base);
	for (i = 0; i < lev; i++)
	    printf("  ");
	for (; cp < *buf; cp++)
	    printf("%02x ", *cp);
	printf("-- <%s, len=%d>\n", n, vallen);

//	if (typ < CCNTLV_MAX_TYPE && ccntlv_recurse[typ]) {
	ctx2 = must_recurse(ctx, typ);
	if (ctx2) {
	    *len -= vallen;
	    if (parse_ccntlv_sequence(lev+1, ctx2, base, buf, &vallen, n) < 0)
		return -1;
	    continue;
	}

	while (vallen > 0) {
	    maxi = vallen > 8 ? 8 : vallen;
	    cp = *buf;
	    printf("%04zx  ", cp - base);
	    for (i = 0; i < lev+1; i++)
		printf("  ");
	    for (i = 0; i < 8; i++, cp++)
		printf(i < maxi ? "%02x " : "   ", *cp);
	    cp = *buf;
	    for (i = 79 - 6 - 2*(lev+1) - 8*3 - 12; i > 0; i--)
		printf(" ");
	    printf("  |");
	    for (i = 0; i < maxi; i++, cp++)
		printf("%c", isprint(*cp) ? *cp : '.');
	    printf("|\n");

	    vallen -= maxi;
	    *buf += maxi;
	    *len -= maxi;
	}
    }
    return 0;
}


void
tlv_ccn201311(unsigned char *data, unsigned int len)
{
    unsigned char *buf;
    char *mp;
    struct ccnx_tlvhdr_ccnx201311_s *hp;

    hp = (struct ccnx_tlvhdr_ccnx201311_s*) data;

    printf("%04zx  hdr.vers=%d\n",
	   (unsigned char*) &(hp->version) - data, hp->version);
    if (hp->msgtype == CCNX_TLV_TL_Interest)
	mp = "Interest\\toplevelCtx";
    else if (hp->msgtype == CCNX_TLV_TL_Object)
	mp = "Object\\toplevelCtx";
    else
	mp = "unknown";
    printf("%04zx  hdr.mtyp=0x%02x (%s)\n",
	   (unsigned char*) &(hp->msgtype) - data, hp->msgtype, mp);
    printf("%04zx  hdr.plen=%d\n",
	   (unsigned char*) &(hp->payloadlen) - data, ntohs(hp->payloadlen));
    printf("%04zx  hdr.olen=%d\n",
	   (unsigned char*) &(hp->hdrlen) - data, ntohs(hp->hdrlen));

    if (8 + ntohs(hp->hdrlen) + ntohs(hp->payloadlen) != len) {
	fprintf(stderr, "length mismatch\n");
    }

    buf = data + 8;
    // dump the sequence of TLV fields of the optional header
    len = ntohs(hp->hdrlen);
    if (len > 0) {
	parse_ccntlv_sequence(0, CTX_HOP, data, &buf, &len, "header");
	if (len != 0)
	    fprintf(stderr, "%d left over bytes in header\n", len);
    }
    printf("%04zx  hdr.end\n", buf - data);

    // dump the sequence of TLV fields of the payload
    len = ntohs(hp->payloadlen);
    buf = data + 8 + ntohs(hp->hdrlen);
    parse_ccntlv_sequence(0, CTX_TOPLEVEL, data, &buf, &len, "payload");
    if (len != 0) {
    }

    printf("%04zx  pkt.end\n", buf - data);
}


int
main(int argc, char *argv[])
{
    unsigned char data[64*1024];
    int rc;
    unsigned int len, maxlen;

    if (argc > 1) {
	fprintf(stderr, "usage: %s < ccntlv_encoded_data\n", argv[0]);
	return 2;
    }

    len = 0;
    maxlen = sizeof(data);
    while (maxlen > 0) {
	rc = read(0, data+len, maxlen);
	if (rc == 0)
	    break;
	if (rc < 0) {
	    perror("read");
	    return 1;
	}
	len += rc;
	maxlen -= rc;
    }

    printf("# Parsing %d byte%s (CCNx TLV Nov 2013 format) //patched 20140318\n#\n",
	   len, len!=1 ? "s" : "");
    tlv_ccn201311(data, len);

    return 0;
}
