/*
 * @f util/ccn-lite-ctrl.c
 * @b control utility to steer a ccn-lite relay via UNIX sockets
 *
 * Copyright (C) 2012-15, Christian Tschudin, University of Basel
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
 * 2012-06-01  created
 * 2013-07     <christopher.scherb@unibas.ch> heavy reworking and parsing
 *             of return message
 */

#include "ccnl-common.h"
#include "ccnl-crypto.h"

// ----------------------------------------------------------------------

size_t
split_string(char *in, char c, char *out)
{

    size_t i = 0, j = 0;
    if (!in[0]) {
        return 0;
    }
    if (in[0] == c) {
        ++i;
    }
    while (in[i] != c) {
        if (in[i] == 0) {
            break;
        }
        out[j] = in[i];
        ++i; ++j;
    }
    out[j] = 0;
    return i;
}

time_t
get_mtime(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        perror(path);
        exit(1);
    }
    return statbuf.st_mtime;
}

int8_t
add_ccnl_name(uint8_t *out, uint8_t *bufend, char *ccn_path, size_t *retlen)
{
    char comp[256];
    size_t len = 0, len2 = 0, h = 0;
    memset(comp, 0 , 256);
    if (ccnl_ccnb_mkHeader(out + len, bufend, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {
        return -1;
    }
    while ( (h = split_string(ccn_path + len2, '/', comp)) ) {
        len2 += h;
        if (ccnl_ccnb_mkStrBlob(out + len, bufend, CCN_DTAG_COMPONENT, CCN_TT_DTAG, comp, &len)) {
            return -1;
        }
        memset(comp, 0 , 256);
    }
    if (out + len + 1 >= bufend) {
        return -1;
    }
    out[len++] = 0;
    *retlen += len;
    return 0;
}

int8_t
mkDebugRequest(uint8_t *out, size_t buflen, char *dbg, char *private_key_path, size_t *retlen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t stmt[1000];
    (void)private_key_path;

    if (ccnl_ccnb_mkHeader(out, out + buflen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {   // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + buflen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "debug", &len1)) {
        return -1;
    }

    // prepare debug statement
    if (ccnl_ccnb_mkHeader(stmt, stmt + sizeof(stmt), CCNL_DTAG_DEBUGREQUEST, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(stmt+len3, stmt + sizeof(stmt), CCN_DTAG_ACTION, CCN_TT_DTAG, "debug", &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(stmt+len3, stmt + sizeof(stmt), CCNL_DTAG_DEBUGACTION, CCN_TT_DTAG, dbg, &len3)) {
        return -1;
    }
    if (len3 + 1 >= sizeof(stmt)) {
        return -1;
    }
    stmt[len3++] = 0; // end-of-debugstmt

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj+sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj+sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                         (char*) stmt, len3, &len2)) {
        return -1;
    }
    if (len2 + 1 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                         (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if(private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= buflen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *retlen += len;
    return 0;
}

int8_t
mkNewEthDevRequest(uint8_t *out, size_t outlen, char *devname, char *ethtype,
           char *frag, char *flags, char *private_key_path, size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t faceinst[2000];
    (void)private_key_path;

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out + len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev", &len1)) {
        return -1;
    }

    // prepare DEVINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst, faceinst + sizeof(faceinst), CCNL_DTAG_DEVINSTANCE, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_ACTION, CCN_TT_DTAG,
                            "newdev", &len3)) {
        return -1;
    }
    if (devname) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_DEVNAME, CCN_TT_DTAG,
                                devname, &len3)) {
            return -1;
        }
    }
    if (ethtype) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCN_DTAG_PORT, CCN_TT_DTAG,
                                ethtype, &len3)) {
            return -1;
        }
    }
    if (frag) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_FRAG, CCN_TT_DTAG,
                                frag, &len3)) {
            return -1;
        }
    }
    if (flags) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG,
                                flags, &len3)) {
            return -1;
        }
    }
    if (len3 + 1 >= sizeof(faceinst)) {
        return -1;
    }
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ,
                           CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                         (char*) faceinst, len3, &len2)) {
        return -1;
    }

    if (len2 + 1 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1+sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                  (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if(private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *reslen += len;
    return 0;
}


int
mkNewUDPDevRequest(uint8_t *out, size_t outlen, char *ip4src, char *ip6src, char *port,
           char *frag, char *flags, char *private_key_path, size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t faceinst[2000];
    (void) private_key_path;

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out + len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev", &len1)) {
        return -1;
    }

    // prepare DEVINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst, faceinst + sizeof(faceinst), CCNL_DTAG_DEVINSTANCE, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_ACTION, CCN_TT_DTAG,
                            "newdev", &len3)) {
        return -1;
    }
    if (ip4src) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_IP4SRC, CCN_TT_DTAG,
                                ip4src, &len3)) {
            return -1;
        }
    }
    if (ip6src) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_IP6SRC, CCN_TT_DTAG,
                                ip6src, &len3)) {
            return -1;
        }
    }
    if (port) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCN_DTAG_PORT, CCN_TT_DTAG,
                                port, &len3)) {
            return -1;
        }
    }
    if (frag) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_FRAG, CCN_TT_DTAG,
                                frag, &len3)) {
            return -1;
        }
    }
    if (flags) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG,
                                flags, &len3)) {
            return -1;
        }
    }
    if (len3 + 1 >= sizeof(faceinst)) {
        return -1;
    }
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst, len3, &len2)) {
        return -1;
    }
    if (len2 + 1 >= sizeof(contentobj)) {
        return  -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                         (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if(private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *reslen += len;
    return 0;
}

int8_t
mkDestroyDevRequest(uint8_t *out, size_t outlen, char *faceid, char *private_key_path, size_t *reslen)
{
    (void) private_key_path;
    (void) faceid;
    (void) out;
    (void) outlen;
//    *reslen = 0;
    (void) reslen;
    return 0;
}

int8_t
mkEchoserverRequest(uint8_t *out, size_t outlen, char *path, int suite,
                    char *private_key_path, size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t fwdentry[2000];
    char suite_s[1];
    char *cp;
    (void) private_key_path;

    DEBUGMSG(DEBUG, "mkEchoserverRequest uri=%s suite=%d\n", path, suite);

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {   // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out + outlen, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out + outlen, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out + outlen, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "echoserver", &len1)) {
        return -1;
    }

    // prepare FWDENTRY
    if (ccnl_ccnb_mkHeader(fwdentry, fwdentry + sizeof(fwdentry), CCN_DTAG_FWDINGENTRY, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(fwdentry+len3, fwdentry + sizeof(fwdentry), CCN_DTAG_ACTION, CCN_TT_DTAG, "echoserver", &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkHeader(fwdentry+len3, fwdentry + sizeof(fwdentry), CCN_DTAG_NAME, CCN_TT_DTAG, &len3)) {  // prefix
        return -1;
    }

    cp = strtok(path, "/");
    while (cp) {
        size_t cmplen_s = strlen(cp);
        if (cmplen_s >= UINT16_MAX) {
            return -1;
        }
        uint16_t cmplen = (uint16_t) cmplen_s;
#ifdef USE_SUITE_CCNTLV
        if (suite == CCNL_SUITE_CCNTLV) {
            char* oldcp = cp;
            cp = malloc( (cmplen + 4) * (sizeof(char)) );
            if (!cp) {
                return -1;
            }
            cp[0] = CCNX_TLV_N_NameSegment >> 8;
            cp[1] = CCNX_TLV_N_NameSegment;
            cp[2] = (char) ((cmplen >> 8) & 0xff);
            cp[3] = (char) (cmplen & 0xff);
            memcpy(cp + 4, oldcp, cmplen);
            cmplen += 4;
        }
#endif
        if (ccnl_ccnb_mkBlob(fwdentry+len3, fwdentry + sizeof(fwdentry), CCN_DTAG_COMPONENT, CCN_TT_DTAG,
                       cp, cmplen, &len3)) {
            if (cp) {
                free(cp);
            }
            return -1;
        }
#ifdef USE_SUITE_CCNTLV
        if (suite == CCNL_SUITE_CCNTLV) {
            free(cp);
        }
#endif
        cp = strtok(NULL, "/");
    }
    if (len3 >= sizeof(fwdentry)) {
        return -1;
    }
    fwdentry[len3++] = 0; // end-of-prefix

    suite_s[0] = suite;
    if (ccnl_ccnb_mkStrBlob(fwdentry+len3, fwdentry + sizeof(fwdentry), CCNL_DTAG_SUITE, CCN_TT_DTAG, suite_s, &len3)) {
        return -1;
    }
    if (len3 >= sizeof(fwdentry)) {
        return -1;
    }
    fwdentry[len3++] = 0; // end-of-fwdentry

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {
        return -1;
    }   // contentobj
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) fwdentry, len3, &len2)) {
        return -1;
    }
    if (len2 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                  (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if (private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    *reslen += len;
    return 0;
}

int8_t
mkNewFaceRequest(uint8_t *out, size_t outlen, char *macsrc, char *ip4src, char *ip6src, char *wpan_addr,
         char *wpan_panid, char *host, char *port, char *flags, char *private_key_path, size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t faceinst[2000];
    (void)private_key_path;

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface", &len1)) {
        return -1;
    }

    // prepare FACEINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst, faceinst + sizeof(faceinst), CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_ACTION, CCN_TT_DTAG, "newface", &len3)) {
        return -1;
    }
    if (macsrc) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_MACSRC, CCN_TT_DTAG, macsrc, &len)) {
            return -1;
        }
    }
    if (ip4src) {
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_IP4SRC, CCN_TT_DTAG, ip4src, &len3)) {
            return -1;
        }
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17", &len3)) {
            return -1;
        }
    }
    if (ip6src) {
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_IP6SRC, CCN_TT_DTAG, ip6src, &len3)) {
            return -1;
        }
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17", &len3)) {
            return -1;
        }
    }
    if (host) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCN_DTAG_HOST, CCN_TT_DTAG, host, &len3)) {
            return -1;
        }
    }
    if (port) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCN_DTAG_PORT, CCN_TT_DTAG, port, &len3)) {
            return -1;
        }
    }
    if (wpan_addr && wpan_panid) {
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_WPANADR, CCN_TT_DTAG, wpan_addr, &len3)) {
            return -1;
        }
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_WPANPANID, CCN_TT_DTAG, wpan_panid, &len3)) {
            return -1;
        }
    }
    /*
    if (frag) {
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_FRAG, CCN_TT_DTAG, frag, &len3)) {
            return -1;
        }
    }
    */
    if (flags) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, flags, &len3)) {
            return -1;
        }
    }
    if (len3 >= sizeof(faceinst)) {
        return -1;
    }
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst, len3, &len2)) {
        return -1;
    }
    if (len2 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                  (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if (private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *reslen += len;
    return 0;
}


int8_t
mkNewUNIXFaceRequest(uint8_t *out, size_t outlen, char *path, char *flags, char *private_key_path, size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t faceinst[2000];
    (void)private_key_path;

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface", &len1)) {
        return -1;
    }

    // prepare FACEINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst, faceinst + sizeof(faceinst), CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_ACTION, CCN_TT_DTAG, "newface", &len3)) {
        return -1;
    }
    if (path) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_UNIXSRC, CCN_TT_DTAG, path, &len3)) {
            return -1;
        }
    }
    /*
    if (frag) {
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_FRAG, CCN_TT_DTAG, frag, &len3)) {
           return -1;
        }
    }
    */
    if (flags) {
        if (ccnl_ccnb_mkStrBlob(faceinst + len3, faceinst + sizeof(faceinst), CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG,
                                flags, &len3)) {
            return -1;
        }
    }
    if (len3 >= sizeof(faceinst)) {
        return -1;
    }
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst, len3, &len2)) {
        return -1;
    }
    if (len2 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                  (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if (private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *reslen += len;
    return 0;
}


int8_t
mkDestroyFaceRequest(uint8_t *out, size_t outlen, char *faceid, char *private_key_path, size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t faceinst[2000];
    (void)private_key_path;
//    char num[20];

//    sprintf(num, "%d", faceID);

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "destroyface", &len1)) {
        return -1;
    }

    // prepare FACEINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst, faceinst + sizeof(faceinst), CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_ACTION, CCN_TT_DTAG, "destroyface", &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_FACEID, CCN_TT_DTAG, faceid, &len3)) {
        return -1;
    }
    if (len3 >= sizeof(faceinst)) {
        return -1;
    }
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst, len3, &len2)) {
        return -1;
    }
    if (len2 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                  (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if (private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *reslen += len;
    return 0;
}


int8_t
mkSetfragRequest(uint8_t *out, size_t outlen, char *faceid, char *frag, char *mtu, char *private_key_path,
                 size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t faceinst[2000];
    (void)private_key_path;
//    char num[20];

//    sprintf(num, "%d", faceID);

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {   // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "setfrag", &len1)) {
        return -1;
    }

    // prepare FACEINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst, faceinst + sizeof(faceinst), CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_ACTION, CCN_TT_DTAG, "setfrag", &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCN_DTAG_FACEID, CCN_TT_DTAG, faceid, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_FRAG, CCN_TT_DTAG, frag, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst + sizeof(faceinst), CCNL_DTAG_MTU, CCN_TT_DTAG, mtu, &len3)) {
        return -1;
    }
    if (len3 >= sizeof(faceinst)) {
        return -1;
    }
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {   // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                             (char*) faceinst, len3, &len2)) {
        return -1;
    }
    if (len2 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                             (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if(private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *reslen += len;
    return 0;
}


// ----------------------------------------------------------------------

int8_t
mkPrefixregRequest(uint8_t *out, size_t outlen, char reg, char *path, char *faceid, int suite ,char *private_key_path,
                   size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[2000];
    uint8_t fwdentry[2000];
    char suite_s[2];
    char *cp;
    (void)private_key_path;

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,
                     reg ? "prefixreg" : "prefixunreg", &len1)) {
        return -1;
    }

    // prepare FWDENTRY
    if (ccnl_ccnb_mkHeader(fwdentry, fwdentry + sizeof(fwdentry), CCN_DTAG_FWDINGENTRY, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(fwdentry+len3, fwdentry + sizeof(fwdentry), CCN_DTAG_ACTION, CCN_TT_DTAG,
                      reg ? "prefixreg" : "prefixunreg", &len3)) {
        return -1;
    }
    if (ccnl_ccnb_mkHeader(fwdentry+len3, fwdentry + sizeof(fwdentry), CCN_DTAG_NAME, CCN_TT_DTAG, &len3)) {  // prefix
        return -1;
    }

    cp = strtok(path, "/");
    while (cp) {
        size_t cmplen_s =  strlen(cp);
        if (cmplen_s > UINT16_MAX) {
            return -1;
        }
        uint16_t cmplen = (uint16_t) cmplen_s;
        if (suite == CCNL_SUITE_CCNTLV) {
            char* oldcp = cp;
            cp = malloc( (cmplen + 4) * (sizeof(char)) );
            if (!cp) {
                return -1;
            }
            cp[0] = CCNX_TLV_N_NameSegment >> 8;
            cp[1] = CCNX_TLV_N_NameSegment;
            cp[2] = (char) ((cmplen >> 8) & 0xff);
            cp[3] = (char) (cmplen & 0xff);
            memcpy(cp + 4, oldcp, cmplen);
            cmplen += 4;
        }
        if (ccnl_ccnb_mkBlob(fwdentry+len3, fwdentry + sizeof(fwdentry), CCN_DTAG_COMPONENT, CCN_TT_DTAG,
                       cp, cmplen, &len3)) {
            free(cp);
            return -1;
        }
        if (suite == CCNL_SUITE_CCNTLV) {
            free(cp);
        }
        cp = strtok(NULL, "/");
    }
    if (len3 + 1 >= sizeof(fwdentry)) {
        return -1;
    }
    fwdentry[len3++] = 0; // end-of-prefix
    if (ccnl_ccnb_mkStrBlob(fwdentry+len3, fwdentry + sizeof(fwdentry), CCN_DTAG_FACEID, CCN_TT_DTAG, faceid, &len3)) {
        return -1;
    }

    suite_s[0] = suite;
    suite_s[1] = 0;
    if (ccnl_ccnb_mkStrBlob(fwdentry+len3, fwdentry + sizeof(fwdentry), CCNL_DTAG_SUITE, CCN_TT_DTAG, suite_s, &len3)) {
        return -1;
    }
    if (len3 + 1 >= sizeof(fwdentry)) {
        return -1;
    }
    fwdentry[len3++] = 0; // end-of-fwdentry

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) fwdentry, len3, &len2)) {
        return -1;
    }
    if (len2 + 1 >= sizeof(contentobj)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                  (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if (private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    *reslen += len;
    return 0;
}

struct ccnl_prefix_s*
getPrefix(uint8_t *data, size_t datalen, int32_t *suite)
{
    struct ccnl_prefix_s *prefix;
    size_t skip;
    struct ccnl_pkt_s *pkt = NULL;

    *suite = ccnl_pkt2suite(data, datalen, &skip);

    if (*suite < 0 || *suite >= CCNL_SUITE_LAST) {
        DEBUGMSG(ERROR, "?unknown packet?\n");
        return 0;
    }
    DEBUGMSG(TRACE, "  suite %s\n", ccnl_suite2str(*suite));
    data += skip;
    datalen -= skip;

    switch(*suite) {
    case CCNL_SUITE_CCNB: {
        uint64_t num;
        uint8_t typ;
        uint8_t *start = data;
        if (!ccnl_ccnb_dehead(&data, &datalen, &num, &typ) && typ == CCN_TT_DTAG)
            pkt = ccnl_ccnb_bytes2pkt(start, &data, &datalen);
        break;
    }
    case CCNL_SUITE_CCNTLV: {
        uint8_t *start = data;
        size_t hdrlen;
        if (ccnl_ccntlv_getHdrLen(data, datalen, &hdrlen)) {
            DEBUGMSG(ERROR, "Error getting header length\n");
            return NULL;
        };

        if (hdrlen > 0) {
            data += hdrlen;
            datalen -= hdrlen;
            pkt = ccnl_ccntlv_bytes2pkt(start, &data, &datalen);
        }
        break;
    }
    case CCNL_SUITE_NDNTLV: {
        uint64_t typ;
        size_t len;
        uint8_t *start = data;
        if (!ccnl_ndntlv_dehead(&data, &datalen, &typ, &len)) {
            pkt = ccnl_ndntlv_bytes2pkt(typ, start, &data, &datalen);
        }
        break;
    }
    default:
        break;
    }
    if (!pkt || !pkt->pfx) {
       DEBUGMSG(ERROR, "Error in prefix extract\n");
       return NULL;
    }
    prefix = ccnl_prefix_dup(pkt->pfx);
    pkt->pfx = NULL;
    ccnl_pkt_free(pkt);
    return prefix;
}

int8_t
mkAddToRelayCacheRequest(uint8_t *out, size_t outlen, char *fname,
                         char *private_key_path, int *suite, size_t *reslen)
{
    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;
    uint8_t *contentobj = NULL, *stmt = NULL, *out1 = NULL, h[12] = {0}, *data = NULL;
    long datalen_l;
    size_t datalen;
    uint8_t chunkflag;
    struct ccnl_prefix_s *prefix = NULL;
    char *prefix_string = NULL;
    int8_t ret = -1;
    (void)private_key_path;

    FILE *file = fopen(fname, "r");
    if (!file) {
        goto Bail;
    }
    //determine size of the file
    fseek(file, 0L, SEEK_END);
    datalen_l = ftell(file);
    if (datalen_l < 0 || (unsigned long) datalen_l > SIZE_MAX) {
        goto Bail;
    }
    datalen = (size_t) datalen_l;
    fseek(file, 0L, SEEK_SET);
    data = (uint8_t *) ccnl_malloc(sizeof(uint8_t)*datalen+1);
    if (!data) {
        goto Bail;
    }
    memset(data, 0, sizeof(uint8_t)*datalen+1);
    fread(data, datalen, 1, file);
    fclose(file);
    file = NULL;

    prefix = getPrefix(data, datalen, suite);
    if (!prefix) {
        DEBUGMSG(ERROR, "  no prefix in file %s\n", fname);
        return -1;
    }
    DEBUGMSG(DEBUG, "  prefix in file: <%s>\n", ccnl_prefix_to_path(prefix));
    prefix_string = ccnl_prefix_to_path_detailed(prefix, 0, 1, 1);

    //Create ccn-lite-ctrl interest object with signature to add content...
    //out = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 5000);
    out1 = (uint8_t*) ccnl_malloc(sizeof(uint8_t) * 5000);
    if (!out1) {
        goto Bail;
    }
    contentobj = (uint8_t*) ccnl_malloc(sizeof(uint8_t) * 4000);
    if (!contentobj) {
        goto Bail;
    }
    stmt = (uint8_t*) ccnl_malloc(sizeof(uint8_t) * 1000);
    if (!stmt) {
        goto Bail;
    }

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        goto Bail;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        goto Bail;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + 5000, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + 5000, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + 5000, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "addcacheobject", &len1)) {
        goto Bail;
    }

    DEBUGMSG(DEBUG, "NAME:%s\n", prefix_string);

    if (ccnl_ccnb_mkStrBlob(stmt+len3, stmt + 1000, CCN_DTAG_COMPONENT, CCN_TT_DTAG, prefix_string, &len3)) {
        goto Bail;
    }


    if (ccnl_ccnb_mkHeader(contentobj+len2, contentobj + 4000, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        goto Bail;
    }

    memset(h, '\0', sizeof(h));
    snprintf((char*)h, sizeof(h), "%d", *suite);
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + 4000, CCNL_DTAG_SUITE, CCN_TT_DTAG,  // suite
                             (char*) h, strlen((char*)h), &len2)) {
        goto Bail;
    }

    if (!prefix->chunknum){
        prefix->chunknum = ccnl_malloc(sizeof(uint32_t));
        if (!prefix->chunknum) {
            goto Bail;
        }
        *prefix->chunknum = 0;
        chunkflag = 0;
    } else {
        chunkflag = 1;
    }

    memset(h, '\0', sizeof(h));
    snprintf((char*)h, sizeof(h), "%d", *prefix->chunknum);
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + 4000, CCNL_DTAG_CHUNKNUM, CCN_TT_DTAG,  // chunknum
                            (char*) h, strlen((char*)h), &len2)) {
        goto Bail;
    }

    memset(h, '\0', sizeof(h));
    snprintf((char*)h, sizeof(h), "%d", chunkflag);
    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + 4000, CCNL_DTAG_CHUNKFLAG, CCN_TT_DTAG,  // chunkflag
                            (char*) h, strlen((char*)h), &len2)) {
        goto Bail;
    }


    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + 4000, CCN_DTAG_NAME, CCN_TT_DTAG,  // content
                             (char*) stmt, len3, &len2)) {
        goto Bail;
    }
    if (len2 + 1 >= 4000) {
        goto Bail;
    }
    contentobj[len2++] = 0; // end-of-contentobj


    if (ccnl_ccnb_mkBlob(out1+len1, out1 + 5000, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                             (char*) contentobj, len2, &len1)) {
        goto Bail;
    }

