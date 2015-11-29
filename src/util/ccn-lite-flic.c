/*
 * @f util/ccn-lite-flic.c
 * @b utility for FLIC (FLIC = File-Like-ICN-Collection)
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-11-10  created
 */

#define USE_SUITE_CCNTLV
// #define USE_SUITE_CISTLV
// #define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV

#define USE_HMAC256
#define USE_IPV4
#define USE_NAMELESS
#define USE_UNIXSOCKET

#define MAX_BLOCK_SIZE (64*1024)

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"
#include "ccnl-socket.c"

// ----------------------------------------------------------------------

char *progname;

void
usage(int exitval)
{
    fprintf(stderr, "usage: %s -p DIRPATH [options] FILE [URI]  (producing)\n"
                    "       %s [options] URI                    (retrieving)\n"
            "  -b SIZE    Block size (default is 4K)\n"
            "  -d DIGEST  ObjHashRestriction (32B in hex)\n"
            "  -e N       0=drop-if-nosig-or-inval, 1=warn-if-nosig-or-inval, 2=warn-inval\n"
            "  -k FNAME   HMAC256 key (base64 encoded)\n"
            "  -m URI     URI of external metadata\n"
            "  -o FNAME   outfile\n"
            "  -s SUITE   (ccnx2015, ndn2013)\n"
            "  -u a.b.c.d/port  UDP destination\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL   (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
            "  -x PATH    use UNIX IPC instead od UDP\n"
            , progname, progname);
    exit(exitval);
}

// ----------------------------------------------------------------------

struct list_s { // s-expr
    char typ;
    unsigned char *var;
    int varlen;
    struct list_s *first;
    struct list_s *rest;
};

enum {
    M_MANIFESTMSG,
    M_NAME,
    M_HASHGROUP,
    M_HG_METADATA,
    M_HG_PTR2DATA,
    M_HG_PTR2MANIFEST,
    M_MT_LOCATOR,
    M_MT_EXTERNALNAME,
    M_MT_BLOCKSIZE,
    M_MT_OVERALLDATASIZE,
    M_MT_OVERALLDATASHA256,
    M_SIGINFO,
    M_SIGVAL,
};

#ifdef USE_SUITE_CCNTLV
unsigned short ccntlv_m_codes[] = {
    CCNX_TLV_TL_Manifest,
    CCNX_TLV_M_Name,
    CCNX_MANIFEST_HASHGROUP,
    CCNX_MANIFEST_HG_METADATA,
    CCNX_MANIFEST_HG_PTR2DATA,
    CCNX_MANIFEST_HG_PTR2MANIFEST,
    CCNX_MANIFEST_MT_LOCATOR,
    CCNX_MANIFEST_MT_EXTERNALMETADATA,
    CCNX_MANIFEST_MT_BLOCKSIZE,
    CCNX_MANIFEST_MT_OVERALLDATASIZE,
    CCNX_MANIFEST_MT_OVERALLDATASHA256,
    0,
    0
};
#endif

#ifdef USE_SUITE_NDNTLV
unsigned short ndntlv_m_codes[] = {
    NDN_TLV_Manifest,
    NDN_TLV_Name,
    NDN_TLV_MANIFEST_HASHGROUP,
    NDN_TLV_MANIFEST_HG_METADATA,
    NDN_TLV_MANIFEST_HG_PTR2DATA,
    NDN_TLV_MANIFEST_HG_PTR2MANIFEST,
    NDN_TLV_MANIFEST_MT_LOCATOR,
    NDN_TLV_MANIFEST_MT_EXTERNALMETADATA,
    NDN_TLV_MANIFEST_MT_BLOCKSIZE,
    NDN_TLV_MANIFEST_MT_OVERALLDATASIZE,
    NDN_TLV_MANIFEST_MT_OVERALLDATASHA256,
    NDN_TLV_SignatureInfo,
    NDN_TLV_SignatureValue
};
#endif

int theSuite = CCNL_SUITE_DEFAULT;
unsigned short *m_codes;

