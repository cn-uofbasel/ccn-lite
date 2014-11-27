/*
 * @f util/ccn-lite-fetch.c
 * @b request content: send an interest, wait for reply, output to stdout
 *
 * Copyright (C) 2013-14, Basil Kohler, University of Basel
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
 * 2014-10-13  created
 */


#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"
#include "ccnl-socket.c"

// ----------------------------------------------------------------------

int
ccnl_fetchContentForChunkName(struct ccnl_prefix_s *prefix,
                              char* nfnexpr,
                              unsigned int *chunknum,
                              int suite, 
                              unsigned char *out, int out_len, 
                              int *len, 
                              float wait, int sock, struct sockaddr sa) {

    int (*mkInterest)(struct ccnl_prefix_s*, int*, unsigned char*, int);
    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        DEBUGMSG(99, "CCNB not implemented\n");
        exit(-1);
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        mkInterest = ccntlv_mkInterest;
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        mkInterest = ndntlv_mkInterest;
        break;
#endif
    default:
        DEBUGMSG(99, "unknown suite\n");
        exit(-1);
    }

    int nonce = random();
    *len = mkInterest(prefix, &nonce, out, out_len);

    if (sendto(sock, out, *len, 0, &sa, sizeof(sa)) < 0) {
        perror("sendto");
        myexit(1);
    }
    if (block_on_read(sock, wait) <= 0) {
        DEBUGMSG(99, "timeout after block_on_read\n");
        return -1;
    }
    *len = recv(sock, out, out_len, 0);

    return 0;
}

int ccnl_ndntlv_extractDataAndChunkinfo(unsigned char **data, int *datalen, 
                                        unsigned int *chunknum, unsigned int *lastchunknum,
                                        unsigned char **content, int *contentlen) {
    int typ, len;
    unsigned char *cp = *data;
    int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
    struct ccnl_prefix_s *prefix;

    if (ccnl_ndntlv_dehead(data, datalen, &typ, &len)) {
        DEBUGMSG(99, "could not dehead\n");
        return -1;
    }
    buf = ccnl_ndntlv_extract(*data - cp,
                  data, datalen,
                  &scope, &mbf, &minsfx, &maxsfx, lastchunknum,
                  &prefix, NULL,
                  &nonce, // nonce
                  &ppkl, //ppkl
                  content, contentlen);
    if (!buf) {
        DEBUGMSG(99, "ndntlv_extract: parsing error or no prefix\n"); 
        return -1;
    } 
    if (typ == NDN_TLV_Interest) {
        DEBUGMSG(99, "ignoring parsed interest with name %s\n", ccnl_prefix_to_path(prefix));
    } else { // data packet with content -------------------------------------
        *chunknum = *prefix->chunknum;
    }
    return 0;
}
int ccnl_ccntlv_extractDataAndChunkinfo(unsigned char **data, int *datalen, 
                                      unsigned int *chunknum, unsigned int *lastchunknum,
                                      unsigned char **content, int *contentlen) {
    struct ccnl_prefix_s *prefix;
    *datalen -= 8;
    *data += 8;
    int hdrlen = 8;

    if (ccnl_ccntlv_extract(hdrlen,
                           data, datalen,
                           &prefix,
                           0, 0, // keyid/keyidlen
                           lastchunknum,
                           content, contentlen) == NULL) {
        fprintf(stderr, "Error in ccntlv_extract\n");
        exit(-1);
    } 

    if (!prefix->chunknum) {
        fprintf(stderr, "Extracted correct data object but name did not contain a chunk number\n");
        exit(-1);
    } else {
        *chunknum = *prefix->chunknum;
    }

    return 0;
}
int ccnl_extractDataAndChunkInfo(unsigned char **data, int *datalen, 
                                 int suite, 
                                 unsigned int *chunknum, unsigned int *lastchunknum,
                                 unsigned char **content, int *contentlen) {
    int result = -1;
    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        result = ccnl_ccntlv_extractDataAndChunkinfo(data, datalen, chunknum, lastchunknum, content, contentlen);
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        result = ccnl_ndntlv_extractDataAndChunkinfo(data, datalen, chunknum, lastchunknum, content, contentlen);
        break;
#endif
    default:
    DEBUGMSG(99, "suite not implemented or used %d", suite);
    exit(-1);
   }

   return result;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[64*1024];
    int len, opt, sock = 0, suite = CCNL_SUITE_DEFAULT;
    char *udp = NULL, *ux = NULL;
    struct sockaddr sa;
    float wait = 3.0;

    debug_level = 99;

    while ((opt = getopt(argc, argv, "hs:u:w:x:")) != -1) {
        switch (opt) {
        case 's':
            suite = ccnl_str2suite(optarg);
            if (suite < 0 || suite >= CCNL_SUITE_LAST)
                goto usage;
            break;
        case 'u':
            udp = optarg;
            break;
        case 'w':
            wait = atof(optarg);
            break;
        case 'x':
            ux = optarg;
            break;
        case 'h':
        default:
usage:
            fprintf(stderr, "usage: %s [options] URI [NFNexpr]\n"
            "  -s SUITE         (ccnb, ccnx2014, ndn2013)\n"
            "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/6363)\n"
            "  -w timeout       in sec (float)\n"
            "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
            "Examples:\n"
            "%% peek /ndn/edu/wustl/ping             (classic lookup)\n"
            "%% peek /th/ere  \"lambda expr\"          (lambda expr, in-net)\n"
            "%% peek \"\" \"add 1 1\"                    (lambda expr, local)\n"
            "%% peek /rpc/site \"call 1 /test/data\"   (lambda RPC, directed)\n",
            argv[0]);
            exit(1);
        }
    }
    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        if (!udp)
            udp = "127.0.0.1/9695";
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        if (!udp)
            udp = "127.0.0.1/9695";
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        if (!udp)
            udp = "127.0.0.1/6363";
        break;