#ifdef USE_SIGNATURES
    if(private_key_path) {
        len += add_signature(out+len, private_key_path, out1, len1);
    }
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        goto Bail;
    }
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; //name end
    out[len++] = 0; //interest end
    // printf("Contentlen %d\n", len1);

    *reslen += len;

    ret = 0;

Bail:
    if (file) {
        fclose(file);
    }
    if (data) {
        ccnl_free(data);
    }
    if (out1) {
        ccnl_free(out1);
    }
    if (contentobj) {
        ccnl_free(contentobj);
    }
    if (stmt) {
        ccnl_free(stmt);
    }
    if (prefix) {
        ccnl_free(prefix);
    }
    return ret;
}

int8_t
mkRemoveFormRelayCacheRequest(uint8_t *out, size_t outlen, char *ccn_path, char *private_key_path, size_t *reslen){

    size_t len = 0, len1 = 0, len2 = 0, len3 = 0;

    uint8_t out1[CCNL_MAX_PACKET_SIZE];
    uint8_t contentobj[10000];
    uint8_t stmt[2000];
    (void)private_key_path;

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out+len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len1)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len1)) {
        return -1;
    }
    //signatur nach hier, über den rest
    if (ccnl_ccnb_mkStrBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG, "removecacheobject", &len1)) {
        return -1;
    }

    // prepare debug statement
    if (ccnl_ccnb_mkHeader(stmt, stmt + sizeof(stmt), CCN_DTAG_CONTENT, CCN_TT_DTAG, &len3)) {
        return -1;
    }
    if (add_ccnl_name(stmt+len3, stmt + sizeof(stmt), ccn_path, &len3)) {
        return -1;
    }

    if (len3 + 1 >=sizeof(stmt)) {

    }
    stmt[len3++] = 0; // end-of-debugstmt

    // prepare CONTENTOBJ with CONTENT
    if (ccnl_ccnb_mkHeader(contentobj, contentobj + sizeof(contentobj), CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len2)) {  // contentobj
        return -1;
    }

    if (ccnl_ccnb_mkBlob(contentobj+len2, contentobj + sizeof(contentobj), CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                             (char*) stmt, len3, &len2)) {
        return -1;
    }
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    if (ccnl_ccnb_mkBlob(out1+len1, out1 + sizeof(out1), CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
                             (char*) contentobj, len2, &len1)) {
        return -1;
    }

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, out1, len1);
#endif /*USE_SIGNATURES*/
    if (len + len1 + 2 >= outlen) {
        return -1;
    }
    memcpy(out+len, out1, len1);
    len += len1;

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    *reslen += len;
    return 0;

}

