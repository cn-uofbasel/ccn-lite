/*
 * @f util/ccn-lite-produce.c
 * @b CLI produce, produce segmented content for file
 *
 * Copyright (C) 2013, Basil Kohler, University of Basel
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
 * 2014-09-01 created <basil.kohler@unibas.ch>
 */

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV
 
#define USE_SIGNATURES

#define CCNL_MAX_CHUNK_SIZE 4048
#define CCNL_MIN_CHUNK_SIZE 1

#include "ccnl-common.c"
#include "ccnl-crypto.c"

#ifdef XXX
int
ccnl_ccnb_mkContent(char **namecomp,
      unsigned char *publisher, int plen,
      unsigned char *body, int blen,
      char *private_key_path,
      char *witness,
      unsigned char *out)
{
    int len = 0, k;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // interest

    // add signature
#ifdef USE_SIGNATURES
    if(private_key_path)
        len += add_signature(out+len, private_key_path, body, blen);  
#endif
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    k = strlen(*namecomp);
    len += ccnl_ccnb_mkHeader(out+len, k, CCN_TT_BLOB);
    memcpy(out+len, *namecomp++, k);
    len += k;
    out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    if (publisher) {
    struct timeval t;
    unsigned char tstamp[6];
    uint32_t *sec;
    uint16_t *secfrac;

    gettimeofday(&t, NULL);
    sec = (uint32_t*)(tstamp + 0); // big endian
    *sec = htonl(t.tv_sec);
    secfrac = (uint16_t*)(tstamp + 4);
    *secfrac = htons(4048L * t.tv_usec / 1000000);
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_TIMESTAMP, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, sizeof(tstamp), CCN_TT_BLOB);
    memcpy(out+len, tstamp, sizeof(tstamp));
    len += sizeof(tstamp);
    out[len++] = 0; // end-of-timestamp

    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_SIGNEDINFO, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, plen, CCN_TT_BLOB);
    memcpy(out+len, publisher, plen);
    len += plen;
    out[len++] = 0; // end-of-publisher
    out[len++] = 0; // end-of-signedinfo
    }

    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, blen, CCN_TT_BLOB);
    memcpy(out + len, body, blen);
    len += blen;
    out[len++] = 0; // end-of-content

    out[len++] = 0; // end-of-contentobj

    return len;
}

#endif

struct chunk {
    char data[CCNL_MAX_CHUNK_SIZE];
    int len;
    struct chunk *next;
};

