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
#define USE_SHA256
#define USE_UNIXSOCKET

#define MAX_BLOCK_SIZE (64*1024)

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"
#include "ccnl-socket.c"

// ----------------------------------------------------------------------

char *progname;
char fnbuf[512];

void
usage(int exitval)
{
    fprintf(stderr, "usage: %s -p REPODIR [options] FILE [URI]  (producing)\n"
                    "       %s [options] URI                    (retrieving)\n"
            "  -b SIZE    Block size (default is 4K)\n"
            "  -d DIGEST  ObjHashRestriction (32B in hex)\n"
            "  -e N       0=drop-if-nosig-or-inval, 1=warn-if-nosig-or-inval, 2=warn-inval\n"
            "  -k FNAME   HMAC256 key (base64 encoded)\n"
            "  -m URI     URI of external metadata\n"
            "  -o FNAME   outfile\n"
            "  -s SUITE   (ccnx2015, ndn2013)\n"
            "  -t SHAPE   tree shape: balanced, 7mpr, deep\n"
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
    M_METAINFO,
    M_MI_CONTENTTYPE,
    M_CONTENT,
    M_HASHGROUP,
    M_HG_METADATA,
    M_HG_PTR2DATA,
    M_HG_PTR2MANIFEST,
    M_MT_LOCATOR,
    M_MT_EXTERNALNAME,
    M_MT_BLOCKSIZE,
    M_MT_OVERALLDATASIZE,
    M_MT_OVERALLDATASHA256,
    M_MT_TREEDEPTH,
    M_SIGINFO,
    M_SIGVAL,
};

#ifdef USE_SUITE_CCNTLV
unsigned short ccntlv_m_codes[] = {
    CCNX_TLV_TL_Manifest,
    CCNX_TLV_M_Name,
    0, // na
    0, // na
    0, // na
    CCNX_MANIFEST_HASHGROUP,
    CCNX_MANIFEST_HG_METADATA,
    CCNX_MANIFEST_HG_PTR2DATA,
    CCNX_MANIFEST_HG_PTR2MANIFEST,
    CCNX_MANIFEST_MT_LOCATOR,
    CCNX_MANIFEST_MT_EXTERNALMETADATA,
    CCNX_MANIFEST_MT_BLOCKSIZE,
    CCNX_MANIFEST_MT_OVERALLDATASIZE,
    CCNX_MANIFEST_MT_OVERALLDATASHA256,
    CCNX_MANIFEST_MT_TREEDEPTH,
    0, // na
    0  // na
};
#endif

#ifdef USE_SUITE_NDNTLV
unsigned short ndntlv_m_codes[] = {
    NDN_TLV_Data,
    NDN_TLV_Name,
    NDN_TLV_MetaInfo,
    NDN_TLV_ContentType,
    NDN_TLV_Content,
    NDN_TLV_MANIFEST_HASHGROUP,
    NDN_TLV_MANIFEST_HG_METADATA,
    NDN_TLV_MANIFEST_HG_PTR2DATA,
    NDN_TLV_MANIFEST_HG_PTR2MANIFEST,
    NDN_TLV_MANIFEST_MT_LOCATOR,
    NDN_TLV_MANIFEST_MT_EXTERNALMETADATA,
    NDN_TLV_MANIFEST_MT_BLOCKSIZE,
    NDN_TLV_MANIFEST_MT_OVERALLDATASIZE,
    NDN_TLV_MANIFEST_MT_OVERALLDATASHA256,
    NDN_TLV_MANIFEST_MT_TREEDEPTH,
    NDN_TLV_SignatureInfo,
    NDN_TLV_SignatureValue
};
#endif

int theSuite = CCNL_SUITE_DEFAULT;
char *theRepoDir;
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
int (*flic_dehead)(unsigned char **buf, int *len, unsigned int *typ, unsigned int *vallen);
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
    { 'c', M_MI_CONTENTTYPE,        "T=content type" },       // NDN
    { 'd', M_HG_PTR2DATA,           "T=data hash ptr" },
    { 'e', M_MT_TREEDEPTH,          "T=tree depth" },
    { 'f', M_METAINFO,              "T=meta info" },          // NDN
    { 'g', M_HASHGROUP,             "T=hash pointer group" },
    { 'h', M_MT_OVERALLDATASHA256,  "T=meta overall hash" },
    { 'i', M_SIGINFO,               "T=signature info" },
    { 'l', M_MT_LOCATOR,            "T=meta locator" },
    { 'm', M_MANIFESTMSG,           "T=manifest" },           // Data in NDN
    { 'n', M_NAME,                  "T=name" },
    { 'o', M_CONTENT,               "T=content" },            // NDN
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
    node->first->var = ccnl_malloc(1+len); // do alloc even when len==0
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

struct list_s*
ccnl_manifest_prepend(struct list_s *where, char *typ, struct list_s *what)
{
    struct list_s *elmnt = ccnl_calloc(1, sizeof(*elmnt));

    elmnt->first = ccnl_manifest_atomic(typ);
    elmnt->first->rest = what;
    elmnt->rest = where->rest;
    where->rest = elmnt;

    return elmnt;
}

struct list_s*
ccnl_manifest_getEmptyTemplate(int suite)
{
    struct list_s *m, *e;

    switch(suite) {
    case CCNL_SUITE_CCNTLV:
        m = ccnl_manifest_atomic("mfst");
        ccnl_manifest_prepend(m, "grp", NULL);
        break;
    case CCNL_SUITE_NDNTLV:
        m = ccnl_manifest_atomic("mfst");
        e = ccnl_manifest_prepend(m, "o", NULL);
        ccnl_manifest_prepend(e->first, "grp", NULL);
        e = ccnl_manifest_prepend(m, "f", NULL);
        e = ccnl_manifest_prepend(e->first, "c", NULL);
        e->first->rest = ccnl_manifest_atomic_uint(NDN_Content_Manifest);
        break;
    default:
        m = NULL;
    }
    return m;
}

struct list_s*
ccnl_manifest_getFirstHashGroup(struct list_s *m)
{
    switch(theSuite) {
    case CCNL_SUITE_CCNTLV:
        return m->rest->first;
    case CCNL_SUITE_NDNTLV:
        return m->rest->rest->first->rest->first;
    default:
        break;
    }
    return NULL;
}

// typ is either "dptr" or "tptr", for data hash ptr or tree/manifest hash ptr
// md is the hash value (array of 32 bytes) to be stored
void
ccnl_manifest_prependHash(struct list_s *m, char *typ, unsigned char *md)
{
    ccnl_manifest_prepend(ccnl_manifest_getFirstHashGroup(m), typ,
                          ccnl_manifest_atomic_blob(md, SHA256_DIGEST_LENGTH));
}


// this defines the current hashgroup's metadata
void
ccnl_manifest_setMetaData(struct list_s *m, struct ccnl_prefix_s *locator,
                          struct ccnl_prefix_s *metadataUri,
                          int blockSize, long totalSize, int depth,
                          unsigned char* totalDigest)
{
    struct list_s *grp = ccnl_manifest_getFirstHashGroup(m);
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
    if (depth)
        ccnl_manifest_prepend(meta->first, "eedepth",
                              ccnl_manifest_atomic_uint(depth));
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
assertDir(char *dirpath, char *lev1, char *lev2)
{
    snprintf(fnbuf, sizeof(fnbuf), "%s/%c%c", dirpath, lev1[0], lev1[1]);
    if (mkdir(fnbuf, 0777) && errno != EEXIST) {
        DEBUGMSG(FATAL, "could not create directory %s\n", fnbuf);
        exit(-1);
    }

    if (!lev2)
        return;
    snprintf(fnbuf, sizeof(fnbuf), "%s/%c%c/%c%c",
             dirpath, lev1[0], lev1[1], lev2[0], lev2[1]);
    if (mkdir(fnbuf, 0777) && errno != EEXIST) {
        DEBUGMSG(FATAL, "could not create directory %s\n", fnbuf);
        exit(-1);
    }
}

char*
digest2fname(char *dirpath, unsigned char *packetDigest)
{
    char *hex = digest2str(packetDigest);

    assertDir(dirpath, hex, hex+2);
    snprintf(fnbuf, sizeof(fnbuf), "%s/%02x/%02x/%s", dirpath,
             packetDigest[0], packetDigest[1], hex+4);

    return fnbuf;
}

void
flic_link(char *dirpath, char *linkdir,
          unsigned char *packetDigest, unsigned char *linkDigest)
{
    char *hex, lnktarget[100];

    assertDir(dirpath, linkdir, NULL);

    hex = digest2str(packetDigest);
    snprintf(lnktarget, sizeof(lnktarget), "../%02x/%02x/%s",
             packetDigest[0], packetDigest[1], hex+4);

    hex = digest2str(linkDigest);
    snprintf(fnbuf, sizeof(fnbuf), "%s/%s/%s", dirpath, linkdir, hex);
    
    symlink(lnktarget, fnbuf);
}

int
flic_obj2file(unsigned char *data, int len, char *dirpath,
              struct ccnl_prefix_s *name, unsigned char *digest)
{
    int f, offset = sizeof(out);
    char *fname;

    if ((len+8) > offset)
        return -1;

    len = flic_prependContent(name, data, len, NULL, NULL, &offset, out);
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

    fname = digest2fname(dirpath, digest);
    f = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
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
    unsigned int typ, len;
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
                   struct key_s *keys, char *dirpath,
                   unsigned char *dataDigest /* in */,
                   unsigned char *packetDigest /* out */)
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
#ifdef USE_SUITE_NDNTLV
    else if (theSuite == CCNL_SUITE_NDNTLV) {
      // signature in NDN, even if empty, is mandatory
        ccnl_manifest_append(m, "info", ccnl_manifest_atomic_blob(
                              (unsigned char*) "\x1b\x01\x00\x1c\x00", 5));
        ccnl_manifest_append(m, "val", ccnl_manifest_atomic_blob(out, 0));
    }
#endif
    
    emit(m, 0, &offset, out);
    if (keys)
        flic_finalize_HMAC_signature(keys,
                                     out + sizeof(out) - SHA256_DIGEST_LENGTH,
                                     out + offset, endOfValidation - offset);

    len = sizeof(out) - offset;
    ccnl_SHA256(out + offset, len, packetDigest);

#ifdef USE_SUITE_CCNTLV
    if (flic_prependFixedHdr)
        len = flic_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                   len, 255, &offset, out);
#endif

    // Save the packet to disk
    fname = digest2fname(dirpath, packetDigest);
    f = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (f < 0) {
        perror("file open:");
        return;
    }
    if (isRoot) {
        if (name)
            flic_link(dirpath, "nm", packetDigest, packetDigest);
        flic_link(dirpath, "hs", packetDigest, dataDigest);
    }
    write(f, out + offset, len);
    close(f);
}