// ----------------------------------------------------------------------

int udp_open2(uint16_t port, struct sockaddr_in *si)
{
    int s;
    socklen_t len;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("udp socket");
        return -1;
    }

    si->sin_addr.s_addr = htonl(INADDR_ANY);
    si->sin_port = htons(port);
    si->sin_family = PF_INET;
    if(bind(s, (struct sockaddr *)si, sizeof(*si)) < 0) {
        perror("udp sock bind");
        return -1;
    }
    len = sizeof(*si);
    getsockname(s, (struct sockaddr*) si, &len);
    return s;
}

int
ccnl_crypto_ux_open(char *frompath)
{
    int sock, bufsize;
    struct sockaddr_un name;

    /* Create socket for sending */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        exit(1);
    }
    unlink(frompath);
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, frompath, sizeof(name.sun_path));
    if (bind(sock, (struct sockaddr *) &name,
            sizeof(struct sockaddr_un))) {
        perror("binding name to datagram socket");
        exit(1);
    }
//    printf("socket -->%s\n", NAME);

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return sock;
}

int
udp_sendto2(int sock, char *address, uint16_t port,
           unsigned char *data, size_t len)
{
    ssize_t rc;
    struct sockaddr_in si_other;
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(port);
    if (inet_aton(address, &si_other.sin_addr)==0) {
        DEBUGMSG(ERROR, "inet_aton() failed\n");
        exit(1);
    }
    rc = sendto(sock, data, len, 0, (struct sockaddr*) &si_other,
                sizeof(si_other));
    return rc;
}

