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

#include "../../../../../../ccnx.h"
#include "../../../../../../ccnl.h"

// ----------------------------------------------------------------------

#define SPACING 2

typedef int bool;
#define true 1
#define false 0

bool
dehead(unsigned char **buf,
       int *len, int *num, int *typ, FILE* out)
{
    int i, val = 0;

    if (*len <= 0)
		return false;

	// if current value in buffer is 0 set typ and num to zero, advance one and reduce len
    if (**buf == 0) {
		*num = *typ = 0;
		*buf += 1;
		*len -= 1;
		return true;
    }

    // parse block
    for (i = 0; i < (int)sizeof(i) && i < *len; i++) {
    	// get 1 byte block
		unsigned char c = (*buf)[i];

		// if only the 8th bit is not set, we have a 7bit byte, which can be parsed to a tag
		if ( c & 0x80 ) {
			// parse the tag and return success
		    *num = (val << 4) | ((c >> 3) & 0xf);
		    *typ = c & 0x7;
		    *buf += i+1;
		    *len -= i+1;
		    return true;
		}
		// if the 8th bit was set, shift the current value with 7 bits and add the current char
		val = (val << 7) | c;
    }

    fprintf(stderr, "<native> warning: dehead could not parse everything\n");
    return false;
}

void
print_level(int lev, FILE *out)
{
	int i;
    for(i = 0; i < lev*SPACING; i++) fprintf(out, " ");
    return;
}

void
print_tag_with_attr_size(char *tag_name, int attr_size, int lev, FILE *out)
{
	print_level(lev, out);
	fprintf(out, "<%s size=\"%i\">\n", tag_name, attr_size);
}

void
print_tag_unkown(char *tt, int num, int lev, FILE *out)
{
	print_level(lev, out);
	fprintf(out, "<unkown tt=\"%s\" num=\"%i\">\n", tt, num);
}

void
print_tag(char *tag_name, int lev, FILE *out)
{
	print_level(lev, out);
	fprintf(out, "<%s>\n", tag_name);
}

void
print_closing_tag(char *tag_name, int lev, FILE *out)
{
    if(tag_name != NULL) {
    	print_level(lev-1, out);
    	fprintf(out, "</%s>\n", tag_name);
    } else {
        fprintf(stderr, "<native> warnning: skipped closing tag because it was null\n");
    }
}

bool
is_readable(char c)
{
	return isprint(c) && c != '%';
}

void
print_data(char *data, int num, int lev, FILE *out, bool print_readable)
{
	int i;

	print_tag_with_attr_size("data", num, lev, out);

	print_level(lev + SPACING, out);
	if(print_readable) {
		for(i = 0; i < num; i++) {
		    if (is_readable(data[i]))
				fprintf(out, "%c", data[i]);
		    else
				fprintf(out, "%%%02x", data[i]);
		}
	} else {
		fwrite(data, 1, num, out);
	}
	fprintf(out, "\n");
	print_closing_tag("data", lev + 1, out);
}

