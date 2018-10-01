#include "ccnl-ext-hmac.h"

#ifdef USE_HMAC256

// RFC2104 keyval generation
void
ccnl_hmac256_keyval(uint8_t *key, size_t klen,
                    uint8_t *keyval) // MUST have 64 bytes (BLOCK_LENGTH)
{
    DEBUGMSG(TRACE, "ccnl_hmac256_keyval %zu bytes\n", klen);

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
ccnl_hmac256_keyid(uint8_t *key, size_t klen,
                   uint8_t *keyid) // MUST have 32 bytes (DIGEST_LENGTH)
{
    SHA256_CTX_t ctx;

    DEBUGMSG(TRACE, "ccnl_hmac256_keyid %zu bytes\n", klen);

    ccnl_SHA256_Init(&ctx);
    ccnl_SHA256_Update(&ctx, key, klen);

    if (klen > SHA256_BLOCK_LENGTH) {
        uint8_t md[32];
        ccnl_SHA256_Final(md, &ctx);
        ccnl_SHA256_Init(&ctx);
        ccnl_SHA256_Update(&ctx, md, sizeof(md));
    }

    ccnl_SHA256_Final(keyid, &ctx);
}

// internal
void
ccnl_hmac256_keysetup(SHA256_CTX_t *ctx, uint8_t *keyval, size_t kvlen,
                      uint8_t pad)
{
    uint8_t buf[64];
    size_t i;

    if (kvlen > sizeof(buf)) {
        kvlen = sizeof(buf);
    }
    for (i = 0; i < kvlen; i++, keyval++) {
        buf[i] = *keyval ^ pad;
    }
    while (i < sizeof(buf)) {
        buf[i++] = (uint8_t) (0 ^ pad); //TODO: WTF?
    }

    ccnl_SHA256_Init(ctx);
    ccnl_SHA256_Update(ctx, buf, sizeof(buf));
}

// RFC2104 signature generation
void
ccnl_hmac256_sign(uint8_t *keyval, size_t kvlen,
                  uint8_t *data, size_t dlen,
                  uint8_t *md, size_t *mlen)
{
    uint8_t tmp[SHA256_DIGEST_LENGTH];
    SHA256_CTX_t ctx;

    DEBUGMSG(TRACE, "ccnl_hmac_sign %zu bytes\n", dlen);

    ccnl_hmac256_keysetup(&ctx, keyval, kvlen, 0x36); // inner hash
    ccnl_SHA256_Update(&ctx, data, dlen);
    ccnl_SHA256_Final(tmp, &ctx);

    ccnl_hmac256_keysetup(&ctx, keyval, kvlen, 0x5c); // outer hash
    ccnl_SHA256_Update(&ctx, tmp, sizeof(tmp));
    ccnl_SHA256_Final(tmp, &ctx);

    if (*mlen > SHA256_DIGEST_LENGTH) {
        *mlen = SHA256_DIGEST_LENGTH;
    }
    memcpy(md, tmp, *mlen);
}

#ifdef NEEDS_PACKET_CRAFTING

#ifdef USE_SUITE_CCNTLV

// write Content packet *before* buf[offs], adjust offs and return bytes used
int8_t
ccnl_ccntlv_prependSignedContentWithHdr(struct ccnl_prefix_s *name,
                                        uint8_t *payload, size_t paylen,
                                        uint32_t *lastchunknum,
                                        size_t *contentpos,
                                        uint8_t *keyval, // 64B
                                        uint8_t *keydigest, // 32B
                                        size_t *offset, uint8_t *buf, size_t *retlen)
{
    size_t mdlength = 32, mdoffset, endofsign, oldoffset, len;
    uint8_t hoplimit = 255; // setting to max (conten obj has no hoplimit)
    (void)keydigest;

    if (*offset < (8 + paylen + 4+32 + 3*4+32)) {
        return -1;
    }

    oldoffset = *offset;

    *offset -= mdlength; // reserve space for the digest
    mdoffset = *offset;
    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationPayload, mdlength, offset, buf)) {
        return -1;
    }
    endofsign = *offset;