int ux_sendto2(int sock, char *topath, unsigned char *data, size_t len)
{
    struct sockaddr_un name;
    ssize_t rc;

    /* Construct name of socket to send to. */
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, topath, sizeof(name.sun_path));

    /* Send message. */
    rc = sendto(sock, data, len, 0, (struct sockaddr*) &name,
        sizeof(struct sockaddr_un));
    if (rc < 0) {
        DEBUGMSG(ERROR, "named pipe \'%s\'\n", topath);
        perror("sending datagram message");
    }
    return rc;
}

int8_t
make_next_seg_debug_interest(int num, uint8_t *out, size_t outlen, size_t *reslen)
{
    size_t len = 0;
    unsigned char cp[100];

    snprintf((char*)cp, sizeof(cp), "seqnum-%d", num);

    if (ccnl_ccnb_mkHeader(out, out + outlen, CCN_DTAG_INTEREST, CCN_TT_DTAG, &len)) {  // interest
        return -1;
    }
    if (ccnl_ccnb_mkHeader(out + len, out + outlen, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out + len, out + outlen, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "mgmt", &len)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out + len, out + outlen, CCN_DTAG_COMPONENT, CCN_TT_DTAG, (char*)cp, &len)) {
        return -1;
    }

    if (len + 2 >= outlen) {
        return -1;
    }
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    *reslen += len;
    return 0;
}