// ----------------------------------------------------------------------

static unsigned char body[MAX_BLOCK_SIZE];

int
flic_mkDataChunk(int f, struct ccnl_prefix_s *name, long offset, int len,
                 int maxdatalen, unsigned char *outDigest)
{
    int num_bytes;
    DEBUGMSG(DEBUG, "flic: block at offset %ld (len=%d)\n", offset, len);

    if (len > maxdatalen)
        len = maxdatalen;
    lseek(f, offset, SEEK_SET);
    num_bytes = read(f, body, len);
    if (num_bytes != len) {
        DEBUGMSG(FATAL, "Read %dB from offset %ld failed.\n",
                 len, offset);
        return -1;
    }

    DEBUGMSG(VERBOSE, "  creating packet with %d bytes\n", len);
    flic_obj2file(body, len, theRepoDir, name, outDigest);

    return 0;
}

void
flic_produceDeepTree(struct key_s *keys, int block_size,
                     char *fname, struct ccnl_prefix_s *name,
                     struct ccnl_prefix_s *meta)
{
    int f, len, chunk_number, depth = 0;
    SHA256_CTX_t ctx;
    unsigned char md[SHA256_DIGEST_LENGTH], total[SHA256_DIGEST_LENGTH];
    struct list_s *m;
    long offset, length;
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