int (*flic_prependTL)(unsigned short type, unsigned short len,
                      int *offset, unsigned char *buf);
int (*flic_prependUInt)(unsigned int intval,int *offset, unsigned char *buf);
int (*flic_prependNameComponents)(struct ccnl_prefix_s *name,
                                  int *offset, unsigned char *buf);
int (*flic_prependContent)(struct ccnl_prefix_s *name, unsigned char *payload,
                           int paylen, unsigned int *lastchunknum,
                           int *contentpos, int *offset, unsigned char *buf);
int (*flic_prependFixedHdr)(unsigned char ver, unsigned char packettype,
                            unsigned short payloadlen, unsigned char hoplimit,
                            int *offset, unsigned char *buf);
int (*flic_dehead)(unsigned char **buf, int *len,unsigned int *typ,int *vallen);
struct ccnl_pkt_s* (*flic_bytes2pkt)(unsigned char *start,
                                     unsigned char **data, int *datalen);
int (*flic_mkInterest)(struct ccnl_prefix_s *name, int *dummy,
                 unsigned char *objHashRestr, unsigned char *out, int outlen);

// ----------------------------------------------------------------------

struct emit_s {
    char sym;
    int codeNdx;
    char *debug;
} translate[] = {
    { 'a', M_HG_METADATA,           "T=about (metadata)" },
    { 'b', M_MT_BLOCKSIZE,          "T=meta block size" },
    { 'd', M_HG_PTR2DATA,           "T=data hash ptr" },
    { 'g', M_HASHGROUP,             "T=hash pointer group" },
    { 'h', M_MT_OVERALLDATASHA256,  "T=meta overall hash" },
    { 'i', M_SIGINFO,               "T=signature info" },
    { 'l', M_MT_LOCATOR,            "T=meta locator" },
    { 'm', M_MANIFESTMSG,           "T=manifest" },
    { 'n', M_NAME,                  "T=name" },
    { 's', M_MT_OVERALLDATASIZE,    "T=meta overall data size" },
    { 't', M_HG_PTR2MANIFEST,       "T=tree hash ptr" },
    { 'v', M_SIGVAL,                "T=signature value" },
    { 'x', M_MT_EXTERNALNAME,       "T=external metadata URI" }
};

int
emit(struct list_s *lst, unsigned short len,
     int *offset, unsigned char *buf)
{
    int len2;

    if (!lst)
        return len;

    DEBUGMSG(TRACE, "   emit starts: len=%d\n", len);

    if (lst->typ) { // atomic
        char *cp = NULL;
        unsigned i;

        for (i = 0; i < sizeof(translate)/sizeof(struct emit_s); i++) {
            if (lst->typ == translate[i].sym) {
                cp = translate[i].debug;
                len += flic_prependTL(m_codes[translate[i].codeNdx],
                                      len, offset, buf);
                break;
            } else if (lst->typ == '@') {
                cp = "blob";
                len = lst->varlen;
                *offset -= len;
                memcpy(buf + *offset, lst->var, len);
                break;
            }
        }
        if (!cp) {
            DEBUGMSG(FATAL, "unknown type '%c'\n", lst->typ);
        } else {
            DEBUGMSG(VERBOSE, "  prepending %s, returning %d Bytes\n", cp, len);
        }
        return len;
    }

    // non-atomic
    len2 = emit(lst->rest, 0, offset, buf);
    DEBUGMSG(TRACE, "   rest: len2=%d, len=%d\n", len2, len);
    return emit(lst->first, len2, offset, buf) + len;
}

/*
void
ccnl_printExpr(struct list_s *t, char last, char *out)
{
    if (!t)
        return;
    if (t->typ) {
        if (t->varlen)
            sprintf(out, "atomic %c (%d bytes)\n", t->typ, t->varlen);
        else
            sprintf(out, "atomic %c\n", t->typ);
        return;
    }
    strcat(out, "(");
    last = '(';
    do {
        if (last != '(')
            strcat(out, " ");
        out += strlen(out);
        ccnl_printExpr(t->first, last, out);
        out += strlen(out);
        last = 'a';
        t = t->rest;
    } while (t);
    strcat(out, ")");
    return;
}
*/

