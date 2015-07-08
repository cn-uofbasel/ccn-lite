/*
 * @f ccnl-ext-hmac.c
 * @b HMAC-256 signing support
 *
 * Copyright (C) 2015 <christian.tschudin@unibas.ch>
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
 * 2015-05-08 created
 */

#ifdef USE_HMAC256

#include "lib-sha256.c"

// RFC2014 keyval generation
void
ccnl_hmac256_keyval(unsigned char *key, int klen,
                    unsigned char *keyval) // MUST have 64 bytes (BLOCK_LENGTH)
{
    DEBUGMSG(TRACE, "ccnl_hmac256_keyval %d bytes\n", klen);

    if (klen <= SHA256_BLOCK_LENGTH) {
        memcpy(keyval, key, klen);
    } else {
        SHA256_CTX_t ctx;

        ccnl_SHA256_Init(&ctx);
        ccnl_SHA256_Update(&ctx, key, klen);
        ccnl_SHA256_Final(keyval, &ctx);
        klen = SHA256_DIGEST_LENGTH;
    }
    memset(keyval + klen, 0, SHA256_BLOCK_LENGTH - klen);
}

void
ccnl_hmac256_keyid(unsigned char *key, int klen,
                   unsigned char *keyid) // MUST have 32 bytes (DIGEST_LENGTH)
{
    SHA256_CTX_t ctx;

    DEBUGMSG(TRACE, "ccnl_hmac256_keyid %d bytes\n", klen);

    ccnl_SHA256_Init(&ctx);
    ccnl_SHA256_Update(&ctx, key, klen);

    if (klen > SHA256_BLOCK_LENGTH) {
        unsigned char md[32];
        ccnl_SHA256_Final(md, &ctx);
        ccnl_SHA256_Init(&ctx);
        ccnl_SHA256_Update(&ctx, md, sizeof(md));
    }

    ccnl_SHA256_Final(keyid, &ctx);
}

// internal
void
ccnl_hmac256_keysetup(SHA256_CTX_t *ctx, unsigned char *keyval, int kvlen,
                      unsigned char pad)
{
    unsigned char buf[64];
    int i;

    if (kvlen > sizeof(buf))
        kvlen = sizeof(buf);
    for (i = 0; i < kvlen; i++, keyval++)
        buf[i] = *keyval ^ pad;
    while (i < sizeof(buf))
        buf[i++] = 0 ^ pad;

    ccnl_SHA256_Init(ctx);
    ccnl_SHA256_Update(ctx, buf, sizeof(buf));
}

// RFC2014 signature generation
void
ccnl_hmac256_sign(unsigned char *keyval, int kvlen,
                  unsigned char *data, int dlen,
                  unsigned char *md, int *mlen)
{
    unsigned char tmp[SHA256_DIGEST_LENGTH];
    SHA256_CTX_t ctx;

    DEBUGMSG(TRACE, "ccnl_hmac_sign %d bytes\n", dlen);

    ccnl_hmac256_keysetup(&ctx, keyval, kvlen, 0x36); // inner hash
    ccnl_SHA256_Update(&ctx, data, dlen);
    ccnl_SHA256_Final(tmp, &ctx);

    ccnl_hmac256_keysetup(&ctx, keyval, kvlen, 0x5c); // outer hash
    ccnl_SHA256_Update(&ctx, tmp, sizeof(tmp));
    ccnl_SHA256_Final(tmp, &ctx);

    if (*mlen > SHA256_DIGEST_LENGTH)
        *mlen = SHA256_DIGEST_LENGTH;
    memcpy(md, tmp, *mlen);
}

#ifdef NEEDS_PACKET_CRAFTING

#ifdef USE_SUITE_CCNTLV