    DEBUGMSG(INFO, "Creating deep FLIC from %s\n", fname);
    DEBUGMSG(INFO, "... file size  = %ld\n", length);
    DEBUGMSG(INFO, "... block size = %d\n", block_size);

    m = ccnl_manifest_getEmptyTemplate(theSuite);
    while (chunk_number > 0) {
        offset = (long) block_size * --chunk_number;
        flic_mkDataChunk(f, theSuite == CCNL_SUITE_NDNTLV ? name : NULL,
                         offset, length - offset, block_size, md);

        // Prepend the hash to the current manifest
        ccnl_manifest_prependHash(m, "dptr", md);
        num_pointers++;

        // Finally, check to see if we need to cap the current manifest and
        // move onto the next one
        if (num_pointers == max_pointers || offset == 0) {
            depth++;
            if (offset)
                flic_manifest2file(m, 0, NULL, NULL, theRepoDir, NULL, md);
            else { // root manifest
                ccnl_manifest_setMetaData(m, 0, meta, block_size, length,
                                          depth, total);
                if (theSuite == CCNL_SUITE_NDNTLV)
                    ccnl_prefix_appendCmp(name, (unsigned char*) "!", 1);
                flic_manifest2file(m, 1, name, keys, theRepoDir, total, md);
            }
            ccnl_manifest_free(m);
            m = ccnl_manifest_getEmptyTemplate(theSuite);
            // append the hash of previous manifest (to make the connection)
            ccnl_manifest_prependHash(m, "tptr", md);
            num_pointers = 1;
        }
    }

    close(f);
    ccnl_manifest_free(m);
}

void
flic_produce7mprTree(struct key_s *keys, int block_size,
                     char *fname, struct ccnl_prefix_s *name,
                     struct ccnl_prefix_s *meta)
{
    return;
}

struct list_s*
balancedTree(int f, struct ccnl_prefix_s *name, int blocksz, int reserved,
             int lev, int *depth, long offset, long length,
             unsigned char *outDigest, SHA256_CTX_t ctx)
{
    struct list_s *m = ccnl_manifest_getEmptyTemplate(theSuite), *m2;
    int maxppc, chunk_number, maxdatalen, num_pointers = 0;
    unsigned char md[SHA256_DIGEST_LENGTH];

    DEBUGMSG(DEBUG, "balancedTree lev=%d, offs=%ld, len=%ld\n",
             lev, offset, length);
    if (lev > *depth)
        *depth = lev;

    maxdatalen = blocksz - reserved;
    maxppc =  maxdatalen / (32 + 4);
    chunk_number = ((length - 1) / maxdatalen) + 1;
    
    DEBUGMSG(VERBOSE, "  maxdatalen=%d, maxppc=%d, #chunks=%d\n",
             maxdatalen, maxppc, chunk_number);

    if (chunk_number > maxppc) {
        int left, right;
        right = (chunk_number - maxppc + 2) / 2;
        left = (chunk_number - maxppc + 2) - right;

        DEBUGMSG(VERBOSE, "  left=%d, right=%d\n", left, right);
        m2 = balancedTree(f, name, blocksz, reserved, lev+1, depth,
                          offset + (maxppc-2 + left)*maxdatalen,
                          length - (maxppc-2 + left)*maxdatalen, md, ctx);
        if (!m2)
            goto Failed;
        flic_manifest2file(m2, 0, NULL, NULL, theRepoDir, NULL, md);
        ccnl_manifest_free(m2);
        ccnl_manifest_prependHash(m, "tptr", md);

        m2 = balancedTree(f, name, blocksz, reserved, lev+1, depth,
                          offset + (maxppc-2)*maxdatalen,
                          left*maxdatalen, md, ctx);
        if (!m2)
            goto Failed;
        flic_manifest2file(m2, 0, NULL, NULL, theRepoDir, NULL, md);
        ccnl_manifest_free(m2);
        ccnl_manifest_prependHash(m, "tptr", md);

        chunk_number -= left + right;
        num_pointers += 2;
    }
    DEBUGMSG(VERBOSE, "  lev=%d, %d chunks left to add, length=%ld\n",
             lev, chunk_number, length);
    while (chunk_number > 0) {
        long pos = offset + --chunk_number * maxdatalen;

        flic_mkDataChunk(f, name, pos, offset + length - pos, maxdatalen, md);

        // Prepend the hash to the current manifest
        ccnl_manifest_prependHash(m, "dptr", md);
        num_pointers++;
    }
    return m;
Failed:
    ccnl_manifest_free(m);
    return NULL;
}

