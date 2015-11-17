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

    printf("Creating FLIC from %s\n", fname);
    printf("... file size  = %ld\n", length);
    printf("... block size = %d\n", block_size);

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
        printf("Processing block at offset %d\n", offset);
        memset(body, 0, block_size);
        fseek(f, block_size * (index * -1), SEEK_END);
        num_bytes = fread(body, sizeof(unsigned char), block_size, f);
        if (num_bytes != block_size) {
            printf("Read from offset %d failed.\n", offset);
            return -1;
        }

        printf("Creating a data packet with %d bytes\n", num_bytes);

        int offs = MAX_BLOCK_SIZE;
        memset(data_out, 0, MAX_BLOCK_SIZE);
        len = ccnl_ccntlv_prependContentWithHdr(NULL, body, num_bytes, NULL, NULL, &offs, data_out);
        if (len == -1) {
            printf("wtf!\n");
            return -1;
        }
        printf("new offs = %d\n", offs);

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
        fprintf(stderr, "  ObjectHash%d is 0x", SHA256_DIGEST_LENGTH);
        for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
            fprintf(stderr, "%02x", md[i]);
        fprintf(stderr, "\n");
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
    int block_size = MAX_BLOCK_SIZE; // 64K by default

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
            if (block_size > MAX_BLOCK_SIZE) {
                printf("Error: block size cannot exceed %d\n", MAX_BLOCK_SIZE);
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

    if (targetprefix) {
      	char *uri = NULL;
        char *fname = NULL;
        if (optind < argc - 1) {
            fname = argv[optind];
            uri = argv[optind + 1];
        } else {
            goto Usage;
        }

        flic_produceFromFile(packettype, targetprefix, keys, block_size, fname, uri);
    } else {
        goto Usage;
    }

    return 0;
}

// eof
