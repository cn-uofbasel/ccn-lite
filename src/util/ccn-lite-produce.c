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
#define USE_SUITE_CISTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV

#define USE_SIGNATURES

#define CCNL_MAX_CHUNK_SIZE 4048

#include "ccnl-common.c"
#include "ccnl-crypto.c"

int
main(int argc, char *argv[])
{
    // char *private_key_path = 0;
    //    char *witness = 0;
    unsigned char out[65*1024];
    char *publisher = 0;
    char *infname = 0, *outdirname = 0, *outfname;
    int f, fout, contentlen = 0, opt, plen;
    //    int suite = CCNL_SUITE_DEFAULT;
    int suite = CCNL_SUITE_CCNTLV;
    int chunk_size = CCNL_MAX_CHUNK_SIZE;
    struct ccnl_prefix_s *name;

    while ((opt = getopt(argc, argv, "hc:f:i:o:p:k:w:s:v:")) != -1) {
        switch (opt) {
        case 'c':
            chunk_size = atoi(optarg);
            if (chunk_size > CCNL_MAX_CHUNK_SIZE) {
                DEBUGMSG(WARNING, "max chunk size is %d (%d is to large), using max chunk size\n", CCNL_MAX_CHUNK_SIZE, chunk_size);
                chunk_size = CCNL_MAX_CHUNK_SIZE;
            }
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
            DEBUGMSG(ERROR,
             "publisher key digest has wrong length (%d instead of 32)\n",
             plen);
            exit(-1);
            }
            break;
        case 's':
            suite = ccnl_str2suite(optarg);
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
        fprintf(stderr,
        "Creates a chunked content object stream for the input data and writes them to stdout.\n"
        "usage: %s [options] URL\n"
        "  -c SIZE          size for each chunk (max %d)\n"
        "  -f FNAME         filename of the chunks when using -o\n"
        "  -i FNAME         input file (instead of stdin)\n"
        "  -o DIR           output dir (instead of stdout), filename default is cN, otherwise specify -f\n"
        "  -p DIGEST        publisher fingerprint\n"
        "  -s SUITE         (ccnb, ccnx2015, cisco2015, iot2014, ndn2013)\n"
#ifdef USE_LOGGING
        "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
        ,
        argv[0],
        CCNL_MAX_CHUNK_SIZE);
        exit(1);
        }
    }

    if (!ccnl_isSuite(suite))
        goto Usage;

    // mandatory url
    if (!argv[optind])
        goto Usage;

    char *url_orig = argv[optind];
    char url[strlen(url_orig)];
    optind++;

    // optional nfn
    char *nfnexpr = argv[optind];

    int status;
    struct stat st_buf;
    if(outdirname) {
        // Check if outdirname is a directory and open it as a file
        status = stat(outdirname, &st_buf);
        if (status != 0) {
            // DEBUGMSG (ERROR, "Error (%d) when opening file %s\n", errno, outdirname);
            DEBUGMSG(ERROR, "Error (%d) when opening output dir %s (probaby does not exist)\n", errno, outdirname);
            goto Usage;
        }
        if (S_ISREG (st_buf.st_mode)) {
            // DEBUGMSG (ERROR, "Error: %s is a file and not a directory.\n", argv[optind]);
            DEBUGMSG(ERROR, "Error: output dir %s is a file and not a directory.\n", outdirname);
            goto Usage;
        }
    }
    if(infname) {
        // Check if outdirname is a directory and open it as a file
        status = stat(infname, &st_buf);
        if (status != 0) {
            // DEBUGMSG (ERROR, "Error (%d) when opening file %s\n", errno, outdirname);
            DEBUGMSG(ERROR, "Error (%d) when opening input file %s (probaby does not exist)\n", errno, infname);
            goto Usage;
        }
        if (S_ISDIR (st_buf.st_mode)) {
            // DEBUGMSG (ERROR, "Error: %s is a file and not a directory.\n", argv[optind]);
            DEBUGMSG(ERROR, "Error: input file %s is a directory and not a file.\n", infname);
            goto Usage;
        }
        f = open(infname, O_RDONLY);
        if (f < 0) {
            perror("file open:");
        }
    } else {
      f = 0;
    }

    char default_file_name[2] = "c";
    if (!outfname) {
        outfname = default_file_name;
    } else if(!outdirname) {
        DEBUGMSG(WARNING, "filename -f without -o output dir does nothing\n");
    }

    char *chunk_buf;
    chunk_buf = ccnl_malloc(chunk_size * sizeof(unsigned char));
    int chunk_len, is_last = 0, offs = -1;
    unsigned int chunknum = 0;

    char outpathname[255];
    char fileext[10];
    switch (suite) {
        case CCNL_SUITE_CCNB:
            strcpy(fileext, "ccnb");
            break;
        case CCNL_SUITE_CCNTLV:
            strcpy(fileext, "ccntlv");
            break;
        case CCNL_SUITE_CISTLV:
            strcpy(fileext, "cistlv");
            break;
        case CCNL_SUITE_IOTTLV:
            strcpy(fileext, "iottlv");
            break;
        case CCNL_SUITE_NDNTLV:
            strcpy(fileext, "ndntlv");
            break;
        default:
            DEBUGMSG(ERROR, "fileext for suite %d not implemented\n", suite);
    }

    
    FILE *fp = fopen(infname, "r");
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    rewind(fp);
    fclose(fp);

    unsigned int lastchunknum = sz/chunk_size;
    if (sz % chunk_size == 0) --lastchunknum;

    chunk_len = 1;
    chunk_len = read(f, chunk_buf, chunk_size);
    while (!is_last && chunk_len > 0) {

        if (chunk_len < chunk_size) {
            is_last = 1;
        }

        strcpy(url, url_orig);
        offs = CCNL_MAX_PACKET_SIZE;
        name = ccnl_URItoPrefix(url, suite, nfnexpr, &chunknum);

        switch (suite) {
        case CCNL_SUITE_CCNTLV:
            contentlen = ccnl_ccntlv_prependContentWithHdr(name,
                            (unsigned char *)chunk_buf, chunk_len,
                            &lastchunknum, //is_last ? &chunknum : NULL, 
                            NULL, // int *contentpos
                            &offs, out);
            break;
        case CCNL_SUITE_CISTLV:
            contentlen = ccnl_cistlv_prependContentWithHdr(name,
                                                           (unsigned char *)chunk_buf, chunk_len,
                                                           is_last ? &chunknum : NULL,
                                                           &offs,
                                                           NULL, // int *contentpos
                                                           out);
            break;
        case CCNL_SUITE_IOTTLV:
            ccnl_iottlv_prependReply(name, (unsigned char *) chunk_buf,
                                     chunk_len, &offs, NULL,
                                     is_last ? &chunknum : NULL, out);
            ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offs, out);
            contentlen = CCNL_MAX_PACKET_SIZE - offs;
            break;
        case CCNL_SUITE_NDNTLV:
            contentlen = ccnl_ndntlv_prependContent(name,
                                 (unsigned char *) chunk_buf, chunk_len,
                                 NULL,
                                 &lastchunknum,// is_last ? &chunknum : NULL,
                                 &offs, out);
            break;
        default:
            DEBUGMSG(ERROR, "produce for suite %i is not implemented\n", suite);
            goto Error;
            break;
        }

        if (outdirname) {
            sprintf(outpathname, "%s/%s%d.%s", outdirname, outfname, chunknum, fileext);
//            DEBUGMSG(INFO, "%s/%s%d.%s\n", outdirname, outfname, chunknum, fileext);

            DEBUGMSG(INFO, "writing chunk %d to file %s\n", chunknum, outpathname);

            fout = creat(outpathname, 0666);
            write(fout, out + offs, contentlen);
            close(fout);
        } else {
            DEBUGMSG(INFO, "writing chunk %d\n", chunknum);
            fwrite(out + offs, sizeof(unsigned char),contentlen, stdout);
        }

        chunknum++;
        if (!is_last) {
            chunk_len = read(f, chunk_buf, chunk_size);
        }
    }

    close(f);
    ccnl_free(chunk_buf);
    return 0;

Error:
    close(f);
    ccnl_free(chunk_buf);
    return -1;
}

// eof