void
flic_produceBalancedTree(struct key_s *keys, int block_size, int reserved,
                         char *fname, struct ccnl_prefix_s *name,
                         struct ccnl_prefix_s *meta)
{
    int f, len, depth = 0;
    SHA256_CTX_t ctx;
    unsigned char md[SHA256_DIGEST_LENGTH], total[SHA256_DIGEST_LENGTH];
    struct list_s *m;
    long length;

    DEBUGMSG(DEBUG, "produceBalancedTree\n");

    f = open(fname, O_RDONLY);
    if (f < 0) {
        perror("file open:");
        return;
    }

    // compute overall content SHA256
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

    DEBUGMSG(INFO, "Creating balanced FLIC from %s\n", fname);
    DEBUGMSG(INFO, "... file size  = %ld\n", length);
    DEBUGMSG(INFO, "... block size = %d\n", block_size);

    m = balancedTree(f, theSuite == CCNL_SUITE_NDNTLV ? name : NULL,
                     block_size, reserved, 1, &depth, 0, length, total, ctx);
    if (m) {
        DEBUGMSG(INFO, "... reached depth=%d\n", depth);
        ccnl_manifest_setMetaData(m, 0, meta, block_size, length,
                                  depth, total);
        if (theSuite == CCNL_SUITE_NDNTLV)
            ccnl_prefix_appendCmp(name, (unsigned char*) "!", 1);
        flic_manifest2file(m, 1, name, keys, theRepoDir, total, md);
    }
    ccnl_manifest_free(m);

    close(f);
}

// ----------------------------------------------------------------------

int pktcnt;
//unsigned char *digests;

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

TailRecurse:
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

    if (flic_dehead(&msg, &msglen, &typ, (unsigned int*) &len) < 0)
        goto Bail;
    if (typ != m_codes[M_MANIFESTMSG]) {
        // could be ordinary content, which would be valid, right?
#ifdef USE_SUITE_CCNTLV
        if (typ == CCNX_TLV_TL_Object) {
            write(fd, pkt->content, pkt->contlen);
            ccnl_SHA256_Update(total, pkt->content, pkt->contlen);
            goto OK;
        }
#endif
        goto Bail;
    }

#ifdef USE_SUITE_NDNTLV
    if (theSuite == CCNL_SUITE_NDNTLV) {
        if (pkt->contentType != NDN_Content_Manifest)
            goto Bail;
        msg = pkt->content;
        len = pkt->contlen;
    }
