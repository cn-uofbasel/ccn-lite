/*
 * @f util/ccn-lite-parse.c
 * @b CCN lite - parse or generate ccnb data from stdin
 *
 * Copyright (C) 2011-13, Christian Tschudin, University of Basel
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
 * 2011-03-13 created (cft): orig name ccnl-parse-ccnb.c
 * 2013-03-18 updated (ms): brought here mkInterest() and mkContent() and
 *  renamed the file to ccnl-ccnb.c
 */

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include "../ccnx.h"
#include "../ccnl.h"

#include "ccnl-common.c"

// ----------------------------------------------------------------------

int
deheadAndPrint(int lev, unsigned char *base, unsigned char **buf,
       int *len, int *num, int *typ)
{
     int i, val = 0;

    if (*len <= 0)
	return -1;

    printf("0x%04zx ", *buf - base);
    for (i = 0; i < lev; i++) printf("  ");
    if (**buf == 0) {
	printf("0x00 ");
	*num = *typ = 0;
	*buf += 1;
	*len -= 1;
	return 0;
    }
    for (i = 0; i < (int)sizeof(i) && i < *len; i++) {
	unsigned char c = (*buf)[i];
	printf("0x%02x ", c);
	if ( c & 0x80 ) {
	    *num = (val << 4) | ((c >> 3) & 0xf);
	    *typ = c & 0x7;
	    *buf += i+1;
	    *len -= i+1;
	    return 0;
	}
	val = (val << 7) | c;
    } 
    printf("?");
    return -1;
}

static int
test_recursive_parsing(/* work in progress */)
{
    return 0;
}