char *
ccn_tag_to_str(int tag)
{
    switch (tag) {
	    case CCN_DTAG_ANY:		 		return "any";
	    case CCN_DTAG_NAME:		 		return "name";
	    case CCN_DTAG_COMPONENT:		return "component";
	    case CCN_DTAG_CONTENT:	 		return "content";
	    case CCN_DTAG_SIGNEDINFO:    	return "signedinfo";
	    case CCN_DTAG_INTEREST:	 		return "interest";
	    case CCN_DTAG_KEY:		 		return "key";
	    case CCN_DTAG_KEYLOCATOR:	 	return "keylocator";
	    case CCN_DTAG_KEYNAME:	 		return "keyname";
	    case CCN_DTAG_SIGNATURE:	 	return "signature";
	    case CCN_DTAG_TIMESTAMP:	 	return "timestamp";
	    case CCN_DTAG_TYPE:		 		return "type";
	    case CCN_DTAG_NONCE:	 		return "nonce";
	    case CCN_DTAG_SCOPE:	 		return "scope";
	    case CCN_DTAG_WITNESS:	 		return "witness";
	    case CCN_DTAG_SIGNATUREBITS: 	return "signaturebits";
	    case CCN_DTAG_FRESHNESS:	 	return "freshnessSeconds";
	    case CCN_DTAG_FINALBLOCKID:  	return "finalblockid";
	    case CCN_DTAG_PUBPUBKDIGEST: 	return "publisherPubKeyDigest";
	    case CCN_DTAG_PUBCERTDIGEST: 	return "publisherCertDigest";
	    case CCN_DTAG_CONTENTOBJ:	 	return "contentobj";
	    case CCN_DTAG_ACTION:	 		return "action";
	    case CCN_DTAG_FACEID:	 		return "faceID";
	    case CCN_DTAG_IPPROTO:	 		return "ipProto";
	    case CCN_DTAG_HOST:		 		return "host";
	    case CCN_DTAG_PORT:		 		return "port";
	    case CCN_DTAG_FWDINGFLAGS:	 	return "forwardingFlags";
	    case CCN_DTAG_FACEINSTANCE:	 	return "faceInstance";
	    case CCN_DTAG_FWDINGENTRY:	 	return "forwardingEntry";
	    case CCN_DTAG_MINSUFFCOMP:	 	return "minSuffixComponents";
	    case CCN_DTAG_MAXSUFFCOMP:	 	return "maxSuffixComponents";
	    case CCN_DTAG_SEQNO:	 		return "sequenceNumber";
	    case CCN_DTAG_FragA:	 		return "FragA (Type)";   // frag Type
	    case CCN_DTAG_FragB:	 		return "FragB (SeqNr)";  // frag SeqNr
	    case CCN_DTAG_FragC:	 		return "FragC (Flag)";   // frag h2h Flag
	    case CCN_DTAG_FragD:	 		return "FragD";
	    case CCN_DTAG_FragP:	 		return "FragP (Fragment)";  // frag
	    case CCN_DTAG_CCNPDU:	 		return "ccnProtocolDataUnit";
		case CCNL_DTAG_MACSRC:	 		return "MACsrc";
		case CCNL_DTAG_IP4SRC:	 		return "IP4src";
		case CCNL_DTAG_FRAG:			return "fragmentation";
		case CCNL_DTAG_FACEFLAGS:		return "faceFlags";
		case CCNL_DTAG_DEBUGREQUEST:	return "debugRequest";
		case CCNL_DTAG_DEBUGACTION:		return "debugAction";
		case CCNL_DTAG_FRAG2012_OLOSS:	return "fragmentOurLoss";
	    case CCNL_DTAG_FRAG2012_YSEQN:	return "fragmentYourSeqNo";

	    default: return 0;
    }
}


int
ccnb2xml(int lev, unsigned char *base, unsigned char **buf,
	     int *len, char *cur_tag, FILE *out, bool print_readable)
{
    int num, typ = -1, i, j;
    char *next_tag;

    while (dehead(buf, len, &num, &typ, out)) {
        if(typ == -1) return 0;
		switch (typ) {
		case CCN_TT_BLOB:
		case CCN_TT_UDATA:
		    print_data(*buf, num, lev, out, print_readable);
			*buf += num;
			*len -= num;
		    break;
		case CCN_TT_DTAG:
			next_tag = ccn_tag_to_str(num);
		    if (next_tag)
		    	print_tag(next_tag, lev, out);
			else
				print_tag_unkown(next_tag, num, lev, out);

		    if (ccnb2xml(lev+1, base, buf, len, next_tag, out, print_readable) < 0)
				return -1;

		    break;
		case 0:
		    if (num == 0 || cur_tag != NULL) { // end tag
				print_closing_tag(cur_tag, lev, out);
				return 0;
		    }
		}
    }
    return 0;
}


int
main(int argc, char *argv[])
{
    unsigned char data[64*1024], *buf = data;
    int rc, len, maxlen;

    if (argc > 1) {
	fprintf(stderr, "usage: %s < ccn_encoded_data\n", argv[0]);
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
    ccnb2xml(0, data, &buf, &len, NULL, stdout, true);

    return 0;
}