#endif

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
                unsigned int signLen = SHA256_DIGEST_LENGTH;
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

    while (flic_dehead(&msg, &len, &typ, (unsigned int*) &len2) >= 0) {
        if (typ != m_codes[M_HASHGROUP]) {
            msg += len2;
            len -= len2;
            continue;
        }
        DEBUGMSG(TRACE, "hash group\n");
        while (flic_dehead(&msg, &len, &typ, (unsigned int*) &len2) >= 0) {
            if (len2 == SHA256_DIGEST_LENGTH) {
                if (typ == m_codes[M_HG_PTR2DATA]) {
                    pktcnt++;
                    DEBUGMSG(DEBUG, "@@ %3d  %s\n", pktcnt, digest2str(msg));
//                    digests = ccnl_realloc(digests, pktcnt*SHA256_DIGEST_LENGTH);
//                    memcpy(digests + (pktcnt - 1)*SHA256_DIGEST_LENGTH, msg, SHA256_DIGEST_LENGTH);
                    if (flic_do_dataPtr(sock, sa, NULL, msg, fd, total) < 0)
                        return -1;
                } else if (typ == m_codes[M_HG_PTR2MANIFEST]) {
                    DEBUGMSG(DEBUG, "@> ---  %s\n", digest2str(msg));
                    if (len == SHA256_DIGEST_LENGTH) {
                        objHashRestr = msg;
                        keys = NULL;
                        free_packet(pkt);
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
OK:
    free_packet(pkt);
    return 0;
Bail:
    DEBUGMSG(DEBUG, "do_manifestPtr had a problem\n");
    free_packet(pkt);
    return -1;
}

// ----------------------------------------------------------------------

struct task_s {
    char type; // 0=dataptr, 1=mfstptr, 2=data
    unsigned char md[SHA256_DIGEST_LENGTH];
    struct ccnl_pkt_s *pkt;
    int retry_cnt;
    struct timeval timeout;
    struct timeval lastsent;
} *tasks;
int taskcnt;
int timeoutcnt;
long rtt;
long rttaccu;
int rttcnt;
int sendwindow = 20;
int seqno;
int RbytesPerRTT, WbytesPerRTT;
struct timeval nextout;
int expandlimit = 300;
struct ccnl_buf_s *outq; // list of hashes for which to send an interest
int outqlen;
int outqlimit = 2;
int sendcnt;

void
ccnl_timeval_add(struct timeval *t, int usec)
{
    if (!t)
        return;
    t->tv_usec += usec;
    if (t->tv_usec > 1000000) {
        t->tv_sec += t->tv_usec / 1000000;
        t->tv_usec = t->tv_usec % 1000000;
    }
}

void
window_expandManifest(int curtask, struct ccnl_pkt_s *pkt,
                      int incr_usec)
{

    unsigned char *msg, *msg2;
    int msglen, msglen2;
    unsigned int typ, cnt, len, len2;

    DEBUGMSG(FATAL, "expand %d %d\n", curtask, taskcnt);
    
    DEBUGMSG(VERBOSE, "@> ---  %s expand %d %d\n",
             digest2str(tasks[curtask].md), curtask, taskcnt);
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
#ifdef USE_SUITE_NDNTLV
    if (theSuite == CCNL_SUITE_NDNTLV) {
        if (pkt->contentType != NDN_Content_Manifest)
            goto Bail;
        msg = pkt->content;
        len = pkt->contlen;
    }
#endif

    // first pass: count the number of pointers
    cnt = 0;
    msg2 = msg;
    msglen2 = len;
    while (flic_dehead(&msg2, &msglen2, &typ, &len2) >= 0) {
        if (typ != m_codes[M_HASHGROUP]) {
            msg2 += len2;
            msglen2 -= len2;
            continue;
        }
        DEBUGMSG(TRACE, "hash group\n");
        while (flic_dehead(&msg2, &msglen2, &typ, &len2) >= 0) {
            if (len2 == SHA256_DIGEST_LENGTH)
                cnt++;
            msg2 += len2;
            msglen2 -= len2;
        }
    }

    // allocate memory for the new pointers
    // TODO: handle cnt == 0
    if (cnt > 1) {
        tasks = ccnl_realloc(tasks, (taskcnt + cnt - 1)*sizeof(struct task_s));
        if (!tasks) {
            DEBUGMSG(FATAL, "realloc failed @@@@@@@@@@@@@@@@@@@@@@@@\n");
        }
        if (curtask != (taskcnt - 1))
            memmove(tasks + curtask + cnt, tasks + curtask + 1,
                    (taskcnt - curtask - 1)*sizeof(struct task_s));
        memset(tasks + curtask, 0, cnt * sizeof(struct task_s));
    } else {
        DEBUGMSG(FATAL, "*** handle cnt %d <= 1\n", cnt);
    }
    taskcnt += cnt - 1;

    // second pass: copy the pointers
    if (curtask < taskcnt) {
        struct task_s *tp = tasks + curtask;
        cnt = 0;
        while (flic_dehead(&msg, (int*) &len, &typ, &len2) >= 0) {
            if (typ != m_codes[M_HASHGROUP]) {
                msg += len2;
                len -= len2;
                continue;
            }
            DEBUGMSG(TRACE, "hash group\n");
            while (flic_dehead(&msg, (int*) &len, &typ, &len2) >= 0) {
                if (len2 == SHA256_DIGEST_LENGTH) {
                    if (typ == m_codes[M_HG_PTR2DATA])
                        tp->type = 0;
                    else if (typ == m_codes[M_HG_PTR2MANIFEST]) \
                        tp->type = 1;
                    memcpy(tp->md, msg, SHA256_DIGEST_LENGTH);
                    tp++;
                    cnt++;
                }
                msg += len2;
                len -= len2;
            }
        }
    }
    return;
Bail:
    DEBUGMSG(DEBUG, "do_manifestPtr had a problem\n");
    return;
}

int
packet_upcall(int sock)
{
    int i, len = recv(sock, out, sizeof(out), 0);
    struct timeval now;

    DEBUGMSG(DEBUG, "recv returned %d\n", len);
    if (len <= 0)
        return -1;

    unsigned char *data = out;
    int datalen = len;
#ifdef USE_SUITE_CCNTLV
    if (theSuite == CCNL_SUITE_CCNTLV) {
        data += 8;
        datalen -= 8;
    }
#endif
    struct ccnl_pkt_s *pkt = flic_bytes2pkt(out, &data, &datalen);
    if (!pkt)
        return -1;

    DEBUGMSG(DEBUG, "received %d %s\n", taskcnt, digest2str(pkt->md));
    // search for received digest
    for (i = 0; i < taskcnt; i++) {
        if (tasks[i].type >= 2 ||
            memcmp(pkt->md, tasks[i].md, SHA256_DIGEST_LENGTH))
            continue;

        DEBUGMSG(DEBUG, "  at index %d, type=%d\n", i, tasks[i].type);
        if (tasks[i].type < 2 && tasks[i].retry_cnt == 1) {
            ccnl_get_timeval(&now);
            DEBUGMSG(DEBUG, "raw rtt=%ld (pos %d)\n", timevaldelta(&now, &tasks[i].lastsent), i);
            rttaccu += timevaldelta(&now, &tasks[i].lastsent);
            rttcnt++;
            tasks[i].retry_cnt = 2;
        }
        if (tasks[i].type == 0) {
            tasks[i].type = 2;
            tasks[i].pkt = pkt;
            RbytesPerRTT += tasks[i].pkt->contlen;
            for (i++; i < taskcnt; i++) {
                if (tasks[i].type != 0 ||
                          memcmp(pkt->md, tasks[i].md, SHA256_DIGEST_LENGTH))
                    continue;
                tasks[i].type = 2;
                tasks[i].pkt = ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
                tasks[i].pkt->buf = ccnl_buf_new(pkt->content, pkt->contlen);
                tasks[i].pkt->content = tasks[i].pkt->buf->data;
                tasks[i].pkt->contlen = tasks[i].pkt->buf->datalen;
            }
            pkt = NULL;
            break;
        } else if (tasks[i].type == 1) {
            window_expandManifest(i, pkt, 10);
            free_packet(pkt);
            return 1;
        } else {
            DEBUGMSG(DEBUG, "  duplicate %d\n", i);
            free_packet(pkt);
        }
        pkt = NULL;
        break;
    }
    if (pkt) {
        DEBUGMSG(DEBUG, "  not found: %s\n", digest2str(pkt->md));
        free_packet(pkt);
    }
    return 0;
}

int
packet_request(int sock, struct sockaddr *sa,
               struct ccnl_prefix_s *locator, int i)
{
    struct ccnl_buf_s *buf, *msg;
    int rc = -1;

    if (tasks[i].type == 0)
        DEBUGMSG(DEBUG, "packet_request %d\n", i);
    else
        DEBUGMSG(DEBUG, "packet_request %d -- manifest\n", i);

    buf = ccnl_buf_new(tasks[i].md, SHA256_DIGEST_LENGTH);
    if (!buf)
        return -1;
    for (msg = outq; msg; msg = msg->next) { // already in the queue?
        if (msg->datalen == buf->datalen &&
            !memcmp(msg->data, buf->data, msg->datalen)) {
            DEBUGMSG(DEBUG, "    not enqueued because already there\n");
            ccnl_free(buf);
            goto SetTimer;
        }
        if (!msg->next) {
            msg->next = buf;
            outqlen++;
            rc = 0;
            goto SetTimer;
        }
    }
    outq = buf;
    outqlen++;
    rc = 0;
SetTimer:
    ccnl_get_timeval(&tasks[i].lastsent);
    memcpy(&tasks[i].timeout, &tasks[i].lastsent, sizeof(struct timeval));
    ccnl_timeval_add(&tasks[i].timeout, 2*rtt);
    return rc;
}

// ----------------------------------------------------------------------

int
repo_io_loop(int sock, struct sockaddr *sa,
             struct ccnl_prefix_s *locator, int fd)
{
    int maxfd = -1, rc, i, dropcnt = 0;
    fd_set readfs, writefs;
    struct timeval now, endofRTT, last;

    maxfd = sock;
    if (fd > maxfd)
        maxfd = fd;
    maxfd++;

    ccnl_get_timeval(&last);
    ccnl_get_timeval(&endofRTT);
    ccnl_timeval_add(&endofRTT, rtt);
    DEBUGMSG(INFO, "starting repo main event and IO loop\n");
    while (taskcnt > 0) {
        int usec;

        ccnl_get_timeval(&now);

        for (i = 0; outqlen < outqlimit && i < sendwindow && sendcnt < sendwindow; i++) {
            if (i > expandlimit)
                break;

            if (tasks[i].type > 1)
                continue;

            long due = timevaldelta(&tasks[i].timeout, &now);
            if (due > 0) {
                if (due < usec)
                    usec = due;
                continue;
            }
            // we have a timeout event for position i
            if (tasks[i].retry_cnt > 0) {
                dropcnt++;     // for this RTT period
                timeoutcnt++;  // global counter
                if (tasks[i].retry_cnt > 50) {
                    DEBUGMSG(FATAL, "exceeded timeout limit\n");
                    DEBUGMSG(FATAL, "%d timeouts, last rtt was %ld\n",
                             timeoutcnt, rtt);
                    exit(1);
                }
                DEBUGMSG(INFO, "!! timeout for #%d (cnt %d)\n",
                         i, tasks[i].retry_cnt);
                packet_request(sock, sa, locator, i);
                tasks[i].retry_cnt++;
            } else if (i < sendwindow) {
                if (packet_request(sock, sa, locator, i))
                    tasks[i].retry_cnt = 2;
                else
                    tasks[i].retry_cnt = 1;
            }
        }

        FD_ZERO(&readfs);
        FD_ZERO(&writefs);

        usec = rtt;
        if (outq) {
            long wait = timevaldelta(&nextout, &now);
            if (wait < 0)
                wait = 0;
            if (wait < usec)
                usec = wait;
            DEBUGMSG(DEBUG, "outq %d usec\n", usec);
        }

        if (tasks[0].type == 2)
            FD_SET(fd, &writefs);
        FD_SET(sock, &readfs);

        if (usec >= 0) {
            struct timeval deadline;
            DEBUGMSG(VERBOSE, "%d qlen=%d\n", usec, outqlen);
            deadline.tv_sec = usec / 1000000;
            deadline.tv_usec = usec % 1000000;
            rc = select(maxfd, &readfs, &writefs, NULL, &deadline);
        } else
            rc = select(maxfd, &readfs, &writefs, NULL, NULL);
        if (rc < 0) {
            perror("select(): ");
            exit(EXIT_FAILURE);
        }

        if (tasks[0].type == 2 && !FD_ISSET(fd, &writefs)) {
            DEBUGMSG(DEBUG, "blocked on consumer\n");
        }

        ccnl_get_timeval(&now);
        if (outq && timevaldelta(&nextout, &now) < 0 && sendcnt < sendwindow) {
            struct ccnl_buf_s *n = outq->next;
            for (i = 0; i < taskcnt; i++) {
                if (!memcmp(outq->data, tasks[i].md, SHA256_DIGEST_LENGTH)) {
                    int len = flic_mkInterest(locator, NULL, tasks[i].md,
                                              out, sizeof(out));
                    if (len <= 0)
                        break;
                    sendto(sock, out, len, 0, sa, sizeof(*sa));
                    DEBUGMSG(DEBUG, "sendto pos=%d\n", i);
                    ccnl_get_timeval(&tasks[i].lastsent);
                    sendcnt++;
                    break;
                }
            }
            ccnl_free(outq);
            outq = n;
            outqlen--;
            memcpy(&nextout, &now, sizeof(now)); // minum time between pckts
            ccnl_timeval_add(&nextout, 500);            
        }

        if (FD_ISSET(sock, &readfs)) {
            if (packet_upcall(sock) == 1) { // manifest
                // activate (at most 10) newly joined pointers
//                for (i = 0; outqlen < outqlimit && i < sendwindow; i++) {
                for (i = 0; outqlen < outqlimit && i < taskcnt; i++) {
                    if (tasks[i].type > 1)
                        continue;
                    if (!tasks[i].retry_cnt) {
                        if (packet_request(sock, sa, locator, i))
                            tasks[i].retry_cnt = 2;
                        else
                            tasks[i].retry_cnt = 1;
                    }
                }
            }
        }

        if (FD_ISSET(fd, &writefs) && tasks[0].type == 2 ) {
            int d = timevaldelta(&now, &last);
            DEBUGMSG(DEBUG, "@@ %3d  %d %s\n", seqno, d, digest2str(tasks[0].md));
            seqno++;
            memcpy(&last, &now, sizeof(last));

            /*
            DEBUGMSG(INFO, "  delivering #%d (%d bytes), taskcnt=%d\n",
            ++pktcnt, tasks[0].u.pkt->contlen, taskcnt);
            */
            // DEBUGMSG(INFO, "%s\n", digest2str(tasks[0].md));
 
            write(fd, tasks[0].pkt->content, tasks[0].pkt->contlen);

            WbytesPerRTT += tasks[0].pkt->contlen;
            free_packet(tasks[0].pkt);
            taskcnt--;
            memmove(tasks, tasks + 1, taskcnt*sizeof(struct task_s));

            // activate newly joined pointers:
            for (i = 0; outqlen < outqlimit && i < sendwindow; i++) {
                if (tasks[i].type > 1)
                    continue;
                if (!tasks[i].retry_cnt) {
                    if (packet_request(sock, sa, locator, i))
                        tasks[i].retry_cnt = 2;
                    else
                        tasks[i].retry_cnt = 1;
                }
            }

/*
            if (tasks[0].type == 2) {
                DEBUGMSG(DEBUG, "#0 ready in %ld usec\n", rtt/sendwindow/2);
            } else {
                DEBUGMSG(INFO, "#0 not ready ------------------------\n");
            }
*/
        }

        // re-eval RTT and next deadline
        if (timevaldelta(&now, &endofRTT) > 0) { // one RTT is over
            int newwindow = sendwindow;

            if (rttcnt)
                rtt = 0.8*rtt + 0.2*rttaccu/rttcnt;
            if (rtt < 2000) // minumu rtt (tolerate some local jitter)
                rtt = 2000;
            DEBUGMSG(DEBUG, "new rtt is %ld\n", rtt);
            rttaccu = rttcnt = 0;

            if (dropcnt) {
                newwindow *= 0.75;
            } else if (newwindow < 25) {
                newwindow *= 2;
            } else
                newwindow += 2;
            if (newwindow < 2)
                newwindow = 2;
            if (newwindow > 40)
                newwindow = 40;
            sendwindow = newwindow;
            

            memcpy(&endofRTT, &now, sizeof(now));
            ccnl_timeval_add(&endofRTT, rtt);

            DEBUGMSG(INFO, "rtt=%ld, cwnd=%d, tasks=%d, in=%d, out=%d, qlen=%d, err=%d\n",
                     rtt, sendwindow, taskcnt, RbytesPerRTT, WbytesPerRTT, outqlen, dropcnt);
            sendcnt = 0;
            dropcnt = 0;
            RbytesPerRTT = WbytesPerRTT = 0;

        }

        // treat manifest pointers preferentially
        for (i = 0; outqlen < outqlimit && i < taskcnt; i++) {
            if (i > expandlimit)
                break;
            if (tasks[i].type > 1)
                continue;
            if (tasks[i].type == 1 && !tasks[i].retry_cnt) {
                if (packet_request(sock, sa, locator, i))
                    tasks[i].retry_cnt = 2;
                else
                    tasks[i].retry_cnt = 1;
            }
        }
    }

    return 0;
}

void
window_retrieval(int sock, struct sockaddr *sa, struct ccnl_prefix_s *locator,
                 unsigned char *objHashRestr, int fd)
{
    struct ccnl_pkt_s *pkt;
    struct timeval t1, endofRTT;

    ccnl_get_timeval(&t1);
    pkt = flic_lookup(sock, sa, locator, objHashRestr);
    if (!pkt)
        return;
    ccnl_get_timeval(&endofRTT);
    rtt = timevaldelta(&endofRTT, &t1);
    if (rtt < 20000)
        rtt = 20000;
    DEBUGMSG(DEBUG, "first RTT is %ld usec\n", rtt);
    ccnl_timeval_add(&endofRTT, rtt);

    tasks = ccnl_calloc(1, sizeof(struct task_s));
    taskcnt = 1;
    window_expandManifest(0, pkt, 20);
    free_packet(pkt);

    DEBUGMSG(INFO, "starting with %d lookup tasks\n", taskcnt);
    packet_request(sock, sa, locator, 0);
    tasks[0].retry_cnt = 1;

    repo_io_loop(sock, sa, locator, fd);
}

// ----------------------------------------------------------------------

enum {
    TREE_BALANCED,
    TREE_7MPR,
    TREE_DEEP
};

int
main(int argc, char *argv[])
{
    char *outfname = NULL;
    char *udp = strdup("127.0.0.1/9695"), *ux = NULL;
    struct ccnl_prefix_s *uri = NULL, *metadataUri = NULL;
    int treeShape = TREE_BALANCED, opt, exitBehavior = 0, i;
    struct key_s *keys = NULL;
    int block_size = 4096;
    unsigned char *objHashRestr = NULL;

    progname = argv[0];

    while ((opt = getopt(argc, argv, "hb:d:e:f:k:m:o:p:s:t:u:v:x:")) != -1) {
        switch (opt) {
        case 'b':
            block_size = atoi(optarg);
            if (block_size > (MAX_BLOCK_SIZE-256) || block_size < 64) {
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
            theRepoDir = optarg;
            break;
        case 's':
            theSuite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(theSuite))
                usage(1);
            break;
        case 't':
            if (!strcmp(optarg, "deep"))
                treeShape = TREE_DEEP;
            else if (!strcmp(optarg, "7mpr"))
                treeShape = TREE_7MPR;
            else
                treeShape = TREE_BALANCED;
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
        block_size -= 8 + 8; // nameless obj will be exactly 4096B
        break;
#endif
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
        block_size -= 19; // nameless obj will be exactly 4096B
        break;
#endif
    default:
        DEBUGMSG(FATAL, "unknown suite\n");
        exit(-1);
    }

    if (theRepoDir) {
        struct stat s;
        char *fname = NULL;

        if (stat(theRepoDir, &s)) {
            perror("access to theRepoDir");
            return -1;
        }
        if (!(s.st_mode & S_IFDIR)) {
            DEBUGMSG(FATAL, "<%s> is not a directory\n", theRepoDir);
            return -1;
        }
        if (optind >= argc)
            goto Usage;
        fname = argv[optind++];
        if (optind < argc)
            uri = ccnl_URItoPrefix(argv[optind++], theSuite, NULL, NULL);
        if (optind < argc)
            goto Usage;

        switch (treeShape) {
        case TREE_DEEP:
            flic_produceDeepTree(keys, block_size, fname, uri, metadataUri);
            break;
        case TREE_7MPR:
            flic_produce7mprTree(keys, block_size, fname, uri, metadataUri);
            break;
        case TREE_BALANCED:
            flic_produceBalancedTree(keys, block_size, 4+4,
                                     fname, uri, metadataUri);
        default:
            break;
        }
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

/*
        sequential/non-windowed retrieval:

        if (!flic_do_manifestPtr(sock, &sa, exitBehavior, keys,
                                 uri, objHashRestr, fd, &total)) {
            ccnl_SHA256_Final(md, &total);
            DEBUGMSG(INFO, "Total SHA256 is %s\n", digest2str(md));
        }
*/
        (void)md;
        (void)exitBehavior;
        window_retrieval(sock, &sa, uri, objHashRestr, fd);

        close(fd);

    }

    return 0;
}

// eof