// ----------------------------------------------------------------------
// manifest API

struct list_s*
ccnl_manifest_atomic(char *var)
{
    struct list_s *node;

    node = ccnl_calloc(1, sizeof(*node));
    node->first = ccnl_calloc(1, sizeof(*node));
    node->first->typ = var[0];

    return node;
}

struct list_s*
ccnl_manifest_atomic_blob(unsigned char *blob, int len)
{
    struct list_s *node;

    node = ccnl_calloc(1, sizeof(*node));
    node->first = ccnl_calloc(1, sizeof(*node));
    node->first->typ = '@';
    node->first->var = ccnl_malloc(len);
    memcpy(node->first->var, blob, len);
    node->first->varlen = len;

    return node;
}

struct list_s*
ccnl_manifest_atomic_uint(unsigned int val)
{
    unsigned char tmp[16];
    int offs = sizeof(tmp), len;

    len = flic_prependUInt(val, &offs, tmp);
    return ccnl_manifest_atomic_blob(tmp + offs, len);
}

struct list_s*
ccnl_manifest_atomic_prefix(struct ccnl_prefix_s *pfx)
{
    static unsigned char dummy[256];
    int offset, len;

    offset = sizeof(dummy);
    len = flic_prependNameComponents(pfx, &offset, dummy);
    if (len < 0)
        return 0;

    return ccnl_manifest_atomic_blob(dummy + offset, len);
}

void
ccnl_manifest_append(struct list_s *where, char *typ, struct list_s *what)
{
    struct list_s **epp, *elmnt = ccnl_calloc(1, sizeof(*elmnt));

    elmnt->first = ccnl_manifest_atomic(typ);
    elmnt->first->rest = what;
    for (epp = &where->rest; *epp; epp = &(*epp)->rest);
    *epp = elmnt;
}

void
ccnl_manifest_prepend(struct list_s *where, char *typ, struct list_s *what)
{
    struct list_s *elmnt = ccnl_calloc(1, sizeof(*elmnt));

    elmnt->first = ccnl_manifest_atomic(typ);
    elmnt->first->rest = what;
    elmnt->rest = where->rest;
    where->rest = elmnt;
}

struct list_s*
ccnl_manifest_getEmptyTemplate()
{
    struct list_s *m;

    m = ccnl_manifest_atomic("mfst");
    ccnl_manifest_prepend(m, "grp", NULL);

    return m;
}

// typ is either "dptr" or "tptr", for data hash ptr or tree/manifest hash ptr
// md is the hash value (array of 32 bytes) to be stored
void
ccnl_manifest_prependHash(struct list_s *m, char *typ, unsigned char *md)
{
    ccnl_manifest_prepend(m->rest->first /* hash grp */, typ, 
                          ccnl_manifest_atomic_blob(md, SHA256_DIGEST_LENGTH));
}


// this defines the current hashgroup's metadata
void
ccnl_manifest_setMetaData(struct list_s *m, struct ccnl_prefix_s *locator,
                          struct ccnl_prefix_s *metadataUri,
                          int blockSize, long totalSize,
                          unsigned char* totalDigest)
{
    struct list_s *grp = m->rest->first;
    struct list_s *meta = ccnl_calloc(1, sizeof(struct list_s));

    meta->first = ccnl_manifest_atomic("about");

    if (metadataUri)
        ccnl_manifest_prepend(meta->first, "xternalMetadata",
                              ccnl_manifest_atomic_prefix(metadataUri));
    if (totalDigest)
        ccnl_manifest_prepend(meta->first, "hash",
                              ccnl_manifest_atomic_blob(totalDigest,
                                                        SHA256_DIGEST_LENGTH));
    if (totalSize >= 0)
        ccnl_manifest_prepend(meta->first, "size",
                              ccnl_manifest_atomic_uint(totalSize));
    if (blockSize >= 0)
        ccnl_manifest_prepend(meta->first, "blocksz",
                              ccnl_manifest_atomic_uint(blockSize));
    if (locator)
        ccnl_manifest_prepend(meta->first, "locator",
                              ccnl_manifest_atomic_prefix(locator));

    meta->rest =  grp->rest;
    grp->rest = meta;
}

