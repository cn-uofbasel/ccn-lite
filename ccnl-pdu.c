/*
 * @f util/ccnl-pdu.c
 * @b CCN lite - create CCN protocol data units
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
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
 */

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include "ccnx.h"
#include "ccnl.h"


// ----------------------------------------------------------------------
// (ms): Brought here the following two. I noticed also that some
// of them are replicated elsewhere in the util/ dir. Should we put them
// in one place only ?

int
mkInterest(char **namecomp, unsigned int *nonce, unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
    len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    k = strlen(*namecomp);
    len += mkHeader(out+len, k, CCN_TT_BLOB);
    memcpy(out+len, *namecomp++, k);
    len += k;
    out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name
    if (nonce) {
    len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
    len += mkHeader(out+len, sizeof(unsigned int), CCN_TT_BLOB);
    memcpy(out+len, (void*)nonce, sizeof(unsigned int));
    len += sizeof(unsigned int);
    }
    out[len++] = 0; // end-of-interest

    return len;
}


static int
mkContent(char **namecomp, char *data, int datalen, unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // content
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
    len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    k = strlen(*namecomp);
    len += mkHeader(out+len, k, CCN_TT_BLOB);
    memcpy(out+len, *namecomp++, k);
    len += k;
    out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    len += mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG); // content obj
    len += mkHeader(out+len, datalen, CCN_TT_BLOB);
    memcpy(out+len, data, datalen);
    len += datalen;
    out[len++] = 0; // end-of-content obj

    out[len++] = 0; // end-of-content

    return len;
}

// eof