#endif
        default:
            udp = "127.0.0.1/6363";
        }

    if (!argv[optind]) 
        goto usage;

    srandom(time(NULL));

    if (ux) { // use UNIX socket
        struct sockaddr_un *su = (struct sockaddr_un*) &sa;
        su->sun_family = AF_UNIX;
        strcpy(su->sun_path, ux);
        sock = ux_open();
    } else { // UDP
        struct sockaddr_in *si = (struct sockaddr_in*) &sa;
        udp = strdup(udp);
        si->sin_family = PF_INET;
        si->sin_addr.s_addr = inet_addr(strtok(udp, "/"));
        si->sin_port = htons(atoi(strtok(NULL, "/")));
        sock = udp_open();
    }

    char *url = argv[optind];

    char *nfnexpr = 0;
    
    if (argv[optind+1]) {
        nfnexpr = argv[optind+1];
    }

    unsigned char *content = 0;
    int contlen;

    unsigned int *curchunknum = NULL;

    if(suite == CCNL_SUITE_CCNTLV) {
        curchunknum = malloc(sizeof(int));
        *curchunknum = 0;
    }

    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(url, suite, nfnexpr, curchunknum);

    for (;;) {

        
        if (curchunknum) {
            if (!prefix->chunknum) {
                prefix->chunknum = ccnl_malloc(sizeof(unsigned int));
            }
            *prefix->chunknum = *curchunknum; 
            DEBUGMSG(DEBUG,  "fetching chunk %d...\n", *curchunknum);
        } else {
            DEBUGMSG(DEBUG, "fetching first chunk...\n");
        }

        if (ccnl_fetchContentForChunkName(prefix, 
                                          nfnexpr,
                                          curchunknum,
                                          suite, 
                                          out, sizeof(out), 
                                          &len, 
                                          wait, sock, sa) < 0) {
            fprintf(stderr, "timeout, retry not implemented, exit\n");
            exit(1);
        }

        unsigned int chunknum = UINT_MAX, lastchunknum = UINT_MAX;
        unsigned char *t = &out[0];
        if (ccnl_extractDataAndChunkInfo(&t, &len, suite, 
                                         &chunknum, &lastchunknum, 
                                         &content, &contlen) < 0) {
            fprintf(stderr, "Could not extract data and chunkinfo\n");
            goto Done;
        } 

        if (chunknum == UINT_MAX) {
            // Response is not chunked, print content and exit
            write(1, content, contlen);
        } else {
            // Response is chunked with chunknum as the answer

            // Check if it is the next chunk, otherwise discard content and fetch the next chunk...
            if (chunknum == 0 || (curchunknum && *curchunknum == chunknum)) {
                DEBUGMSG(DEBUG, "Found chunk %d with contlen=%d\n", *curchunknum, contlen);
                if (!curchunknum) {
                    curchunknum = ccnl_malloc(sizeof(unsigned int));
                    *curchunknum = 0;
                }

                write(1, content, contlen);

                if (lastchunknum != UINT_MAX && lastchunknum == chunknum) {
                    goto Done;
                } else {
                    *curchunknum += 1;
                }
            } else if(!curchunknum) {
                DEBUGMSG(DEBUG, "First chunk is %d and not chunk 0, drop this chunk and start with chunk 0\n", chunknum);
                curchunknum = ccnl_malloc(sizeof(unsigned int));
                *curchunknum = 0;
            } else {
                // error, a specific chunk was fetched, but it was not the next chunk
                DEBUGMSG(ERROR, "Could not find chunk %d, extracted chunknum is %d\n", *curchunknum, chunknum);
                goto Error;
            }
        }
    }

Error:
    close(sock);
    return 1;

Done:
    close(sock);
    return 0; 
}

// eof