// todo: void ccnl_manifest_prependNewHashGroup() { ... } 

void
ccnl_manifest_free(struct list_s *m)
{
    if (!m)
        return;
    if (m->typ == '@')
        ccnl_free(m->var);
    ccnl_manifest_free(m->first);
    ccnl_manifest_free(m->rest);
    ccnl_free(m);
}

// ----------------------------------------------------------------------

static unsigned char out[64*1024];

static char*
digest2str(unsigned char *md)
{
    static char tmp[80];
    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(tmp + 2*i, "%02x", md[i]);
    return tmp;
}

void
assertDir(char *dirpath, char *cp)
{
    char *fn;

    asprintf(&fn, "%s/%c%c", dirpath, cp[0], cp[1]);
    if (mkdir(fn, 0777) && errno != EEXIST) {
        DEBUGMSG(FATAL, "could not create directory %s\n", fn);
        exit(-1);
    }
    free(fn);
}

    
char*
digest2fname(char *dirpath, char isRoot, unsigned char *digest)
{
    char *hex = digest2str(digest), *fn = NULL, *fn2 = NULL;

    if (isRoot) {
        assertDir(dirpath, "zz");
        asprintf(&fn, "../%c%c/%s", hex[0], hex[1], hex+2);
        asprintf(&fn2, "%s/zz/%s", dirpath, hex);
        symlink(fn, fn2);
        free(fn);
        free(fn2);
        fn = NULL;
    }
    
    assertDir(dirpath, hex);
    asprintf(&fn, "%s/%c%c/%s", dirpath, hex[0], hex[1], hex+2);

    return fn;
}

int
flic_namelessObj2file(unsigned char *data, int len,
                      char *dirpath, unsigned char *digest)
{
    int f, offset = sizeof(out);
    char *fname;

    if ((len+8) > offset)
        return -1;

    len = flic_prependContent(NULL, data, len, NULL, NULL, &offset, out);
    if (len < 0)
        return -1;

    ccnl_SHA256(out + offset, len, digest);
    DEBUGMSG(DEBUG, "  %s (%d)\n", digest2str(digest), len);

#ifdef USE_SUITE_CCNTLV
    if (flic_prependFixedHdr)
        len = flic_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                   len, 255, &offset, out);
#endif

    if (len < 0)
        return -1;

    fname = digest2fname(dirpath, 0, digest);
    f = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    free(fname);
    if (f < 0) {
        perror("file open:");
        return -1;
    }
    write(f, out + offset, len);
    close(f);

    return len;
}

void
flic_finalize_HMAC_signature(struct key_s *keys, unsigned char *sigval,
                             unsigned char *buf, int validationLen)
{
    int len;
    unsigned int typ;
    unsigned char keyval[SHA256_BLOCK_LENGTH];
    
#ifdef USE_SUITE_NDNTLV
    if (theSuite == CCNL_SUITE_NDNTLV)
        ccnl_ndntlv_dehead(&buf, &validationLen, &typ, &len);
#endif

    DEBUGMSG(DEBUG, "creating signature for %d bytes\n", validationLen);
    // use first key
    ccnl_hmac256_keyval(keys->key, keys->keylen, keyval);
    len = SHA256_DIGEST_LENGTH;
    ccnl_hmac256_sign(keyval, sizeof(keyval), buf, validationLen,
                      sigval, &len);
}

void
flic_manifest2file(struct list_s *m, char isRoot, struct ccnl_prefix_s *name,
                   struct key_s *keys, char *dirpath, unsigned char *digest)
{
    int offset = sizeof(out), len, f;
    int endOfValidation = 0;
    char *fname;

