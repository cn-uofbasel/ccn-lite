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
    char *infname = 0, *outdirname = 0, *outfname;
    int f, fout, contentlen = 0, opt, plen;
    int suite = CCNL_SUITE_DEFAULT;
    int chunk_size = CCNL_MAX_CHUNK_SIZE;
    struct ccnl_prefix_s *name;

    debug_level = 99;

    while ((opt = getopt(argc, argv, "hc:f:i:o:p:k:w:s:")) != -1) {
        switch (opt) {
        case 'c':
            chunk_size = atoi(optarg);
            if (chunk_size > CCNL_MAX_CHUNK_SIZE) 
                chunk_size = CCNL_MAX_CHUNK_SIZE;
            break;
        case 'f':
            outfname = optarg;
            break;
        case 'i':
            infname = optarg;
            break;
        case 'o':
            outdirname = optarg;
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
            break;
        case 'h':
        default:
Usage:
        fprintf(stderr, 
        "Creates a chunked content object stream for the input data and writes them to stdout.\n"
        "usage: %s [options] URL\n"
        "  -c SIZE          size for each chunk (max %d)\n"
        "  -f FNAME         filename of the chunks when using -o\n"
        "  -i FNAME         input file (instead of stdin)\n"
        "  -o DIR           output dir (instead of stdout), filename default is cN, otherwise specify -f\n"
        "  -p DIGEST        publisher fingerprint\n"
        "  -s SUITE         (ccnb, ccnx2014, ndn2013)\n"
        ,
        argv[0],
        CCNL_MAX_CHUNK_SIZE);
        exit(1);
        }
    }

    if (suite < 0 || suite >= CCNL_SUITE_LAST)
        goto Usage;

    // mandatory url 
    if (!argv[optind])
        goto Usage;

    char *url_orig = argv[optind];
    char url[strlen(url_orig)];
    optind++;

    // optional nfn 
    char *nfnexpr = argv[optind];

    // int status;
    // struct stat st_buf;
    // Check if outdirname is a directory and open it as a file
    // status = stat(outdirname, &st_buf);
    // if (status != 0) {
    //     DEBUGMSG (99, "Error (%d) when opening file %s\n", errno, outdirname);
    //     return 1;
    // }
    // if (S_ISREG (st_buf.st_mode)) {
    //     DEBUGMSG (99, "Error: %s is a file and not a directory.\n", argv[optind]);
    //     goto Usage;
    // }
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
    char *chunk_buf, *next_chunk_buf, *temp_chunk_buf;
    chunk_buf = ccnl_malloc(chunk_size * sizeof(unsigned char));
    next_chunk_buf = ccnl_malloc(chunk_size * sizeof(unsigned char));

    int chunk_len, next_chunk_len;
    int is_last = 0;
    unsigned int chunknum = 0;

    // sprintf(final_chunkname_with_number, "%s%i", outfname, num_chunks - 1);
    char *chunk_data = NULL;
    int offs = -1;

    char default_file_name[1] = "c";
    if(!outfname) {
        outfname = default_file_name;
    }

    char fileext[10];
    switch (suite) {
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
            fprintf(stderr, "fileext for suite %d not implemented\n", suite);
    }

    chunk_len = read(f, chunk_buf, chunk_size);
    while(!is_last && chunk_len > 0) {

        if(chunk_len < chunk_size)
            is_last = 1;
        else
            next_chunk_len = read(f, next_chunk_buf, chunk_size);

        // found last chunk
        if (next_chunk_len <= 0) {
            is_last = 1;
        } 

        strcpy(url, url_orig);
        offs = CCNL_MAX_PACKET_SIZE;
        name = ccnl_URItoPrefix(url, suite, nfnexpr, &chunknum);
        switch (suite) {
        case CCNL_SUITE_CCNTLV: 
            contentlen = ccnl_ccntlv_prependContentWithHdr(name, 
                                                           (unsigned char *)chunk_data, chunk_len, 
                                                           is_last ? &chunknum : NULL, 
                                                           &offs, 
                                                           NULL, // int *contentpos
                                                           out);
            break;
        case CCNL_SUITE_NDNTLV:
            contentlen = ccnl_ndntlv_prependContent(name, 
                                                    (unsigned char *) chunk_data, chunk_len, 
                                                    &offs, NULL,
                                                    is_last ? &chunknum : NULL, 
                                                    out);
            break;
        default:
            fprintf(stderr, "produce for suite %i is not implemented\n", suite);
            goto Error;
            break;
        }

        if(outdirname) {
            sprintf(outfilename, "%s/%s%d.%s\n", outdirname, outfname, chunknum, fileext);

            fprintf(stderr, "writing to %s for chunk %d\n", outfilename, chunknum);

            fout = creat(outfilename, 0666);
            write(fout, out + offs, contentlen);
            close(fout);
        } else {
            fwrite(out + offs, sizeof(unsigned char),contentlen, stdout);
        }

        chunknum++;

        temp_chunk_buf = chunk_buf;
        chunk_buf = next_chunk_buf;
        next_chunk_buf = temp_chunk_buf;
        chunk_len = next_chunk_len;
    } 
    close(f);

    fprintf(stderr, "read %d chunks from stdin\n", chunknum);

    return 0;

Error:
    return -1;
}

// eof
