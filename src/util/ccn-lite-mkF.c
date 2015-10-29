/*
 * @f util/ccn-lite-mkF.c
 * @b CLI mkFragment: split a large file into a fragment series
 *
 * Copyright (C) 2013-15, Christian Tschudin, University of Basel
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
 * 2013-07-06  created
 */

#define USE_LOGGING
#define USE_DEBUG
#define USE_FRAG
#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_CISTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV

#include "ccnl-common.c"

// ----------------------------------------------------------------------

void
file2frags(int suite, unsigned char *data, int datalen, char *fileprefix,
           int bytelimit, unsigned int *seqnr, unsigned int seqnrwidth,
           bool noclobber)
{
    struct ccnl_buf_s *fragbuf;
    struct ccnl_frag_s fr;
    char fname[512];
    int cnt = 0, f;

    memset(&fr, 0, sizeof(fr));
    //    fr.protocol = CCNL_FRAG_CCNx2013;
    // fr.protocol = CCNL_FRAG_SEQUENCED2015;
    fr.protocol = CCNL_FRAG_BEGINEND2015;
    fr.bigpkt = ccnl_buf_new(data, datalen);
    fr.mtu = bytelimit;
    fr.sendseq = *seqnr;
    fr.sendseqwidth = seqnrwidth;
    fr.flagwidth = 1;
    fr.outsuite = suite;

    //    fragbuf = ccnl_frag_getnext(&fr);
    fragbuf = ccnl_frag_getnext(&fr, NULL, NULL);
    while (fragbuf) {
        sprintf(fname, "%s%03d.frag", fileprefix, cnt);
        if (noclobber && !access(fname, F_OK)) {
            printf("file %s already exists, skipping this name\n", fname);
        } else {
            printf("new fragment, len=%zd / %d --> %s\n",
                   fragbuf->datalen, fr.sendseq, fname);
            f = creat(fname, 0666);
            if (f < 0)
                perror("open");
            else {
                if (write(f, fragbuf->data, fragbuf->datalen) < 0)
                    perror("write");
                close(f);
            }
            ccnl_free(fragbuf);
            fragbuf = ccnl_frag_getnext(&fr, NULL, NULL); //ccnl_frag_getnext(&fr);
        }
        cnt++;
    }
    *seqnr = fr.sendseq;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int opt, len, fd;
    unsigned int bytelimit = 1500, seqnr = 0, seqnrlen = 4;
    char *cmdname = argv[0], *cp, *fname, *fileprefix = "frag";
    bool noclobber = false;
    int suite = CCNL_SUITE_DEFAULT;

    while ((opt = getopt(argc, argv, "a:b:f:hns:v:")) != -1) {
        switch (opt) {
        case 'a':
            seqnr = strtol(optarg, &cp, 0);
            if (cp && cp[0]== '/' && isdigit(cp[1]))
                seqnrlen = atoi(cp+1);
            break;
        case 'b':
            bytelimit = atoi((char*) optarg);
            break;
        case 'f':
            fileprefix = optarg;
            break;
        case 'n':
            noclobber = true;
            break;
        case 's':
            suite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(suite)) {
                DEBUGMSG(ERROR, "Unsupported suite %s\n", optarg);
                goto Usage;
            }
            break;
        case 'v':
#ifdef USE_LOGGING
            debug_level = ccnl_debug_str2level(optarg);
#endif
            break;

        case 'h':
        default:
Usage:
            fprintf(stderr, "usage: %s [options] FILE(S)\n"
            "  -a NUM[/SZ] start with seqnr NUM, SZ Bytes (default: 0/4)\n"
            "  -b LIMIT    MTU limit (default is 1500)\n"
            "  -f PREFIX   use PREFIX for fragment file names (default: frag)\n"
            "  -n          no-clobber\n"
            "  -s SUITE    (ccnb, ccnx2015, cisco2015, iot2014, ndn2013)\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
            ,
            cmdname);
            exit(1);
        }
    }

    DEBUGMSG(INFO, "Using suite %d:%s\n", suite, ccnl_suite2str(suite));

    fname = argv[optind] ? argv[optind++] : "-";
    do {
        unsigned char in[64*1024];
        fd = strcmp(fname, "-") ? open(fname, O_RDONLY) : 0;
        if (fd < 0) {
            fprintf(stderr, "error opening file %s\n", fname);
            exit(-1);
        }
        len = read(fd, in, sizeof(in));
        if (len < 0) {
            fprintf(stderr, "error reading file %s\n", fname);
            exit(-1);
        }
        if (len == sizeof(in)) {
            char tmp;
            len = read(fd, &tmp, 1);
            if (len > 0) {
                fprintf(stderr, "error: input file %s larger than %d KB\n",
                        fname, (int)(sizeof(in)/1024));
                exit(-1);
            }
        }
        close(fd);

        file2frags(suite, in, len, fileprefix, bytelimit,
                   &seqnr, seqnrlen, noclobber);
        fname = argv[optind] ? argv[optind++] : NULL;
    } while (fname);

    return 0;
}

// eof