    if (name)
        ccnl_manifest_prepend(m, "name", ccnl_manifest_atomic_prefix(name));

    if (keys) { // take care of signature memory space
        switch(theSuite) {
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            ccnl_manifest_append(m, "info", ccnl_manifest_atomic_blob(
                                     (unsigned char*) "\x1b\x01\x04", 3));
            ccnl_manifest_append(m, "val",
                        ccnl_manifest_atomic_blob(out, SHA256_DIGEST_LENGTH));
            endOfValidation = sizeof(out) - SHA256_DIGEST_LENGTH - 2;
            break;
#endif
        default:
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
            offset -= SHA256_DIGEST_LENGTH;
            flic_prependTL(CCNX_TLV_TL_ValidationPayload,
                           SHA256_DIGEST_LENGTH, &offset, out);
            endOfValidation = offset;
            flic_prependTL(CCNX_VALIDALGO_HMAC_SHA256, 0, &offset, out);
            flic_prependTL(CCNX_TLV_TL_ValidationAlgo, 4, &offset, out);
#endif
            break;
        }
    }
    emit(m, 0, &offset, out);
    if (keys)
        flic_finalize_HMAC_signature(keys,
                                     out + sizeof(out) - SHA256_DIGEST_LENGTH,
                                     out + offset, endOfValidation - offset);

    len = sizeof(out) - offset;
    ccnl_SHA256(out + offset, len, digest);

#ifdef USE_SUITE_CCNTLV
    if (flic_prependFixedHdr)
        len = flic_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                   len, 255, &offset, out);
#endif

    // Save the packet to disk
    fname = digest2fname(dirpath, isRoot, digest);
    f = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    free(fname);
    if (f < 0) {
        perror("file open:");
        return;
    }
    write(f, out + offset, len);
    close(f);
}

// ----------------------------------------------------------------------

static unsigned char body[MAX_BLOCK_SIZE];

void
flic_produceFromFile(char *dirpath, struct key_s *keys, int block_size,
                     char *fname, struct ccnl_prefix_s *name,
                     struct ccnl_prefix_s *meta)
{
    int f, num_bytes, len, chunk_number;
    SHA256_CTX_t ctx;
    unsigned char md[SHA256_DIGEST_LENGTH], total[SHA256_DIGEST_LENGTH];
    struct list_s *m;
    long length, offset;
    int max_pointers = (block_size - 4 - 4) / (32+4), num_pointers = 0;

    DEBUGMSG(DEBUG, "max_pointers per manifest is %d\n", max_pointers);

    f = open(fname, O_RDONLY);
    if (f < 0) {
        perror("file open:");
        return;
    }

    // compute overall SHA256
    ccnl_SHA256_Init(&ctx);
    for (;;) {
        len = read(f, body, sizeof(body));
        if (len <= 0)
            break;
        ccnl_SHA256_Update(&ctx, body, len);
    }
    ccnl_SHA256_Final(total, &ctx);

    // get the number of blocks (leaf nodes)
    length = lseek(f, 0, SEEK_END);
    chunk_number = ((length - 1) / block_size) + 1;

    DEBUGMSG(INFO, "Creating FLIC from %s\n", fname);
    DEBUGMSG(INFO, "... file size  = %ld\n", length);
    DEBUGMSG(INFO, "... block size = %d\n", block_size);