// write Content packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_prependSignedContentWithHdr(struct ccnl_prefix_s *name,
                                        unsigned char *payload, int paylen,
                                        unsigned int *lastchunknum,
                                        int *contentpos,
                                        unsigned char *keyval, // 64B
                                        unsigned char *keydigest, // 32B
                                        int *offset, unsigned char *buf)
{
    int mdlength = 32, mdoffset, endofsign, oldoffset;
    uint32_t len;
    unsigned char hoplimit = 255; // setting to max (conten obj has no hoplimit)

    if (*offset < (8 + paylen + 4+32 + 3*4+32))
        return -1;

    oldoffset = *offset;

    *offset -= mdlength; // reserve space for the digest
    mdoffset = *offset;
    ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationPayload, mdlength, offset, buf);
    endofsign = *offset;
#ifdef XXX // we skip this
    *offset -= 32;
    memcpy(buf + *offset, keydigest, 32);
    ccnl_ccntlv_prependTL(CCNX_VALIDALGO_KEYID, 32, offset, buf);
    ccnl_ccntlv_prependTL(CCNX_VALIDALGO_HMAC_SHA256, 4+32, offset, buf);
    ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationAlgo, 4+4+32, offset, buf);
#endif
    ccnl_ccntlv_prependTL(CCNX_VALIDALGO_HMAC_SHA256, 0, offset, buf);
    ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationAlgo, 4, offset, buf);

    len = oldoffset - *offset;
    len += ccnl_ccntlv_prependContent(name, payload, paylen, lastchunknum,
                                      contentpos, offset, buf);
    if (len >= (((uint32_t)1 << 16) - 8)) {
        DEBUGMSG(ERROR, "payload to sign is too large\n");
        return -1;
    }

    ccnl_hmac256_sign(keyval, 64, buf + *offset, endofsign - *offset,
                      buf + mdoffset, &mdlength);
    ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                len, hoplimit, offset, buf);
    return oldoffset - *offset;
}

#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_NDNTLV

int
ccnl_ndntlv_prependSignedContent(struct ccnl_prefix_s *name,
                           unsigned char *payload, int paylen,
                           unsigned int *final_block_id, int *contentpos,
                           unsigned char *keyval, // 64B
                           unsigned char *keydigest, // 32B
                           int *offset, unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2, mdoffset, endofsign, mdlength = 32;
    unsigned char signatureType[1] = { NDN_SigTypeVal_SignatureHmacWithSha256 };

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards

    *offset -= mdlength; // sha256 msg digest bits, filled out later
    mdoffset = *offset;
    // mandatory
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureValue, mdlength, offset, buf) <0)
        return -1;

    // to find length from start of content to end of SignatureInfo
    endofsign = *offset;

#ifdef XXX // we skip this
    // keyid
    *offset -= 32;
    memcpy(buf + *offset, keydigest, 32);
    if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocatorDigest, 32, offset, buf) < 0)
        return -1;
    if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocator, 32+2, offset, buf) < 0)
        return -1;
#endif

    // use NDN_SigTypeVal_SignatureHmacWithSha256
    if (ccnl_ndntlv_prependBlob(NDN_TLV_SignatureType, signatureType, 1,
                offset, buf)< 0)
        return 1;

    // Groups KeyLocator and Signature Type with stored len
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureInfo, endofsign - *offset, offset, buf) < 0)
        return -1;

    // mandatory payload/content
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
                                offset, buf) < 0)
        return -1;

    // to find length of optional MetaInfo fields
    oldoffset2 = *offset;
    if(final_block_id) {
        if (ccnl_ndntlv_prependIncludedNonNegInt(NDN_TLV_NameComponent,
                                                 *final_block_id,
                                                 NDN_Marker_SegmentNumber,
                                                 offset, buf) < 0)
            return -1;

        // optional
        if (ccnl_ndntlv_prependTL(NDN_TLV_FinalBlockId, oldoffset2 - *offset, offset, buf) < 0)
            return -1;
    }

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_MetaInfo, oldoffset2 - *offset, offset, buf) < 0)
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependName(name, offset, buf))
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
                              offset, buf) < 0)
           return -1;

    if (contentpos)
        *contentpos -= *offset;

    ccnl_hmac256_sign(keyval, 64, buf + *offset, endofsign - *offset,
                      buf + mdoffset, &mdlength);

    return oldoffset - *offset;
}

#endif // USE_SUITE_NDNTLV

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_HMAC256
