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

struct chunk {
    char data[CCNL_MAX_CHUNK_SIZE];
    int len;
    struct chunk *next;
};

int
main(int argc, char *argv[])
{
    // char *private_key_path = 0;
    //    char *witness = 0;
    unsigned char out[65*1024];
    char *publisher = 0;
    char *infname = 0, *outdirname = 0;
    char chunkname[10] = "c";
    char fileext[10];
    char chunkname_with_number[1024];
    char final_chunkname_with_number[1024];
    int f, fout, chunk_len, contentlen = 0, opt, plen;
    int suite = CCNL_SUITE_DEFAULT;
    int chunk_size = CCNL_MAX_CHUNK_SIZE;
    int status;
    struct ccnl_prefix_s *name;
    struct stat st_buf;

    while ((opt = getopt(argc, argv, "hc:i:o:p:k:w:s:")) != -1) {
        switch (opt) {
        case 'c':
            chunk_size = atoi(optarg);
            if(chunk_size > CCNL_MAX_CHUNK_SIZE) chunk_size = CCNL_MAX_CHUNK_SIZE;
            break;
        case 'i':
            infname = optarg;
            break;
/*
        case 'k':
            private_key_path = optarg;
            break;
        case 'w':
            witness = optarg;
            break;
*/
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
        case 's':
            suite = ccnl_str2suite(optarg);
            if (suite >= 0 && suite < CCNL_SUITE_LAST)
                break;
        case 'h':
        default:
Usage:
        fprintf(stderr, 
        "create content object chunks for the input data and writes them "
        "to the files into the given directory.\n"
        "usage: %s [options] OUTDIR URL [NFNexpr]\n"
        "  -c chunk_size size for each chunk (max %d)\n"
        "  -i FNAME   input file (instead of stdin)\n"
        // "  -k FNAME   publisher private key\n"
        "  -p DIGEST  publisher fingerprint\n"
        "  -s SUITE   (ccnb, ccnx2014, ndn2013)\n"
        // "  -w STRING  witness\n"       
        ,
        argv[0],
        CCNL_MAX_CHUNK_SIZE);
        exit(1);
        }
    }


    if (!argv[optind])
        goto Usage;
    outdirname = argv[optind];
    optind++;

    // url required
    if (!argv[optind])
        goto Usage;
    int urllen = strlen(argv[optind]);
    char url_orig[urllen];
    char url[urllen];
    strcpy(url_orig, argv[optind]);

    // OUTIDR required
    if (!argv[optind]) {
        goto Usage;
    }

    // Check if outdirname is a directory and open it as a file
    status = stat(outdirname, &st_buf);
    if (status != 0) {
        DEBUGMSG (99, "Error (%d) when opening file %s\n", errno, outdirname);
        return 1;
    }

    if (S_ISREG (st_buf.st_mode)) {
        DEBUGMSG (99, "Error: %s is a file and not a directory.\n", argv[optind]);
        goto Usage;
    }
/*
    if (S_ISDIR (st_buf.st_mode)) {
        fdir = open(outdirname, O_RDWR);
    }
*/
    if (infname) {
        f = open(infname, O_RDONLY);
        if (f < 0) {
            perror("file open:");
        }
    } else {
      f = 0;
    }



    char outfilename[255];
    char chunk_buf[chunk_size];
    int is_last = 0;
    struct chunk *first_chunk = NULL;
    struct chunk *cur_chunk = NULL;
    struct chunk *chunk = NULL;
    int num_chunks = 0;

    do {
        chunk_len = read(f, chunk_buf, chunk_size);

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

    sprintf(final_chunkname_with_number, "%s%i", chunkname, num_chunks - 1);
    char *chunk_data = NULL;
    int chunknum = 0;
    int lastchunknum = num_chunks - 1;
    int offs = -1;

    switch(suite) {
        case CCNL_SUITE_CCNB:
            strcpy(fileext, "ccnb");
            break;
        case CCNL_SUITE_CCNTLV: 
            strcpy(fileext, "ccntlv");
            break;
        case CCNL_SUITE_NDNTLV:
            strcpy(fileext, "ndntlv");
            break;
        default:
            DEBUGMSG(99, "fileext for suite %d not implemented", suite);
    }


    while(cur_chunk != NULL) {
        chunk_data = cur_chunk->data;
        chunk_len = cur_chunk->len;

        strcpy(url, url_orig);
        offs = CCNL_MAX_PACKET_SIZE;
        switch(suite) {
        case CCNL_SUITE_CCNTLV: 

            name = ccnl_URItoPrefix(url, suite, argv[optind+1]);

            DEBUGMSG(99, "prefix: '%s'\n", ccnl_prefix_to_path(name));

            contentlen = ccnl_ccntlv_fillContentWithHdr(name, 
                                                        (unsigned char *)chunk_data, chunk_len, 
                                                        &chunknum, &lastchunknum,
                                                        &offs, 
                                                        NULL, // int *contentpos
                                                        out);
            break;
        case CCNL_SUITE_NDNTLV:
            sprintf(chunkname_with_number, "%s/%s%d", url, chunkname, chunknum);
            name = ccnl_URItoPrefix(chunkname_with_number, suite, argv[optind+1]);
            contentlen = ccnl_ndntlv_fillContent(name, 
                                                 (unsigned char *) chunk_data, chunk_len, 
                                                 &offs, NULL,
                                                 (unsigned char *) final_chunkname_with_number, strlen(final_chunkname_with_number), 
                                                 out);
            break;
        default:
            DEBUGMSG(99, "produce for suite %i is not implemented\n", suite);
            goto Error;
            break;
        }

        sprintf(outfilename, "%s/%s%d.%s", outdirname, chunkname, chunknum, fileext);

        DEBUGMSG(99, "writing to %s for chunk %d\n", outfilename, chunknum);

        fout = creat(outfilename, 0666);
        write(fout, out + offs, contentlen);
        close(fout);
        chunknum++;
        cur_chunk = cur_chunk->next;
    }
    return 0;

Error:
    return -1;
}

// eof