    m = ccnl_manifest_getEmptyTemplate();
    while (chunk_number > 0) {
        offset = (long) block_size * --chunk_number;
        len = length - offset;
        if (len > block_size)
            len = block_size;

        DEBUGMSG(INFO, "flic: block at offset %ld (len=%d)\n", offset, len);
        lseek(f, offset, SEEK_SET);
        num_bytes = read(f, body, len);
        if (num_bytes != len) {
            DEBUGMSG(FATAL, "Read %dB from offset %ld failed.\n",
                     len, offset);
            return;
        }

        DEBUGMSG(VERBOSE, "Creating a data packet with %d bytes\n", len);
        flic_namelessObj2file(body, len, dirpath, md);

        // Prepend the hash to the current manifest
        ccnl_manifest_prependHash(m, "dptr", md);
        num_pointers++;

        // Finally, check to see if we need to cap the current manifest and
        // move onto the next one
        if (num_pointers == max_pointers || offset == 0) {
            if (offset)
                flic_manifest2file(m, 0, NULL, NULL, dirpath, md);
            else {
                ccnl_manifest_setMetaData(m, 0, meta, block_size, length,total);
                flic_manifest2file(m, 1, name, keys, dirpath, md);
            }

            ccnl_manifest_free(m);
            m = ccnl_manifest_getEmptyTemplate();
            // append the hash of previous manifest (to make the connection)
            ccnl_manifest_prependHash(m, "tptr", md);
            num_pointers = 1;
        }
    }

    close(f);
    ccnl_manifest_free(m);
}

// ----------------------------------------------------------------------

struct ccnl_pkt_s*
flic_lookup(int sock, struct sockaddr *sa, struct ccnl_prefix_s *locator,
            unsigned char *objHashRestr)
{
    char dummy[256];
    unsigned char *data;
    int datalen, len, rc, cnt;

    DEBUGMSG(TRACE, "lookup %s\n", ccnl_prefix2path(dummy, sizeof(dummy),
                                                       locator));
    len = flic_mkInterest(locator, NULL, objHashRestr, out, sizeof(out));
    DEBUGMSG(DEBUG, "interest has %d bytes\n", len);

    for (cnt = 0; cnt < 3; cnt++) {
        rc = sendto(sock, out, len, 0, sa, sizeof(*sa));
        DEBUGMSG(TRACE, "sendto returned %d\n", rc);
        if (rc < 0) {
            perror("sendto");
            return NULL;
        }
        if (block_on_read(sock, 3) > 0) {
            len = recv(sock, out, sizeof(out), 0);
            DEBUGMSG(DEBUG, "recv returned %d\n", len);

            data = out;
            datalen = len;
#ifdef USE_SUITE_CCNTLV
            if (theSuite == CCNL_SUITE_CCNTLV) {
                data += 8;
                datalen -= 8;
            }
#endif
            return flic_bytes2pkt(out, &data, &datalen);
        }
        DEBUGMSG(WARNING, "timeout after block_on_read - %d\n", cnt);
    }
    return 0;
}

int
flic_do_dataPtr(int sock, struct sockaddr *sa, struct ccnl_prefix_s *locator,
                unsigned char *objHashRestr, int fd, SHA256_CTX_t *total)
{
    struct ccnl_pkt_s *pkt;

    pkt = flic_lookup(sock, sa, locator, objHashRestr);
    if (!pkt)
        return -1;
    write(fd, pkt->content, pkt->contlen);
    ccnl_SHA256_Update(total, pkt->content, pkt->contlen);

    return 0;
}