static int
parse_lev(int lev, unsigned char *base, unsigned char **buf,
	       int *len, char *here)
{
    int num, typ, i, j;
    char *n;

    while (deheadAndPrint(lev, base, buf, len, &num, &typ) == 0) {
	switch (typ) {
	case CCN_TT_BLOB:
	case CCN_TT_UDATA:
	    printf(" -- <data (%d byte%s", num, num > 1 ? "s" : "");
	    if (num < 16) {
		printf(": \"");
		while (num-- > 0) {
		    if (isprint(**buf) && **buf != '%')
			printf("%c", **buf);
		    else
			printf("%%%02x", **buf);
		    *buf += 1; *len -= 1;
		}
		printf("\")>\n");
	    } else if (test_recursive_parsing(/* work in progress */)) {
		printf(")>, decoding:\n");
		// ...
		*buf += num;
		*len -= num;
 	    } else {
		printf(")>");
		for (i = 0; i < num; i++) {
		    if ((i%8) == 0) {
			printf("\n0x%04zx  ", *buf - base + i);
			for (j = 0; j < lev; j++) printf("  ");
		    }
		    printf(" 0x%02x", (*buf)[i]);
		}
		printf("\n");
		*buf += num;
		*len -= num;
	    } 
	    break;
	case CCN_TT_DTAG:
	    switch (num) {
	    case CCN_DTAG_ANY:		 n = "any"; break;
	    case CCN_DTAG_NAME:		 n = "name"; break;
	    case CCN_DTAG_COMPONENT:	 n = "component"; break;
	    case CCN_DTAG_CONTENT:	 n = "content"; break;
	    case CCN_DTAG_SIGNEDINFO:    n = "signedinfo"; break;
	    case CCN_DTAG_INTEREST:	 n = "interest"; break;
	    case CCN_DTAG_KEY:		 n = "key"; break;
	    case CCN_DTAG_KEYLOCATOR:	 n = "keylocator"; break;
	    case CCN_DTAG_KEYNAME:	 n = "keyname"; break;
	    case CCN_DTAG_SIGNATURE:	 n = "signature"; break;
	    case CCN_DTAG_TIMESTAMP:	 n = "timestamp"; break;
	    case CCN_DTAG_TYPE:		 n = "type"; break;
	    case CCN_DTAG_NONCE:	 n = "nonce"; break;
	    case CCN_DTAG_SCOPE:	 n = "scope"; break;
	    case CCN_DTAG_WITNESS:	 n = "witness"; break;
	    case CCN_DTAG_SIGNATUREBITS: n = "signaturebits"; break;
	    case CCN_DTAG_FRESHNESS:	 n = "freshnessSeconds"; break;
	    case CCN_DTAG_FINALBLOCKID:  n = "finalblockid"; break;
	    case CCN_DTAG_PUBPUBKDIGEST: n = "publisherPubKeyDigest"; break;
	    case CCN_DTAG_PUBCERTDIGEST: n = "publisherCertDigest"; break;
	    case CCN_DTAG_CONTENTOBJ:	 n = "contentobj"; break;
	    case CCN_DTAG_ACTION:	 n = "action"; break;
	    case CCN_DTAG_FACEID:	 n = "faceID"; break;
	    case CCN_DTAG_IPPROTO:	 n = "ipProto"; break;
	    case CCN_DTAG_HOST:		 n = "host"; break;
	    case CCN_DTAG_PORT:		 n = "port"; break;
	    case CCN_DTAG_FWDINGFLAGS:	 n = "forwardingFlags"; break;
	    case CCN_DTAG_FACEINSTANCE:	 n = "faceInstance"; break;
	    case CCN_DTAG_FWDINGENTRY:	 n = "forwardingEntry"; break;
	    case CCN_DTAG_MINSUFFCOMP:	 n = "minSuffixComponents"; break;
	    case CCN_DTAG_MAXSUFFCOMP:	 n = "maxSuffixComponents"; break;
	    case CCN_DTAG_SEQNO:	 n = "sequenceNumber"; break;
	    case CCN_DTAG_CCNPDU:	 n = "ccnProtocolDataUnit"; break;

	    case CCNL_DTAG_MACSRC:	 n = "MACsrc"; break;
	    case CCNL_DTAG_IP4SRC:	 n = "IP4src"; break;
	    case CCNL_DTAG_ENCAPS:	 n = "encapsulation"; break;
	    case CCNL_DTAG_FACEFLAGS:	 n = "faceFlags"; break;
	    case CCNL_DTAG_DEBUGREQUEST: n = "debugRequest"; break;
	    case CCNL_DTAG_DEBUGACTION:	 n = "debugAction"; break;

	    case CCNL_DTAG_FRAGMENT:     n = "fragment"; break;
	    case CCNL_DTAG_FRAG_OSEQN:   n = "fragmentOurSeqNo"; break;
	    case CCNL_DTAG_FRAG_OLOSS:   n = "fragmentOurLoss"; break;
	    case CCNL_DTAG_FRAG_YSEQN:   n = "fragmentYourSeqNo"; break;
	    case CCNL_DTAG_FRAG_FLAGS:   n = "fragmentFlags"; break;

	    case CCNL_DTAG_WIRE:         n = "wire"; break;
	    case CCNL_DTAG_WFRAG_OSEQN:  n = "wireFragOurSeqNo"; break;
	    case CCNL_DTAG_WFRAG_OLOSS:  n = "wireFragOurLoss"; break;
	    case CCNL_DTAG_WFRAG_YSEQN:  n = "wireFragYourSeqNo"; break;
	    case CCNL_DTAG_WFRAG_FLAGS:  n = "wireFragFlags"; break;

	    default:
		n=0;
	    }
	    if (n)
		printf(" -- <%s>\n", n);
	    else
		printf(" -- <unknown tt=%d num=%d>\n", typ, num);
	    if (parse_lev(lev+1, base, buf, len, n) < 0)
		return -1;
	    if (lev == 0) {
		// base = *buf;
		if (*len > 0)
		    printf("\n");
	    }
	    break;
	case 0:
	    if (num == 0) { // end tag
		printf(" -- </%s>\n", here);
		return 0;
	    }
	default:
	    printf("-- tt=%d num=%d not implemented yet\n", typ, num);
	    break;
	}
    }
    return 0;
}


int
main(int argc, char *argv[])
{
    unsigned char data[64*1024], *buf = data;
    int len;

    if (argc > 1) {
	fprintf(stderr, "usage: %s < ccn_encoded_data\n", argv[0]);
	return 2;
    }
    len = read(0, data, sizeof(data));
    if (len < 0) {
	perror("read");
	return 1;
    }
    printf("Parsing %d byte%s:\n\n", len, len!=1 ? "s":"");
    parse_lev(0, data, &buf, &len, "top");

    return 0;
}
