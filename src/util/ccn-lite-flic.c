/*
 * @f util/ccn-lite-flic.c
 * @b utility for FLIC (FLIC = File-LIke-Collection)
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
// #define USE_SUITE_NDNTLV

#define USE_CCNxMANIFEST
#define USE_HMAC256
#define USE_IPV4
#define USE_NAMELESS

#define MAX_BLOCK_SIZE (64*1024)

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"
#include "ccnl-socket.c"

// ----------------------------------------------------------------------

char *progname;

void
usage(int exitval)
{
    fprintf(stderr, "usage: %s -p TARGETPREFIX [options] FILE [URI]  (producing)\n"
                    "       %s [options] URI                         (retrieving)\n"
            "  -b SIZE    Block size (default is 64K)\n"
            "  -d DIGEST  ObjHashRestriction (32B in hex)\n"
            "  -k FNAME   HMAC256 key (base64 encoded)\n"
            "  -m URI     URI of external metadata\n"
            "  -o FNAME   outfile\n"
            "  -s SUITE   (ccnx2015)\n"
            "  -u a.b.c.d/port  UDP destination\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL   (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
            , progname, progname);
    exit(exitval);
}

// ----------------------------------------------------------------------
// copied from ccn-lite-mkM.c

struct list_s {
    char typ;
    unsigned char *var;
    int varlen;
    struct list_s *first;
    struct list_s *rest;
};

int
emit(struct list_s *lst, unsigned short len, int *offset, unsigned char *buf)
{
    int typ = -1, len2;
    char *cp = "??";

    if (!lst)
        return len;

    DEBUGMSG(TRACE, "   emit starts: len=%d\n", len);
    if (lst->typ) { // atomic
        switch(lst->typ) {
        case 'a':
            cp = "T=about (metadata)";
            typ = CCNX_MANIFEST_HG_METADATA;
            break;
        case 'b':
            cp = "T=meta block size";
            typ = CCNX_MANIFEST_MT_BLOCKSIZE;
            break;
        case 'c': // can be removed for manifests
            cp = "T=content";
            typ = CCNX_TLV_TL_Object;
            break;
        case 'd':
            cp = "T=data hash ptr";
            typ = CCNX_MANIFEST_HG_PTR2DATA;
            break;
        case 'g':
            cp = "T=hash pointer group";
            typ = CCNX_MANIFEST_HASHGROUP;
            break;
        case 'h':
            cp = "T=meta overall hash";
            typ = CCNX_MANIFEST_MT_OVERALLDATASHA256;
            break;
        case 'l':
            cp = "T=meta locator";
            typ = CCNX_MANIFEST_MT_LOCATOR;
            break;
        case 'm':
            cp = "T=manifest";
            typ = CCNX_TLV_TL_Manifest;
            break;
        case 'n':
            cp = "T=name";
            typ = CCNX_TLV_M_Name;
            break;
        case 'p': // can be removed for manifests
            cp = "T=payload";
            typ = CCNX_TLV_M_Payload;
            break;
        case 's':
            cp = "T=meta overall data size";
            typ = CCNX_MANIFEST_MT_OVERALLDATASIZE;
            break;
        case 't':
            cp = "T=tree hash ptr";
            typ = CCNX_MANIFEST_HG_PTR2MANIFEST;
            break;
        case 'x':
            cp = "T=external metadata URI";
            typ = CCNX_MANIFEST_MT_EXTERNALMETADATA;
            break;
        case '@': // blob
            cp = "blob";
            len2 = lst->varlen;
            *offset -= len2;
            memcpy(buf + *offset, lst->var, len2);
            break;
        default:
            DEBUGMSG(FATAL, "unknown type\n");
            break;
        }
        if (typ >= 0) {
            len2 = ccnl_ccntlv_prependTL((unsigned short) typ, len, offset, buf);
            len += len2;
        } else {
            len = len2;
        }
        DEBUGMSG(VERBOSE, "  prepending %s (L=%d), returning %dB\n", cp, len2, len);

        return len;
    }

    // non-atomic
    len2 = emit(lst->rest, 0, offset, buf);
    DEBUGMSG(TRACE, "   rest: len2=%d, len=%d\n", len2, len);
    return emit(lst->first, len2, offset, buf) + len;
}

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

    len = ccnl_ccntlv_prependUInt(val, &offs, tmp);
    return ccnl_manifest_atomic_blob(tmp + offs, len);
}

struct list_s*
ccnl_manifest_atomic_prefix(struct ccnl_prefix_s *pfx)
{
    static unsigned char dummy[256];
    int offset, len;

    offset = sizeof(dummy);
    len = ccnl_ccntlv_prependNameComponents(pfx, &offset, dummy, NULL);
    if (len < 0)
        return 0;

    return ccnl_manifest_atomic_blob(dummy + offset, len);
}

struct list_s*
ccnl_manifest_getEmptyTemplate(void)
{
    struct list_s *m;

    m = ccnl_manifest_atomic("mfst");
    m->rest = ccnl_calloc(1, sizeof(struct list_s));
    m->rest->first = ccnl_manifest_atomic("grp");

    return m;
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

int
flic_namelessObj2file(unsigned char *data, int len,
                      char *fname, unsigned char *digest)
{
    SHA256_CTX_t ctx;
    int f, offset = sizeof(out);

    if ((len+8) > offset)
        return -1;

    len = ccnl_ccntlv_prependContent(NULL, data, len, NULL, NULL,
                                     &offset, out);
    if (len < 0)
        return -1;

    // Compute the hash of the content
    ccnl_SHA256_Init(&ctx);
    ccnl_SHA256_Update(&ctx, out + offset, len);
    ccnl_SHA256_Final(digest, &ctx);
    DEBUGMSG(DEBUG, "  %s (%d)\n", digest2str(digest), len);

    len = ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                      len, 255, &offset, out);
    if (len < 0)
        return -1;

    f = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (f < 0) {
        perror("file open:"); return -1;
    }
    write(f, out + offset, len);
    close(f);

    return len;
}
                            
void
flic_manifest2file(struct list_s *m, struct ccnl_prefix_s *name,
                   struct key_s *keys, char *fname, unsigned char *digest)
{
    SHA256_CTX_t ctx;
    int offset = sizeof(out), mdoffset, mdlen = SHA256_DIGEST_LENGTH, len, f;

    if (name)
        ccnl_manifest_prepend(m, "name", ccnl_manifest_atomic_prefix(name));

    if (keys) {
        offset -= mdlen;
        mdoffset = offset;
        ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationPayload, 32, &offset, out);
        ccnl_ccntlv_prependTL(CCNX_VALIDALGO_HMAC_SHA256, 0, &offset, out);
        ccnl_ccntlv_prependTL(CCNX_TLV_TL_ValidationAlgo, 4, &offset, out);
    }

    len = emit(m, 0, &offset, out);
    ccnl_SHA256_Init(&ctx);
    ccnl_SHA256_Update(&ctx, out + offset, len);
    ccnl_SHA256_Final(digest, &ctx);

    if (keys) {
        // use the first key found in the key file
        unsigned char keyval[64];
        ccnl_hmac256_keyval(keys->key, keys->keylen, keyval);
        ccnl_hmac256_sign(keyval, sizeof(keyval), out + offset, len,
                          out + mdoffset, &mdlen);
        len = sizeof(out) - offset;
    }

    len = ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                      len, 255, &offset, out);

    // Save the packet to disk
    f = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (f < 0) {
        perror("file open:"); return;
    }
    write(f, out + offset, len);
    close(f);
}

// ----------------------------------------------------------------------

void
flic_produceFromFile(int pktype, char *targetprefix,
                     struct key_s *keys, int block_size, char *fname,
                     struct ccnl_prefix_s *name, struct ccnl_prefix_s *meta)
{
    static unsigned char body[MAX_BLOCK_SIZE];
    int f, num_bytes, len, chunk_number, manifest_number = 0;
    SHA256_CTX_t ctx;
    unsigned char md[SHA256_DIGEST_LENGTH], total[SHA256_DIGEST_LENGTH];
    struct list_s *m;
    long length, offset;
    // NOTE: we should play with this
    int max_pointers = 10;
    int num_pointers = 0;
    
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
        fname = NULL;
        asprintf(&fname, "%s-%d", targetprefix, chunk_number);
        flic_namelessObj2file(body, len, fname, md);
        free(fname);

        // Prepend the hash to the current manifest
        ccnl_manifest_prependHash(m, "dptr", md);
        num_pointers++;

        // Finally, check to see if we need to cap the current manifest and
        // move onto the next one
        if (num_pointers == max_pointers || offset == 0) {
            fname = NULL;
            if (offset) {
                asprintf(&fname, "%s-manifest-%d", targetprefix, manifest_number++);
                flic_manifest2file(m, NULL, NULL, fname, md);
            } else {
                asprintf(&fname, "%s-root", targetprefix);
                ccnl_manifest_setMetaData(m, 0, meta, block_size, length,total);
                flic_manifest2file(m, name, keys, fname, md);
            }
            free(fname);

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

struct sockaddr relay;

struct ccnl_pkt_s*
flic_lookup(int sock, struct ccnl_prefix_s *locator,
            unsigned char *objHashRestr)
{
    char dummy[256];
    unsigned char *data;
    unsigned char pkt[4096];
    int datalen, len, rc, cnt;

    DEBUGMSG(TRACE, "lookup %s\n", ccnl_prefix2path(dummy, sizeof(dummy),
                                                       locator));
    len = ccntlv_mkInterest(locator, NULL, objHashRestr, pkt, sizeof(pkt));
    DEBUGMSG(DEBUG, "interest has %d bytes\n", len);

    for (cnt = 0; cnt < 3; cnt++) {
        rc = sendto(sock, pkt, len, 0, &relay, sizeof(relay));
        DEBUGMSG(TRACE, "sendto returned %d\n", rc);
        if (rc < 0) {
            perror("sendto");
            return NULL;
        }
        if (block_on_read(sock, 3) > 0) {
            len = recv(sock, pkt, sizeof(pkt), 0);
            DEBUGMSG(DEBUG, "recv returned %d\n", len);

            data = pkt + 8;
            datalen = len - 8;
            return ccnl_ccntlv_bytes2pkt(pkt, &data, &datalen);
        }
        DEBUGMSG(WARNING, "timeout after block_on_read - %d\n", cnt);
    }
    return 0;
}

void
flic_do_dataPtr(int sock, struct ccnl_prefix_s *locator,
                unsigned char *objHashRestr, int fd, SHA256_CTX_t *total)
{
    struct ccnl_pkt_s *pkt = flic_lookup(sock, locator, objHashRestr);
    write(fd, pkt->content, pkt->contlen);
    ccnl_SHA256_Update(total, pkt->content, pkt->contlen);
}

void
flic_do_manifestPtr(int sock, struct ccnl_prefix_s *locator,
                    unsigned char *objHashRestr, int fd, SHA256_CTX_t *total)
{
    struct ccnl_pkt_s *pkt = flic_lookup(sock, locator, objHashRestr);
    unsigned char *msg;
    int msglen, len, len2;
    unsigned int typ;

    if (!pkt)
        return;
    msg = pkt->buf->data + 8;
    msglen = pkt->buf->datalen - 8;

    if (ccnl_ccntlv_dehead(&msg, &msglen, &typ, &len) < 0)
        goto Bail;
    if (typ != CCNX_TLV_TL_Manifest)
        goto Bail;
    while (ccnl_ccntlv_dehead(&msg, &len, &typ, &len2) >= 0) {
        if (typ != CCNX_MANIFEST_HASHGROUP) {
            msg += len2;
            len -= len2;
            continue;
        }
        DEBUGMSG(TRACE, "hash group\n");
        while (ccnl_ccntlv_dehead(&msg, &len, &typ, &len2) >= 0) {
            if (len2 == SHA256_DIGEST_LENGTH) {
                if (typ == CCNX_MANIFEST_HG_PTR2DATA)
                    flic_do_dataPtr(sock, NULL, msg, fd, total);
                else if (typ == CCNX_MANIFEST_HG_PTR2MANIFEST)
                    flic_do_manifestPtr(sock, NULL, msg, fd, total);
            }
            msg += len2;
            len -= len2;
        }
    }
    return;
Bail:
    DEBUGMSG(DEBUG, "do_manifestPtr had a problem\n");
    return;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    char *targetprefix = NULL, *outfname = NULL;
    char *udp = strdup("127.0.0.1/9695");
    struct ccnl_prefix_s *uri = NULL, *metadataUri = NULL;
    int opt, packettype = CCNL_SUITE_CCNTLV, i;
    struct key_s *keys = NULL;
    int block_size = 4096 - 8 - 8; // nameless obj will be exactly 4096B
    unsigned char *objHashRestr = NULL;

    progname = argv[0];

    while ((opt = getopt(argc, argv, "hb:d:f:k:m:o:p:s:u:v:")) != -1) {
        switch (opt) {
        case 'b':
            block_size = atoi(optarg);
            if (block_size > (MAX_BLOCK_SIZE-256)) {
                DEBUGMSG(FATAL, "Error: block size cannot exceed %d\n", MAX_BLOCK_SIZE - 256);
                goto Usage;
            }
            break;
        case 'd':
            if (strlen(optarg) != 64)
                break;
            ccnl_free(objHashRestr);
            objHashRestr = ccnl_malloc(SHA256_DIGEST_LENGTH);
            for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                objHashRestr[i] = hex2int(optarg[2*i]) * 16 +
                                                   hex2int(optarg[2*i + 1]);
            }
            break;
        case 'k':
            keys = load_keys_from_file(optarg);
            break;
        case 'm':
            metadataUri = ccnl_URItoPrefix(optarg, CCNL_SUITE_CCNTLV,NULL,NULL);
            break;
        case 'o':
            outfname = optarg;
            break;
        case 'p':
            targetprefix = optarg;
            break;
        case 's':
            packettype = ccnl_str2suite(optarg);
            if (packettype >= 0 && packettype < CCNL_SUITE_LAST)
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
        case 'h':
        default:
Usage:
            usage(1);
        }
    }

    if (targetprefix) {
        char *fname = NULL;
        
        if (optind >= argc)
            goto Usage;
        fname = argv[optind++];
        if (optind < argc)
            uri = ccnl_URItoPrefix(argv[optind++], CCNL_SUITE_CCNTLV,NULL,NULL);
        if (optind < argc)
            goto Usage;

        flic_produceFromFile(packettype, targetprefix, keys, block_size,
                             fname, uri, metadataUri);
    } else { // retrieve a manifest tree
        int fd, port, sock;
        const char *addr = NULL;
        struct sockaddr_in *si;
        SHA256_CTX_t total;
        unsigned char md[SHA256_DIGEST_LENGTH];

        if (optind >= argc)
            goto Usage;
        uri = ccnl_URItoPrefix(argv[optind++], CCNL_SUITE_CCNTLV, NULL, NULL);
        if (optind < argc)
            goto Usage;

        if (ccnl_parseUdp(udp, CCNL_SUITE_CCNTLV, &addr, &port) != 0) {
            exit(-1);
        }
        DEBUGMSG(TRACE, "using udp address %s/%d\n", addr, port);
        si = (struct sockaddr_in*) &relay;
        ccnl_setIpSocketAddr(si, addr, port);
        sock = udp_open();

        if (outfname)
            fd = open(outfname, O_CREAT | O_TRUNC | O_WRONLY, 0666);
        else
            fd = 1;

        ccnl_SHA256_Init(&total);
        flic_do_manifestPtr(sock, uri, objHashRestr, fd, &total);
        close(fd);

        ccnl_SHA256_Final(md, &total);
        DEBUGMSG(INFO, "Total SHA256 is %s\n", digest2str(md));
    }

    return 0;
}

// eof