int8_t
handle_ccn_signature(uint8_t **buf, size_t *buflen, char *relay_public_key)
{
    uint64_t num;
    uint8_t typ;
    int8_t verified = 0;
    size_t siglen = 0;
    uint8_t *sigtype = 0, *sig = 0;
    while (ccnl_ccnb_dehead(buf, buflen, &num, &typ) == 0) {

        if (num == 0 && typ == 0) {
            break; // end
        }

        extractStr2(sigtype, CCN_DTAG_NAME);
        siglen = *buflen;
        extractStr2(sig, CCN_DTAG_SIGNATUREBITS);

        if (ccnl_ccnb_consume(typ, num, buf, buflen, 0, 0)) {
            goto Bail;
        }
    }
    siglen = siglen-((*buflen)+4);
#ifdef USE_SIGNATURES
    unsigned char *buf2 = *buf;
    size_t buflen2 = *buflen - 1;
#endif
    if (relay_public_key) {
#ifdef USE_SIGNATURES
        verified = verify(relay_public_key, buf2, buflen2, (unsigned char*)sig, siglen);
#endif
    }
Bail:
    return verified;
}

/**
 * Extract content, verify sig if public key is given as parameter
 * @param len
 * @param buf
 * @return
 */
int8_t
check_has_next(uint8_t *buf, size_t len, char **recvbuffer, size_t *recvbufferlen, char *relay_public_key, int8_t *verified){

    int8_t ret = 1;
    size_t contentlen = 0;
    uint64_t num;
    uint8_t typ;
    char *newbuffer;

    if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
        return 0;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        return 0;
    }

    if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
        return 0;
    }
    if (num == CCN_DTAG_SIGNATURE) {
        if (typ != CCN_TT_DTAG || num != CCN_DTAG_SIGNATURE) {
            return 0;
        }
        *verified = handle_ccn_signature(&buf,&len, relay_public_key);
        if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
            return 0;
        }
    }

    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_FRAG) {
        return 0;
    }

    //check if there is a marker for the last segment
    if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
        return 0;
    }
    if (num == CCN_DTAG_ANY) {
        char buf2[5];
        if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
            return 0;
        }
        memcpy(buf2, buf, num);
        buf2[4] = 0;
        if (!strcmp(buf2, "last")) {
            ret = 0;
        }
        buf += num + 1;
        len -= (num + 1);
        if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
            return 0;
        }
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTDIGEST) {
        return 0;
    }
    if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
        return 0;
    }
    if (typ != CCN_TT_BLOB) {
        return 0;
    }
    contentlen = num;

    newbuffer = realloc(*recvbuffer, (*recvbufferlen+contentlen)*sizeof(char));
    *recvbuffer = newbuffer;
    memcpy(*recvbuffer+*recvbufferlen, buf, contentlen);
    *recvbufferlen += contentlen;

    return ret;
}