int
main(int argc, char *argv[])
{
    char *private_key_path; 
    char *witness;
    unsigned char out[65*1024];
    char *publisher = 0;
    char *infname = 0, *outdirname = 0;
    char chunkname[10] = "c";
    char chunkname_with_number[20];
    char final_chunkname_with_number[20];
    int i = 0, f, fdir, fout, chunk_len, contentlen = 0, opt, plen;
    int packettype = 2;
    int status;
    struct ccnl_prefix_s *name;
    struct stat st_buf;
    int chunkSize = 0;
    private_key_path = 0;
    witness = 0;

    while ((opt = getopt(argc, argv, "hi:o:p:k:w:s:")) != -1) {
        switch (opt) {
        case 'i':
            infname = optarg;
            break;
        case 's':
            packettype = atoi(optarg);
            break;
        case 'k':
            private_key_path = optarg;
            break;
        case 'w':
            witness = optarg;
            break;
        case 'p':
            publisher = optarg;
            plen = unescape_component(publisher);
            if (plen != 32) {
            fprintf(stderr,
             "publisher key digest has wrong length (%d instead of 32)\n",
             plen);
            exit(-1);
            }
            break;
        case 'h':
        default:
Usage:
        fprintf(stderr, 
        "create content object chunks for the input data and writes them "
        "to the files into the given directory.\n"
        "usage: %s [options] OUTDIR URI [NFNexpr]\n"
        "  -s SUITE   0=ccnb, 1=ccntlv, 2=ndntlv (default)\n"
        "  -i FNAME   input file (instead of stdin)\n"
        "  -p DIGEST  publisher fingerprint\n"
        "  -k FNAME   publisher private key\n"
        "  -w STRING  witness\n"       
        ,
        argv[0]);
        exit(1);
        }
    }

    // URI required
    if (!argv[optind])
        goto Usage;

    outdirname = argv[optind];
    printf("outdirname: %s\n", outdirname);
    optind++;

    if (!argv[optind])
        goto Usage;
    char uri[strlen(argv[optind]) ];
    strcpy(uri, argv[optind]);
    // char uriCopy[strlen(argv[optind])];

    // OUTIDR required
    if (!argv[optind]) {
        goto Usage;
    }

    // Check if outdirname is a directory and open it as a file
    status = stat(outdirname, &st_buf);
    if (status != 0) {
        printf ("Error (%d) when opening file %s\n", errno, outdirname);
        return 1;
    }

    if (S_ISREG (st_buf.st_mode)) {
        printf ("Error: %s is a file and not a directory.\n", argv[optind]);
        goto Usage;
    }
    if (S_ISDIR (st_buf.st_mode)) {
        fdir = open(outdirname, O_RDWR);
    }

    if (infname) {
        f = open(infname, O_RDONLY);
        if (f < 0) {
            perror("file open:");
        }
    } else {
      f = 0;
    }

    // TODO add flag for var max chunk size (must be smaller than max chunk size)
    chunkSize = CCNL_MAX_CHUNK_SIZE;
    chunkSize = 5;


    char outfilename[255];
    char chunk_buf[chunkSize];
    int is_last = 0;
    struct chunk *first_chunk = NULL;
    struct chunk *cur_chunk = NULL;
    struct chunk *chunk = NULL;
    int num_chunks = 0;

    do {
        chunk_len = read(f, chunk_buf, chunkSize);

        // Remove linefeed, found last chunk
        if(chunk_buf[chunk_len-1] == 10) {
            chunk_len--;
            is_last = 1;
        }
        if(chunk_len <= 0) {
            break;
        }

        num_chunks += 1;

        chunk = malloc(sizeof(struct chunk));
        strcpy(chunk->data, chunk_buf);
        chunk->len = chunk_len;
        chunk->next = NULL;

        if(cur_chunk == NULL) {
            first_chunk = chunk;
        } else {
            cur_chunk->next = chunk;
        }
        cur_chunk = chunk;
    } while(!is_last);
    close(f);

    cur_chunk = first_chunk;

    strcpy(final_chunkname_with_number, chunkname);
    sprintf(final_chunkname_with_number + strlen(final_chunkname_with_number), "%i", num_chunks - 1);
    i = 0;
    char *chunk_data = NULL;
    int offs = -1;



    while(cur_chunk != NULL) {
        chunk_data = cur_chunk->data;
        chunk_len = cur_chunk->len;

        printf("chunk data: %s\n", chunk_data);

        strcpy(chunkname_with_number, uri);
        strcat(chunkname_with_number, "/");
        strcat(chunkname_with_number, chunkname);
        sprintf(chunkname_with_number + strlen(chunkname_with_number), "%i", i);

        name = ccnl_URItoPrefix(chunkname_with_number, packettype, argv[optind+1]);

        offs = CCNL_MAX_PACKET_SIZE;
        switch(packettype) {
        case CCNL_SUITE_CCNB:
            contentlen = ccnl_ccnb_fillContent(name, (unsigned char *)chunk_data, chunk_len, NULL, out);
            break;
        case CCNL_SUITE_CCNTLV: 
            contentlen = ccnl_ccntlv_fillContentWithHdr(name, (unsigned char *)chunk_data, chunk_len, &offs, NULL, out);
            break;
        case CCNL_SUITE_NDNTLV:
            contentlen = ccnl_ndntlv_fillContent(name, 
                                                 (unsigned char *) chunk_data, chunk_len, 
                                                 &offs, NULL,
                                                 (unsigned char *) final_chunkname_with_number, strlen(final_chunkname_with_number), 
                                                 out);
            break;
        default:
            fprintf(stderr, "encoding for suite %i is not implemented\n", packettype);
            break;
        }

        strcpy(outfilename, outdirname);
        strcat(outfilename, "/");
        strcat(outfilename, chunkname);
        sprintf(outfilename + strlen(outfilename), "%i", i);

        fout = creat(outfilename, 0666);
        write(fout, out + offs, contentlen);
        close(fout);
        i++;
        cur_chunk = cur_chunk->next;
    }
    return 0;
}

// eof
