/*
 * @f util/ccn-lite-ndntlv2hex.c
 * @b CCN lite - parse a NDN-TLV encoded packet (encoding as of March 2014)
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
 * 2014-03-13 created
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "../pkt-ndntlv-dec.c"

#define NDN_TLV_MAX_TYPE 256
static char ndn_tlv_recurse[NDN_TLV_MAX_TYPE];

// ----------------------------------------------------------------------

static char*
type2name_ndn(unsigned type)
{
    char *n;

    switch (type) {
    case NDN_TLV_Interest:		n = "Interest"; break;
    case NDN_TLV_Data:			n = "Data"; break;
    case NDN_TLV_Name:			n = "Name"; break;
    case NDN_TLV_NameComponent:		n = "NameComponent"; break;
    case NDN_TLV_Selectors:		n = "Selectors"; break;
    case NDN_TLV_Nonce:			n = "Nonce"; break;
    case NDN_TLV_Scope:			n = "Scope"; break;
    case NDN_TLV_InterestLifetime:	n = "InterestLifetime"; break;
    case NDN_TLV_MinSuffixComponents:	n = "MinSuffixComponents"; break;
    case NDN_TLV_MaxSuffixComponents:	n = "MaxSuffixComponents"; break;
    case NDN_TLV_PublisherPublicKeyLocator: n = "PublisherPublicKeyLocator"; break;
    case NDN_TLV_Exclude:		n = "Exclude"; break;
    case NDN_TLV_ChildSelector:		n = "ChildSelector"; break;
    case NDN_TLV_MustBeFresh:		n = "MustBeFresh"; break;
    case NDN_TLV_Any:			n = "Any"; break;
    case NDN_TLV_MetaInfo:		n = "MetaInfo"; break;
    case NDN_TLV_Content:		n = "Content"; break;
    case NDN_TLV_SignatureInfo:		n = "SignatureInfo"; break;
    case NDN_TLV_SignatureValue:	n = "SignatureValue"; break;
    case NDN_TLV_ContentType:		n = "ContentType"; break;
    case NDN_TLV_FreshnessPeriod:	n = "FreshnessPeriod"; break;
    case NDN_TLV_FinalBlockId:		n = "FinalBlockId"; break;
    case NDN_TLV_SignatureType:		n = "SignatureType"; break;
    case NDN_TLV_KeyLocator:		n = "KeyLocator"; break;
    case NDN_TLV_KeyLocatorDigest:	n = "KeyLocatorDigest"; break;
    default:
	n = NULL;
    }
    return n;
}


static int
parse_ndntlv_sequence(int lev, unsigned char *base, unsigned char **buf,
		       unsigned int *len, char *here)
{
    unsigned int vallen, typ, i, maxi;
    unsigned char *cp;
    char *n, tmp[100];

    while (*len > 0) {
	cp = *buf;
	if (ccnl_ndntlv_dehead(buf, len, &typ, &vallen) < 0)
	    return -1;
	if (vallen > *len) {
	    fprintf(stderr, "\n%04zx ** NDN_TLV length problem:\n"
		    "  type=%hu, len=%hu larger than %d available bytes\n",
		    cp - base, typ, vallen, *len);
	    exit(-1);
	}

	n = type2name_ndn(typ);
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

	if (typ < NDN_TLV_MAX_TYPE && ndn_tlv_recurse[typ]) {
	    *len -= vallen;
	    if (parse_ndntlv_sequence(lev+1, base, buf, &vallen, n) < 0)
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


static void
tlv_ndn201311(unsigned char *data, unsigned int len)
{
    unsigned char *buf = data;

    // dump the sequence of TLV fields, should start with a name TLV
    parse_ndntlv_sequence(0, data, &buf, &len, "payload");
    printf("%04zx  pkt.end\n", buf - data);
}


int
main(int argc, char *argv[])
{
    unsigned char data[64*1024];
    int rc;
    unsigned int len, maxlen;

    ndn_tlv_recurse[NDN_TLV_Interest] = 1;
    ndn_tlv_recurse[NDN_TLV_Data] = 1;
    ndn_tlv_recurse[NDN_TLV_Name] = 1;
    ndn_tlv_recurse[NDN_TLV_Selectors] = 1;
    ndn_tlv_recurse[NDN_TLV_MetaInfo] = 1;
    ndn_tlv_recurse[NDN_TLV_SignatureInfo] = 1;
    ndn_tlv_recurse[NDN_TLV_KeyLocator] = 1;

    if (argc > 1) {
	fprintf(stderr, "usage: %s < ndntlv_encoded_data\n", argv[0]);
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

    printf("# Parsing %d byte%s (NDN TLV Mar 2014)\n#\n",
	   len, len!=1 ? "s":"");
    tlv_ndn201311(data, len);

    return 0;
}