int
main(int argc, char *argv[])
{
    char mysockname[200];
    uint8_t *recvbuffer = NULL, *recvbuffer2 = NULL;
    size_t recvbufferlen = 0, recvbufferlen2 = 0;
    char *ux = CCNL_DEFAULT_UNIXSOCKNAME;
    char *udp = "0.0.0.0";
    uint16_t port = 0;
    int8_t use_udp = 0;
    unsigned char out[CCNL_MAX_PACKET_SIZE];
    size_t len;
    int sock = 0;
    int8_t verified_i = 0;
    int8_t verified = 1;
    int numOfParts = 1;
    int8_t msgOnly = 0;
    int suite = CCNL_SUITE_DEFAULT;
    char *file_uri = NULL;
    char *ccn_path;
    char *private_key_path = NULL, *relay_public_key = NULL;
    struct sockaddr_in si;
    int opt, i = 0, ret = -1;
    FILE *f = NULL;
    char *udp_temp = NULL;

    while ((opt = getopt(argc, argv, "hk:mp:v:u:x:")) != -1) {
        switch (opt) {
        case 'k':
            relay_public_key = optarg;
            break;
        case 'm':
            msgOnly = 1;
            break;
        case 'p':
            private_key_path = optarg;
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = (int)strtol(optarg, (char**)NULL, 10);
            else {
                debug_level = ccnl_debug_str2level(optarg);
            }
#endif
            break;
        case 'u':
            udp_temp = strdup(optarg);
            /** strdup failed to allocate memory */
            if (!udp_temp) {
                return -1;
            }

            udp = strtok(udp_temp, "/");
            port = strtol(strtok(NULL, "/"), NULL, 0);
            use_udp = 1;
            printf("udp: <%s> <%i>\n", udp, port);

            free(udp_temp);
            break;
        case 'x':
            ux = optarg;
            break;
        case 'h':
        default:
help:
            fprintf(stderr, "usage: %s [options] CMD ...\n"
       "  [-h] [-k relay-public-key] [-m] [-p private-key]\n"
#ifdef USE_LOGGING
       "  [-v (fatal, error, warning, info, debug, verbose, trace)]\n"
#endif
       "  [-u ip-address/port | -x ux_path]\n"
       "where CMD is either of\n"
       "  newETHdev     DEVNAME [ETHTYPE [FRAG [DEVFLAGS]]]\n"
       "  newUDPdev     IP4SRC|any [PORT [FRAG [DEVFLAGS]]]\n"
       "  newUDP6dev    IP6SRC|any [PORT [FRAG [DEVFLAGS]]]\n"
       "  destroydev    DEVNDX\n"
       "  echoserver    PREFIX [SUITE]\n"
       "  newETHface    MACSRC|any MACDST ETHTYPE [FACEFLAGS]\n"
       "  newUDPface    IP4SRC|any IP4DST PORT [FACEFLAGS]\n"
       "  newWPANface   WPAN_ADDR WPAN_PANID [FACEFLAGS]\n"
       "  newUDP6face   IP6SRC|any IP6DST PORT [FACEFLAGS]\n"
       "  newUNIXface   PATH [FACEFLAGS]\n"
       "  destroyface   FACEID\n"
       "  prefixreg     PREFIX FACEID [SUITE]\n"
       "  prefixunreg   PREFIX FACEID [SUITE]\n"
#ifdef USE_FRAG
       "  setfrag       FACEID FRAG MTU\n"
#endif
       "  debug         dump\n"
       "  debug         halt\n"
       "  debug         dump+halt\n"
       "  addContentToCache             ccn-file\n"
       "  removeContentFromCache        ccn-path\n"
       "where FRAG in one of (none, seqd2012, ccnx2013)\n"
       "      SUITE is one of (ccnb, ccnx2015, ndn2013)\n"
       "-m is a special mode which only prints the interest message of the corresponding command\n",
                    argv[0]);

            if (sock) {
                close(sock);
                unlink(mysockname);
            }
            exit(1);
        }
    }

    if (!argv[optind]) {
        goto help;
    }

    argv += optind-1;
    argc -= optind-1;

    if (!strcmp(argv[1], "debug")) {
        if (argc < 3) {
            goto help;
        }
        if (mkDebugRequest(out, sizeof(out), argv[2], private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "newETHdev")) {
        if (argc < 3) {
            goto help;
        }
        if (mkNewEthDevRequest(out, sizeof(out), argv[2],
                     argc > 3 ? argv[3] : "0x88b5",
                     argc > 4 ? argv[4] : "0",
                     argc > 5 ? argv[5] : "0", private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "newUDPdev")) {
        if (argc < 3) {
            goto help;
        }
        if (mkNewUDPDevRequest(out, sizeof(out), argv[2], NULL,
                 argc > 3 ? argv[3] : "9695",
                 argc > 4 ? argv[4] : "0",
                 argc > 5 ? argv[5] : "0", private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "newUDP6dev")) {
        if (argc < 3) {
            goto help;
        }
        if (mkNewUDPDevRequest(out, sizeof(out), NULL, argv[2],
                 argc > 3 ? argv[3] : "9695",
                 argc > 4 ? argv[4] : "0",
                 argc > 5 ? argv[5] : "0", private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "destroydev")) {
        if (argc < 3) {
            goto help;
        }
        if (mkDestroyDevRequest(out, sizeof(out), argv[2], private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "echoserver")) {
        if (argc > 3) {
            suite = ccnl_str2suite(argv[3]);
            if (!ccnl_isSuite(suite)) {
                goto help;
            }
        }
        if (argc < 3) {
            goto help;
        }
        if (mkEchoserverRequest(out, sizeof(out), argv[2], suite, private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "newETHface")||!strcmp(argv[1],
                "newUDPface")||!strcmp(argv[1], "newUDP6face")) {
        if (argc < 5) {
            goto help;
        }
        if (mkNewFaceRequest(out, sizeof(out),
                       !strcmp(argv[1], "newETHface") ? argv[2] : NULL,
                       !strcmp(argv[1], "newUDPface") ? argv[2] : NULL,
                       !strcmp(argv[1], "newUDP6face") ? argv[2] : NULL,
                       NULL, NULL,
                       argv[3], argv[4],
                       argc > 5 ? argv[5] : "0x0001", private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "newWPANface")) {
        if (argc < 4) {
            goto help;
        }
        if (mkNewFaceRequest(out, sizeof(out),
                NULL, NULL, NULL, argv[2], argv[3], NULL, NULL, argc > 5 ? argv[5] : "0x0001", private_key_path,
                &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "newUNIXface")) {
        if (argc < 3) {
            goto help;
        }
        if (mkNewUNIXFaceRequest(out, sizeof(out), argv[2],
                   argc > 3 ? argv[3] : "0x0001", private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "setfrag")) {
        if (argc < 5) {
            goto help;
        }
        if (mkSetfragRequest(out, sizeof(out), argv[2], argv[3], argv[4], private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "destroyface")) {
        if (argc < 3) {
            goto help;
        }
        if (mkDestroyFaceRequest(out, sizeof(out), argv[2], private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "prefixreg")) {
        if (argc > 4) {
            suite = ccnl_str2suite(argv[4]);
            if (!ccnl_isSuite(suite)) {
                goto help;
            }
        }
        if (argc < 4) {
            goto help;
        }
        if (mkPrefixregRequest(out, sizeof(out), 1, argv[2], argv[3], suite, private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "prefixunreg")) {
        if (argc > 4) {
            suite = (int) strtol(argv[4], (char **) NULL, 10);
        }
        if (argc < 4) {
            goto help;
        }
        if (mkPrefixregRequest(out, sizeof(out), 0, argv[2], argv[3], suite, private_key_path, &len)) {
            goto Bail;
        }
    } else if (!strcmp(argv[1], "addContentToCache")){
        if (argc < 3) {
            goto help;
        }
        file_uri = argv[2];
        if (mkAddToRelayCacheRequest(out, sizeof(out), file_uri, private_key_path, &suite, &len)) {
            goto Bail;
        }
    } else if(!strcmp(argv[1], "removeContentFromCache")){
        if (argc < 3) {
            goto help;
        }
        ccn_path = argv[2];
        if (mkRemoveFormRelayCacheRequest(out, sizeof(out), ccn_path, private_key_path, &len)) {
            goto Bail;
        }
    } else{
        DEBUGMSG(ERROR, "unknown command %s\n", argv[1]);
        goto help;
    }

    if (len > 0 && !msgOnly) {
        socklen_t slen = 0;
        size_t len2 = 0;
        int num = 1;
        int hasNext = 0;

        // socket for receiving
        snprintf(mysockname, sizeof(mysockname), "/tmp/.ccn-light-ctrl-%d.sock", getpid());

        if (!use_udp) {
            sock = ccnl_crypto_ux_open(mysockname);
        } else {
            sock = udp_open2((uint16_t) (getpid() % (UINT16_MAX - 1025) + 1025), &si);
        }
        if (!sock) {
            DEBUGMSG(ERROR, "cannot open UNIX/UDP receive socket\n");
            exit(-1);
        }

        if (!use_udp) {
            ux_sendto2(sock, ux, out, len);
        } else {
            udp_sendto2(sock, udp, port, (uint8_t *) out, len);
        }

//  sleep(1);
        memset(out, 0, sizeof(out));
        if (!use_udp) {
            ssize_t recvlen;
            recvlen = recv(sock, out, sizeof(out), 0);
            if (recvlen < 0) {
                goto Bail;
            }
            len = (size_t) recvlen;
        } else {
            ssize_t recvlen;
            recvlen = recvfrom(sock, out, sizeof(out), 0, (struct sockaddr *) &si, &slen);
            if (recvlen < 0) {
                goto Bail;
            }
            len = (size_t) recvlen;
        }
        hasNext = check_has_next(out, len, (char**)&recvbuffer, &recvbufferlen, relay_public_key, &verified_i);
        if (!verified_i) {
            verified = 0;
        }

        while (hasNext) {
            //send an interest for debug packets... and store content in a array...
            uint8_t interest2[100];
            if (make_next_seg_debug_interest(num++, interest2, sizeof(interest2), &len2)) {
                goto Bail;
            }
            if (!use_udp) {
                ux_sendto2(sock, ux, interest2, len2);
            } else {
                udp_sendto2(sock, udp, port, interest2, len2);
            }
            memset(out, 0, sizeof(out));
            if (!use_udp) {
                ssize_t recvlen;
                recvlen = recv(sock, out, sizeof(out), 0);
                if (recvlen < 0) {
                    goto Bail;
                }
                len = (size_t) recvlen;
            } else {
                ssize_t recvlen;
                recvlen = recvfrom(sock, out, sizeof(out), 0,
                               (struct sockaddr *) &si, &slen);
                if (recvlen < 0) {
                    goto Bail;
                }
                len = (size_t) recvlen;
            }
            hasNext =  check_has_next(out+2, len-2, (char**)&recvbuffer,
                                 &recvbufferlen, relay_public_key, &verified_i);
            if (!verified_i) {
                verified = 0;
            }
            ++numOfParts;
        }
        recvbuffer2 = malloc(sizeof(char) * recvbufferlen +1000);
        if (!recvbuffer2) {
            goto Bail;
        }
        if (ccnl_ccnb_mkHeader(recvbuffer2+recvbufferlen2, recvbuffer2 + sizeof(char) * recvbufferlen +1000,
                               CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &recvbufferlen2)) {
            goto Bail;
        }
        if (relay_public_key && use_udp) {
            char sigoutput[200];

            if (verified) {
                snprintf(sigoutput, sizeof(sigoutput), "All parts (%d) have been verified", numOfParts);
                if (ccnl_ccnb_mkStrBlob(recvbuffer2+recvbufferlen2, recvbuffer2 + sizeof(char) * recvbufferlen +1000,
                                        CCN_DTAG_SIGNATURE, CCN_TT_DTAG, sigoutput, &recvbufferlen2)) {
                    goto Bail;
                }
            } else {
                snprintf(sigoutput, sizeof(sigoutput), "NOT all parts (%d) have been verified", numOfParts);
                if (ccnl_ccnb_mkStrBlob(recvbuffer2+recvbufferlen2, recvbuffer2 + sizeof(char) * recvbufferlen +1000,
                                        CCN_DTAG_SIGNATURE, CCN_TT_DTAG, sigoutput, &recvbufferlen2)) {
                    goto Bail;
                }
            }
        }
        memcpy(recvbuffer2+recvbufferlen2, recvbuffer, recvbufferlen);
        recvbufferlen2+=recvbufferlen;
        recvbuffer2[recvbufferlen2++] = 0; //end of content

        write(1, recvbuffer2, recvbufferlen2);
        printf("\n");

        DEBUGMSG(DEBUG, "received %zu bytes.\n", recvbufferlen);

        if (!strcmp(argv[1], "addContentToCache")) { //read ccnb_file
            unsigned char *ccnb_file;
            long fsize = 0;
            f = fopen(file_uri, "r");
            if (!f) {
                goto Bail;
            }
            //determine size of the file
            errno = 0;
            fseek(f, 0L, SEEK_END);
            if (errno) {
                goto Bail;
            }
            fsize = ftell(f);
            if (errno || fsize < 0 || (unsigned long) fsize > SIZE_MAX) {
                goto Bail;
            }
            fseek(f, 0L, SEEK_SET);
            if (errno) {
                goto Bail;
            }
            ccnb_file = (unsigned char *) malloc(sizeof(unsigned char)*fsize);
            if (!ccnb_file) {
                goto Bail;
            }
            if (fread(ccnb_file, (size_t) fsize, 1, f)) {
                goto Bail;
            }
            fclose(f);
            f = NULL;

            //receive request
            memset(out, 0, sizeof(out));
            if (!use_udp) {
                ssize_t recvlen;
                recvlen = recv(sock, out, sizeof(out), 0);
                if (recvlen < 0) {
                    DEBUGMSG(ERROR, "an error occurred on receiving data\n");
                    goto Bail;
                }
                len = (size_t) recvlen;
            } else {
                ssize_t recvlen;
                recvlen = recvfrom(sock, out, sizeof(out), 0,
                               (struct sockaddr *) &si, &slen);
                if (recvlen < 0) {
                    DEBUGMSG(ERROR, "an error occurred on receiving data\n");
                    goto Bail;
                }
                len = (size_t) recvlen;
            }

            //send file
            if (!use_udp) {
                i = ux_sendto2(sock, ux, ccnb_file, fsize);
            } else {
                i = udp_sendto2(sock, udp, port, ccnb_file, fsize);
            }

            if (i) {
                DEBUGMSG(INFO, "Sent file to relay\n");
            }
            free(ccnb_file);
        }
    } else if(msgOnly) {
        fwrite(out, len, 1, stdout);
    } else {
        DEBUGMSG(ERROR, "nothing to send, program terminates\n");
    }

    ret = 0;

Bail:
    if (f) {
        fclose(f);
    }
    free(recvbuffer2);
    free(recvbuffer);
    close(sock);
    unlink(mysockname);

    return ret;
}