#ifdef XXX // we skip this
    *offset -= 32;
    memcpy(buf + *offset, keydigest, 32);
    if (ccnl_ccntlv_prependTL(CCNX_VALIDALGO_KEYID, 32, offset, buf)) {
        return -1;
    }
    if (ccnl_ccntlv_prependTL(CCNX_VALIDALGO_HMAC_SHA256, 4+32, offset, buf)) {
        return -1;
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationAlgo, 4+4+32, offset, buf)) {
        return -1;
    }
#endif
    if (ccnl_ccntlv_prependTL(CCNX_VALIDALGO_HMAC_SHA256, 0, offset, buf)) {
        return -1;
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationAlgo, 4, offset, buf)) {
        return -1;
    }

    len = oldoffset - *offset;
    if (ccnl_ccntlv_prependContent(name, payload, paylen, lastchunknum,
                                   contentpos, offset, buf, &len)) {
        return -1;
    }
    if (len > (UINT16_MAX - 8)) {
        DEBUGMSG(ERROR, "payload to sign is too large\n");
        return -1;
    }

    ccnl_hmac256_sign(keyval, 64, buf + *offset, endofsign - *offset,
                      buf + mdoffset, &mdlength);
    if (ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                    len, hoplimit, offset, buf)) {
        return -1;
    }
    *retlen = oldoffset - *offset;
    return 0;
}

#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_NDNTLV

int8_t
ccnl_ndntlv_prependSignedContent(struct ccnl_prefix_s *name,
                                 uint8_t *payload, size_t paylen,
                                 uint32_t *final_block_id, size_t *contentpos,
                                 uint8_t *keyval, // 64B
                                 uint8_t *keydigest, // 32B
                                 size_t *offset, uint8_t *buf, size_t *reslen) {
    size_t mdlength = 32;
    size_t oldoffset = *offset, oldoffset2, mdoffset, endofsign;
    uint8_t signatureType[1] = {NDN_SigTypeVal_SignatureHmacWithSha256};
    (void) keydigest;
    if (contentpos) {
        *contentpos = *offset - paylen;
    }

    // fill in backwards

    *offset -= mdlength; // sha256 msg digest bits, filled out later
    mdoffset = *offset;
    // mandatory
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureValue, (uint64_t) mdlength, offset, buf)) {
        return -1;
    }

    // to find length from start of content to end of SignatureInfo
    endofsign = *offset;

#ifdef XXX // we skip this
    // keyid
    *offset -= 32;
    memcpy(buf + *offset, keydigest, 32);
    if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocatorDigest, 32, offset, buf)) {
        return -1;
    }
    if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocator, 32+2, offset, buf)) {
        return -1;
    }
#endif

    // use NDN_SigTypeVal_SignatureHmacWithSha256
    if (ccnl_ndntlv_prependBlob(NDN_TLV_SignatureType, signatureType, 1,
                                offset, buf)) {
        return 1;
    }

    // Groups KeyLocator and Signature Type with stored len
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureInfo, endofsign - *offset, offset, buf)) {
        return -1;
    }

    // mandatory payload/content
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
                                offset, buf)) {
        return -1;
    }

    // to find length of optional MetaInfo fields
    oldoffset2 = *offset;
    if (final_block_id) {
        if (ccnl_ndntlv_prependIncludedNonNegInt(NDN_TLV_NameComponent,
                                                 *final_block_id,
                                                 NDN_Marker_SegmentNumber,
                                                 offset, buf)) {
            return -1;
        }
        // optional
        if (ccnl_ndntlv_prependTL(NDN_TLV_FinalBlockId, oldoffset2 - *offset, offset, buf)) {
            return -1;
        }
    }

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_MetaInfo, oldoffset2 - *offset, offset, buf)) {
        return -1;
    }

    // mandatory
    if (ccnl_ndntlv_prependName(name, offset, buf)) {
        return -1;
    }

    // mandatory
    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
                              offset, buf)) {
        return -1;
    }

    if (contentpos) {
        *contentpos -= *offset;
    }

    ccnl_hmac256_sign(keyval, 64, buf + *offset, (endofsign - *offset),
                      buf + mdoffset, &mdlength);

    *reslen = oldoffset - *offset;
    return 0;
}

#endif // USE_SUITE_NDNTLV

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_HMAC256
