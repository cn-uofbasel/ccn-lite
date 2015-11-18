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
#define USE_NAMELESS

#define MAX_BLOCK_SIZE (64*1024)

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"

// ----------------------------------------------------------------------

char *progname;

void
usage(int exitval)
{
    fprintf(stderr, "usage: %s -p TARGETPREFIX [options] FILE [URI]\n"
            "  -b SIZE    Block size (default is 64K)\n"
            "  -k FNAME   HMAC256 key (base64 encoded)\n"
            "  -s SUITE   (ccnx2015)\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
            , progname);
    exit(exitval);
}

// ----------------------------------------------------------------------
// copied from ccn-lite-mkM.c

struct list_s {
    char *var;
    struct list_s *first;
    struct list_s *rest;
};

int
emit(struct list_s *lst, unsigned short len, int *offset, unsigned char *buf)
{
    int typ = -1, len2;
    char *cp = "??", dummy[200];
    struct ccnl_prefix_s *pfx = NULL;

    if (!lst)
        return len;

    DEBUGMSG(TRACE, "   emit starts: len=%d\n", len);
    if (lst->var) { // atomic
        switch(lst->var[0]) {
        case 'a':
            cp = "T=about (metadata)";
            typ = CCNX_MANIFEST_HG_METADATA;
            break;
        case 'b':
            cp = "T=block size";
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
        case 'l':
            cp = "T=locator";
            typ = CCNX_MANIFEST_MT_NAME;
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
            cp = "T=data size";
            typ = CCNX_MANIFEST_MT_OVERALLDATASIZE;
            break;
        case 't':
            cp = "T=tree hash ptr";
            typ = CCNX_MANIFEST_HG_PTR2MANIFEST;
            break;
        case '/':
            pfx = ccnl_URItoPrefix(lst->var, CCNL_SUITE_CCNTLV, NULL, NULL);
            cp = ccnl_prefix2path(dummy, sizeof(dummy), pfx);
            len2 = ccnl_ccntlv_prependNameComponents(pfx, offset, buf, NULL);
            break;
        case '0':
            cp = "hash value";
            *offset -= SHA256_DIGEST_LENGTH;
            // hash2digest(buf + *offset, lst->var + 2); // start after "0x"
            memcpy(buf + *offset, lst->var, SHA256_DIGEST_LENGTH);
            len2 = SHA256_DIGEST_LENGTH;
            break;
        default:
            if (isdigit(lst->var[0])) {
                int v = atoi(lst->var);
                len2 = ccnl_ccntlv_prependUInt((unsigned int)v, offset, buf);
                cp = "int value";
            } else { // treat as a blob
                len2 = strlen(lst->var);
                *offset -= len2;
                memcpy(buf + *offset, lst->var, len2);
            }
            break;
        }
        if (typ >= 0) {
            len2 = ccnl_ccntlv_prependTL((unsigned short) typ, len, offset, buf);
            len += len2;
        } else {
            len = len2;
        }
        DEBUGMSG(DEBUG, "  prepending %s (L=%d), returning %dB\n", cp, len2, len);
        
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
    if (t->var) {
        strcat(out, t->var);
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
    // note: var is not freed upon freeing this node
    struct list_s *node;

    node = ccnl_calloc(1, sizeof(*node));
    node->first = ccnl_calloc(1, sizeof(*var));
    node->first->var = var;
    return node;
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

// typ is either "dptr" or "tptr", for data hash ptr or tree/manifest hash ptr
// md is the hash value (array of 32 bytes) to be stored
void
ccnl_manifest_prependHash(struct list_s *m, char *typ, char *md)
{
    // note: typ and md are not freed upon freeing this list
    struct list_s *grp = m->rest->first;
    struct list_s *hentry = ccnl_calloc(1, sizeof(*hentry));

    hentry->first = ccnl_manifest_atomic(typ);
    hentry->first->rest = ccnl_manifest_atomic(md);
    hentry->rest = grp->rest;
    grp->rest = hentry;
}


void
ccnl_manifest_setMetaData(struct list_s *m, int blockSize,
                          int totalSize, unsigned char* totalDigest)
{
}

void
ccnl_manifest_free(struct list_s *m)
{
    if (!m)
        return;
    ccnl_manifest_free(m->first);
    ccnl_manifest_free(m->rest);
    ccnl_free(m);
}

// serializes the manifest, returns its digest
int
ccnl_manifest2packet(struct list_s *m, int *offset, unsigned char *buf,
                     unsigned char *digest)
{
    int oldoffset = *offset, len;
    SHA256_CTX_t ctx;

    len = emit(m, 0, offset, buf);
    ccnl_SHA256_Init(&ctx);
    ccnl_SHA256_Update(&ctx, buf + *offset, len);
    ccnl_SHA256_Final(digest, &ctx);

    ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                len, 255, offset, buf);
    
    return oldoffset - *offset;
}

void
ccnl_manifest_test()
{
    unsigned char buf[2014], md[SHA256_DIGEST_LENGTH];
    int offset = sizeof(buf), len;
    struct list_s *m;
//    char out[1024];

    m = ccnl_manifest_getEmptyTemplate();
//    out[0] = '\0'; ccnl_printExpr(m, '(', out); fprintf(stderr, "%s\n", out);
    ccnl_manifest_prependHash(m, "dptr", "0xabcd");
    ccnl_manifest_prependHash(m, "dptr", "0xef01");
    ccnl_manifest_prependHash(m, "tptr", "0x2345");
//    out[0] = '\0'; ccnl_printExpr(m, '(', out); fprintf(stderr, "%s\n", out);

    len = ccnl_manifest2packet(m, &offset, buf, md);
    write(1, buf+offset, len);

    fprintf(stderr, "--> 0x");
    for (len = 0; len < (int)sizeof(md); len++)
        fprintf(stderr, "%02x", md[len]);
    fprintf(stderr, "\n");

    ccnl_manifest_free(m);
    fprintf(stderr, "done\n");
}

// ----------------------------------------------------------------------

int
flic_produceFromFile(int pktype, char *targetprefix, struct key_s *keys, int block_size,
    char *fname, char *uri)
{
    FILE *f;
    int num_bytes, len, i;
    // int i, f, len, offs, msgLen;
    // struct ccnl_prefix_s *name = NULL;
    SHA256_CTX_t ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];

    // int num_pointers = 1;
    // int *ptypes = ccnl_malloc(num_pointers * sizeof(ptypes));
    // struct ccnl_prefix_s **names = ccnl_malloc(num_pointers * sizeof(struct ccnl_prefix_s));
    // unsigned char **digests = ccnl_malloc(num_pointers * sizeof(unsigned char*));

    f = fopen(fname, "rb");
    if (f == NULL) {
        perror("file open:");
        return -1;
    }

    // get the number of blocks (leaves)
    fseek(f, 0, SEEK_END);
    long length = ftell(f);

    DEBUGMSG(INFO, "Creating FLIC from %s\n", fname);
    DEBUGMSG(INFO, "... file size  = %ld\n", length);
    DEBUGMSG(INFO, "... block size = %d\n", block_size);

    int num_blocks = ((length - 1) / block_size) + 1;
    int offset = length - block_size;
    if (length % block_size != 0) {
        offset = length - (length % block_size);
    }

    // TODO: need to correct this
    // int max_pointers = 0;

    unsigned char *body = ccnl_malloc(MAX_BLOCK_SIZE);
    unsigned char *data_out = ccnl_malloc(MAX_BLOCK_SIZE);

    int chunk_number = num_blocks;
    int index = 1;
    for (; offset >= 0; offset -= block_size, chunk_number--, index++) {
        DEBUGMSG(DEBUG, "Processing block at offset %d\n", offset);
        memset(body, 0, block_size);
        fseek(f, block_size * (index * -1), SEEK_END);
        num_bytes = fread(body, sizeof(unsigned char), block_size, f);
        if (num_bytes != block_size) {
            DEBUGMSG(FATAL, "Read from offset %d failed.\n", offset);
            return -1;
        }

        DEBUGMSG(DEBUG, "Creating a data packet with %d bytes\n", num_bytes);

        int offs = MAX_BLOCK_SIZE;
        // memset(data_out, 0, MAX_BLOCK_SIZE);
        len = ccnl_ccntlv_prependContentWithHdr(NULL, body, num_bytes, NULL, NULL, &offs, data_out);
        if (len == -1) {
            DEBUGMSG(FATAL, "wtf!\n");
            return -1;
        }
        DEBUGMSG(DEBUG, "new offs = %d\n", offs);

        char *chunk_fname = NULL;
        asprintf(&chunk_fname, "%s-%d", targetprefix, chunk_number);
        FILE *cf = fopen(chunk_fname, "wb");
        if (cf == NULL) {
            perror("file open:");
            return -1;
        }

        fwrite(data_out + offs, 1, len, cf);
        fclose(cf);

        ccnl_SHA256_Init(&ctx);
        ccnl_SHA256_Update(&ctx, data_out + MAX_BLOCK_SIZE - len, len);
        ccnl_SHA256_Final(md, &ctx);
        {
            char tmp[256];
            int u;
            u = sprintf(tmp, "  ObjectHash%d is 0x", SHA256_DIGEST_LENGTH);
            for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
                u += sprintf(tmp+u, "%02x", md[i]);
            DEBUGMSG(VERBOSE, "%s\n", tmp);
        }
    }

    // keep a current manifest until it fills up, hash it, and then add it to the next current manifest

    //
    // for (i = 0; i < num_pointers; i++) {
    //     digests[i] = ccnl_malloc(SHA256_DIGEST_LENGTH);
    //
    //     char *fname = fnames[i];
    //     if (fname) {
    //         f = open(fname, O_RDONLY);
    //         if (f < 0) {
    //             perror("file open:");
    //             return -1;
    //         }
    //
    //         // Read in the content object (or manifest, doesn't matter...)
    //         len = read(f, body, sizeof(body));
    //         close(f);
    //         memset(out, 0, sizeof(out));
    //
    //         // Parse the packet
    //         int skip;
    //         ccnl_pkt2suite(body, len, &skip);
    //
    //         unsigned char *start, *data;
    //         data = start = body + skip;
    //         len -= skip;
    //
    //         int hdrlen = ccnl_ccntlv_getHdrLen(data, len);
    //         data += hdrlen;
    //         len -= hdrlen;
    //
	//     unsigned char *copydata = data;
	//     int copylen = len;
    //
    //         // Convert the raw bytes to a ccnl packet type
    //         struct ccnl_pkt_s *pkt = ccnl_ccntlv_bytes2pkt(start, &copydata, &copylen);
    //
	//     printf("Parsing the %dth packet\n", i);
    //
    //         int tl_type = 0;
    //         int tl_length = 0;
    //         if (ccnl_ccntlv_dehead(&data, &len, (unsigned int *) &tl_type, &tl_length) < 0) {
	// 	printf("An error occurred!\n");
    //             return -1;
	//     }
    //
    //         printf("TYPE = %d\n", tl_type);
    //
    //         // Extract the name, type, and digest to construct the pointer.
    //         ptypes[i] = tl_type;
    //         names[i] = pkt->pfx;
    //         memcpy(digests[i], pkt->md, SHA256_DIGEST_LENGTH);
    //     } else {
    //         return -1;
    //     }
    // }
    //
    // // Now, build the manifest from the ptypes, names, and digests
    // if (uri) {
    //     printf("manifest has a name: %s\n", uri);
    //     name = ccnl_URItoPrefix(uri, CCNL_SUITE_CCNTLV, NULL, NULL);
    //     if (!name)
    //         return -1;
    // }
    //
    // printf("Encoding the FLIC...\n");
    // offs = sizeof(out); // set to the end of the buffer
    // len = ccnl_ccntlv_prependManifestWithHdr(name, &ptypes, names, &digests, SHA256_DIGEST_LENGTH,
    //     &num_pointers, 1, &offs, out, &msgLen);
    //
    // if (len <= 0) {
    //     DEBUGMSG(ERROR, "internal error: empty packet\n");
    //     return -1;
    // }
    //
    // if (outprefix) {
    //     f = creat(outprefix, 0666);
    //     if (f < 0) {
    //         perror("file open:");
    //         return -1;
    //     }
    // } else
    //     f = 1;
    //
    // write(f, out + offs, len);
    // close(f);
    //
    // ccnl_SHA256_Init(&ctx);
    // ccnl_SHA256_Update(&ctx, out + sizeof(out) - msgLen, msgLen);
    // ccnl_SHA256_Final(md, &ctx);
    // fprintf(stderr, "  ObjectHash%d is 0x", SHA256_DIGEST_LENGTH);
    // for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
    //     fprintf(stderr, "%02x", md[i]);
    // fprintf(stderr, "\n");

    return 0;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    char *targetprefix = NULL;
    int opt, packettype = CCNL_SUITE_CCNTLV;
    struct key_s *keys = NULL;
    int block_size = MAX_BLOCK_SIZE - 256; // 64K by default

    progname = argv[0];

    while ((opt = getopt(argc, argv, "hk:p:s:v:f:b:")) != -1) {
        switch (opt) {
        case 'k':
            keys = load_keys_from_file(optarg);
            break;
        case 'p':
            targetprefix = optarg;
            break;
        case 's':
            packettype = ccnl_str2suite(optarg);
            if (packettype >= 0 && packettype < CCNL_SUITE_LAST)
                break;
        case 'b':
            block_size = atoi(optarg);
            if (block_size > (MAX_BLOCK_SIZE-256)) {
                DEBUGMSG(FATAL, "Error: block size cannot exceed %d\n", MAX_BLOCK_SIZE - 256);
                goto Usage;
            }
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

    ccnl_manifest_test();
    exit(1);

    if (targetprefix) {
      	char *uri = NULL;
        char *fname = NULL;
        if (optind >= argc)
            goto Usage;
        fname = argv[optind++];
        if (optind < argc)
            uri = argv[optind++];
        if (optind < argc)
            goto Usage;

        flic_produceFromFile(packettype, targetprefix, keys, block_size, fname, uri);
    } else {
        goto Usage;
    }

    return 0;
}

// eof