int
flic_do_manifestPtr(int sock, struct sockaddr *sa, int exitBehavior,
                    struct key_s *keys, struct ccnl_prefix_s *locator,
                    unsigned char *objHashRestr, int fd, SHA256_CTX_t *total)
{
    struct ccnl_pkt_s *pkt;
    unsigned char *msg;
    int msglen, len, len2;
    unsigned int typ;
    int depth = 0;

TailRecurse:
    (void) depth;
    DEBUGMSG(INFO, "manifest %d\n", depth++);

    pkt = flic_lookup(sock, sa, locator, objHashRestr);
    if (!pkt)
        return -1;
    msg = pkt->buf->data;
    msglen = pkt->buf->datalen;
#ifdef USE_SUITE_CCNTLV
    if (theSuite == CCNL_SUITE_CCNTLV) {
        msg += 8;
        msglen -= 8;
    }
#endif

    if (flic_dehead(&msg, &msglen, &typ, &len) < 0)
        goto Bail;
    if (typ != m_codes[M_MANIFESTMSG])
        goto Bail;

    if (keys) {
        if (!pkt->hmacLen) {
            if (exitBehavior == 0) {
                DEBUGMSG(FATAL, "no signature data found, packet dropped\n");
                return -1;
            }
            if (exitBehavior == 1) {
                DEBUGMSG(INFO, "no signature data found\n");
            }
        } else {
            int cnt = 1;
            DEBUGMSG(DEBUG, "checking signature on %d bytes\n", pkt->hmacLen);
            while (keys) {
                unsigned char keyval[SHA256_BLOCK_LENGTH];
                unsigned char signature[SHA256_DIGEST_LENGTH];
                int signLen = SHA256_DIGEST_LENGTH;
                DEBUGMSG(VERBOSE, "trying key #%d\n", cnt);
                ccnl_hmac256_keyval(keys->key, keys->keylen, keyval);
                ccnl_hmac256_sign(keyval, sizeof(keyval), pkt->hmacStart,
                                  pkt->hmacLen, signature, &signLen);
                if (!memcmp(signature, pkt->hmacSignature, sizeof(signature))) {
                    DEBUGMSG(INFO, "signature is valid (key #%d)\n", cnt);
                    break;
                }
                keys = keys->next;
                cnt++;
            }
            if (!keys) {
                if (exitBehavior == 0) {
                    DEBUGMSG(FATAL, "invalid signature, no output\n");
                    return -1;
                }
                DEBUGMSG(FATAL, "invalid signature\n");
            }
        }
    }

    while (flic_dehead(&msg, &len, &typ, &len2) >= 0) {
        if (typ != m_codes[M_HASHGROUP]) {
            msg += len2;
            len -= len2;
            continue;
        }
        DEBUGMSG(TRACE, "hash group\n");
        while (flic_dehead(&msg, &len, &typ, &len2) >= 0) {
            if (len2 == SHA256_DIGEST_LENGTH) {
                if (typ == m_codes[M_HG_PTR2DATA]) {
                    if (flic_do_dataPtr(sock, sa, NULL, msg, fd, total) < 0)
                        return -1;
                } else if (typ == m_codes[M_HG_PTR2MANIFEST]) {
                    if (len == SHA256_DIGEST_LENGTH) {
                        objHashRestr = msg;
                        keys = NULL;
                        goto TailRecurse;
                    }
                    if (flic_do_manifestPtr(sock, sa, exitBehavior,
                                            0, NULL, msg, fd, total) < 0)
                        return -1;
                }
            }
            msg += len2;
            len -= len2;
        }
    }
    return 0;
Bail:
    DEBUGMSG(DEBUG, "do_manifestPtr had a problem\n");
    return -1;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    char *dirpath = NULL, *outfname = NULL;
    char *udp = strdup("127.0.0.1/9695"), *ux = NULL;
    struct ccnl_prefix_s *uri = NULL, *metadataUri = NULL;
    int opt, exitBehavior = 0, i;
    struct key_s *keys = NULL;
    int block_size = 4096 - 8 - 8; // nameless obj will be exactly 4096B
    unsigned char *objHashRestr = NULL;

    progname = argv[0];

    while ((opt = getopt(argc, argv, "hb:d:e:f:k:m:o:p:s:u:v:x:")) != -1) {
        switch (opt) {
        case 'b':
            block_size = atoi(optarg);
            if (block_size > (MAX_BLOCK_SIZE-256)) {
                DEBUGMSG(FATAL, "Error: block size cannot exceed %d\n",
                         MAX_BLOCK_SIZE - 256);
                goto Usage;
            }
            break;
        case 'd':
            if (strlen(optarg) != 64)
                goto Usage;
            ccnl_free(objHashRestr);
            objHashRestr = ccnl_malloc(SHA256_DIGEST_LENGTH);
            for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
                objHashRestr[i] = hex2int(optarg[2*i]) * 16 +
                                                   hex2int(optarg[2*i + 1]);
            break;
        case 'e':
            exitBehavior = atoi(optarg);
            break;
        case 'k':
            keys = load_keys_from_file(optarg);
            break;
        case 'm':
            metadataUri = ccnl_URItoPrefix(optarg, theSuite, NULL, NULL);
            break;
        case 'o':
            outfname = optarg;
            break;
        case 'p':
            dirpath = optarg;
            break;
        case 's':
            theSuite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(theSuite))
                usage(1);
            break;
        case 'u':
            udp = optarg;
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
        case 'x':
            ux = optarg;
            break;
        case 'h':
        default:
Usage:
            usage(1);
        }
    }

    switch(theSuite) {
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        flic_prependTL = ccnl_ndntlv_prependTL;
        flic_prependUInt = ccnl_ndntlv_prependNonNegIntVal;
        flic_prependNameComponents = ccnl_ndntlv_prependNameComponents;
        flic_prependContent = ccnl_ndntlv_prependContent;
        flic_prependFixedHdr = NULL;
        flic_dehead = ccnl_ndntlv_dehead;
        flic_bytes2pkt = ccnl_ndntlv_bytes2pkt;
        flic_mkInterest = ndntlv_mkInterest;
        m_codes = ndntlv_m_codes;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        flic_prependTL = ccnl_ccntlv_prependTL;
        flic_prependUInt = ccnl_ccntlv_prependUInt;
        flic_prependNameComponents = ccnl_ccntlv_prependNameComponents;
        flic_prependContent = ccnl_ccntlv_prependContent;
        flic_prependFixedHdr = ccnl_ccntlv_prependFixedHdr;
        flic_dehead = ccnl_ccntlv_dehead;
        flic_bytes2pkt = ccnl_ccntlv_bytes2pkt;
        flic_mkInterest = ccntlv_mkInterest;
        m_codes = ccntlv_m_codes;
        break;
#endif
    default:
        DEBUGMSG(FATAL, "unknown suite\n");
        exit(-1);
    }

    if (dirpath) {
        struct stat s;
        char *fname = NULL;

        if (stat(dirpath, &s)) {
            perror("access to dirpath");
            return -1;
        }
        if (!(s.st_mode & S_IFDIR)) {
            DEBUGMSG(FATAL, "<%s> is not a directory\n", dirpath);
            return -1;
        }
        if (optind >= argc)
            goto Usage;
        fname = argv[optind++];
        if (optind < argc)
            uri = ccnl_URItoPrefix(argv[optind++], theSuite, NULL, NULL);
        if (optind < argc)
            goto Usage;

        flic_produceFromFile(dirpath, keys, block_size,
                             fname, uri, metadataUri);
    } else { // retrieve a manifest tree
        int fd, port, sock;
        const char *addr = NULL;
        struct sockaddr sa;
        SHA256_CTX_t total;
        unsigned char md[SHA256_DIGEST_LENGTH];

        if (optind >= argc)
            goto Usage;
        uri = ccnl_URItoPrefix(argv[optind++], theSuite, NULL, NULL);
        if (optind < argc)
            goto Usage;

        if (ccnl_parseUdp(udp, theSuite, &addr, &port) != 0)
            exit(-1);
        if (ux) { // use UNIX socket
            ccnl_setUnixSocketPath((struct sockaddr_un*) &sa, ux);
            sock = ux_open();
        } else { // UDP
            ccnl_setIpSocketAddr((struct sockaddr_in*) &sa, addr, port);
            sock = udp_open();
        }

        if (outfname)
            fd = open(outfname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
        else
            fd = 1;

        ccnl_SHA256_Init(&total);
        if (!flic_do_manifestPtr(sock, &sa, exitBehavior, keys,
                                 uri, objHashRestr, fd, &total)) {
            ccnl_SHA256_Final(md, &total);
            DEBUGMSG(INFO, "Total SHA256 is %s\n", digest2str(md));
        }
        close(fd);

    }

    return 0;
}

// eof
