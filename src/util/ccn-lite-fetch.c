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
#define USE_SUITE_CISTLV
#define USE_SUITE_NDNTLV
#define USE_LOGGING

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
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB) {
        DEBUGMSG(ERROR, "CCNB not implemented\n");
        exit(-1);
    }
#endif

    ccnl_mkInterestFunc mkInterest = ccnl_suite2mkInterestFunc(suite);
    if (!mkInterest) {
        DEBUGMSG(ERROR, "unknown suite %d/not implemented\n", suite);
        exit(-1);
    }

    int nonce = random();
    *len = mkInterest(prefix, &nonce, out, out_len);
/*
        {
            int fd = open("outgoing.bin", O_WRONLY|O_CREAT|O_TRUNC);
            write(fd, out, *len);
            close(fd);
        }
*/
    if (sendto(sock, out, *len, 0, &sa, sizeof(sa)) < 0) {
        perror("sendto");
        myexit(1);
    }
    if (block_on_read(sock, wait) <= 0) {
        DEBUGMSG(WARNING, "timeout after block_on_read\n");
        return -1;
    }
    *len = recv(sock, out, out_len, 0);
/*
        {
            int fd = open("incoming.bin", O_WRONLY|O_CREAT|O_TRUNC);
            write(fd, out, *len);
            close(fd);
        }
*/
    return 0;
}

int
ccnl_extractDataAndChunkInfo(unsigned char **data, int *datalen,
                             int suite, struct ccnl_prefix_s **prefix,
                             unsigned int *lastchunknum,
                             unsigned char **content, int *contentlen)
{
    struct ccnl_pkt_s *pkt = NULL;

    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: {
        int hdrlen;
        unsigned char *start = *data;

        if (ccntlv_isData(*data, *datalen) < 0) {
            DEBUGMSG(WARNING, "Received non-content-object\n");
            return -1;
        }
        hdrlen = ccnl_ccntlv_getHdrLen(*data, *datalen);
        if (hdrlen < 0)
            return -1;

        *data += hdrlen;
        *datalen -= hdrlen;

        pkt = ccnl_ccntlv_bytes2pkt(start, data, datalen);
        break;
    }
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV: {
        int hdrlen;
        unsigned char *start = *data;

        if (cistlv_isData(*data, *datalen) < 0) {
            DEBUGMSG(WARNING, "Received non-content-object\n");
            return -1;
        }
        hdrlen = ccnl_cistlv_getHdrLen(*data, *datalen);
        if (hdrlen < 0)
            return -1;

        *data += hdrlen;
        *datalen -= hdrlen;

        pkt = ccnl_cistlv_bytes2pkt(start, data, datalen);
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV: {
        int typ;
        int len;
        unsigned char *start = *data;

        if (ccnl_ndntlv_dehead(data, datalen, &typ, &len)) {
            DEBUGMSG(WARNING, "could not dehead\n");
            return -1;
        }
        if (typ != NDN_TLV_Data) {
            DEBUGMSG(WARNING, "received non-content-object packet with type %d\n", typ);
            return -1;
        }

        pkt = ccnl_ndntlv_bytes2pkt(typ, start, data, datalen);
        break;
    }
#endif

    default:
        DEBUGMSG(WARNING, "extractDataAndChunkInfo: suite %d not implemented\n", suite);
        return -1;
   }
    if (!pkt) {
        DEBUGMSG(WARNING, "extract(%s): parsing error or no prefix\n",
                 ccnl_suite2str(suite));
        return -1;
    }
    *prefix = ccnl_prefix_dup(pkt->pfx);
    *lastchunknum = pkt->val.final_block_id;
    *content = pkt->content;
    *contentlen = pkt->contlen;
    free_packet(pkt);

    return 0;
}

int
ccnl_prefix_removeChunkNumComponent(int suite,
                           struct ccnl_prefix_s *prefix) {
    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        // TODO: asumes that chunk is at the end!
        if(prefix->comp[prefix->compcnt-1][1] == CCNX_TLV_N_Chunk) {
            prefix->compcnt--;
        } else {
            DEBUGMSG(WARNING, "Tried to remove chunknum from CCNTLV prefix, but either prefix does not have a chunknum "
                              "or the last component is not the chunknum.");
            return -1;
        }
        break;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        // TODO: asumes that chunk is at the end!
        if(prefix->comp[prefix->compcnt-1][1] == CISCO_TLV_NameSegment) {
            prefix->compcnt--;
        } else {
            DEBUGMSG(WARNING, "Tried to remove chunknum from CISTLV prefix, but either prefix does not have a chunknum "
                              "or the last component is not the chunknum.");
            return -1;
        }
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        if(prefix->comp[prefix->compcnt-1][0] == NDN_Marker_SegmentNumber) {
            prefix->compcnt--;
        }
        break;
#endif
    default:
        DEBUGMSG(ERROR, "removeChunkNum: suite %d not implemented\n", suite);
        return -1;
    }

    return 0;
}


// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[64*1024];
    int len, opt, port, sock = 0, suite = CCNL_SUITE_DEFAULT;
    char *addr = NULL, *udp = NULL, *ux = NULL;
    struct sockaddr sa;
    float wait = 3.0;

    while ((opt = getopt(argc, argv, "hs:u:v:w:x:")) != -1) {
        switch (opt) {
        case 's':
            suite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(suite)) {
                DEBUGMSG(ERROR, "Unsupported suite %s\n", optarg);
                goto usage;
            }
            break;
        case 'u':
            udp = optarg;
            break;
        case 'w':
            wait = atof(optarg);
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
usage:
            fprintf(stderr, "usage: %s [options] URI [NFNexpr]\n"
            "  -s SUITE         (ccnb, ccnx2015, cisco2015, iot2014, ndn2013)\n"
            "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/6363)\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
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

    if (!argv[optind])
        goto usage;

    srandom(time(NULL));

    if (ccnl_parseUdp(udp, suite, &addr, &port) != 0) {
        exit(-1);
    }

    DEBUGMSG(TRACE, "using suite %d:%s\n", suite, ccnl_suite2str(suite));
    DEBUGMSG(TRACE, "using udp address %s/%d\n", addr, port);

    if (ux) { // use UNIX socket
        struct sockaddr_un *su = (struct sockaddr_un*) &sa;
        su->sun_family = AF_UNIX;
        strcpy(su->sun_path, ux);
        sock = ux_open();
    } else { // UDP
        struct sockaddr_in *si = (struct sockaddr_in*) &sa;
        si->sin_family = PF_INET;
        si->sin_addr.s_addr = inet_addr(addr);
        si->sin_port = htons(port);
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

    // For CCNTLV always start with the first chunk because of exact content match
    // This means it can only fetch chunked data and not single content-object data
    if (suite == CCNL_SUITE_CCNTLV || suite == CCNL_SUITE_CISTLV) {
        curchunknum = ccnl_malloc(sizeof(unsigned int));
        *curchunknum = 0;
    }

    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(url, suite, nfnexpr, curchunknum);


    const int maxretry = 3;
    int retry = 0;

    while (retry < maxretry) {

        if (curchunknum) {
            if (!prefix->chunknum) {
                prefix->chunknum = ccnl_malloc(sizeof(unsigned int));
            }
            *(prefix->chunknum) = *curchunknum;
            DEBUGMSG(INFO, "fetching chunk %d for prefix '%s'\n", *curchunknum, ccnl_prefix_to_path(prefix));
        } else {
            DEBUGMSG(DEBUG, "fetching first chunk...\n");
            DEBUGMSG(INFO, "fetching first chunk for prefix '%s'\n", ccnl_prefix_to_path(prefix));
        }

        // Fetch chunk
        if (ccnl_fetchContentForChunkName(prefix,
                                          nfnexpr,
                                          curchunknum,
                                          suite,
                                          out, sizeof(out),
                                          &len,
                                          wait, sock, sa) < 0) {
            retry++;
            DEBUGMSG(WARNING, "timeout\n");//, retry number %d of %d\n", retry, maxretry);
        } else {

            unsigned int lastchunknum;
            unsigned char *t = &out[0];
            struct ccnl_prefix_s *nextprefix = 0;

            // Parse response
            if (ccnl_extractDataAndChunkInfo(&t, &len, suite,
                                             &nextprefix,
                                             &lastchunknum,
                                             &content, &contlen) < 0) {
                retry++;
               DEBUGMSG(WARNING, "Could not extract response or it was an interest\n");
            } else {

                prefix = nextprefix;

                // Check if the fetched content is a chunk
                if (!(prefix->chunknum)) {
                    // Response is not chunked, print content and exit
                    write(1, content, contlen);
                    goto Done;
                } else {
                    int chunknum = *(prefix->chunknum);

                    // allocate curchunknum because it is the first fetched chunk
                    if(!curchunknum) {
                        curchunknum = ccnl_malloc(sizeof(unsigned int));
                        *curchunknum = 0;
                    }
                    // Remove chunk component from name
                    if (ccnl_prefix_removeChunkNumComponent(suite, prefix) < 0) {
                        retry++;
                        DEBUGMSG(WARNING, "Could not remove chunknum\n");
                    }

                    // Check if the chunk is the first chunk or the next valid chunk
                    // otherwise discard content and try again (except if it is the first fetched chunk)
                    if (chunknum == 0 || (curchunknum && *curchunknum == chunknum)) {
                        DEBUGMSG(DEBUG, "Found chunk %d with contlen=%d, lastchunk=%d\n", *curchunknum, contlen, lastchunknum);

                        write(1, content, contlen);

                        if (lastchunknum != -1 && lastchunknum == chunknum) {
                            goto Done;
                        } else {
                            *curchunknum += 1;
                            retry = 0;
                        }
                    } else {
                        // retry if the fetched chunk
                        retry++;
                        DEBUGMSG(WARNING, "Could not find chunk %d, extracted chunknum is %d (lastchunk=%d)\n", *curchunknum, chunknum, lastchunknum);
                    }
                }
            }
        }

        if(retry > 0) {
            DEBUGMSG(INFO, "Retry %d of %d\n", retry, maxretry);
        }
    }

    close(sock);
    return 1;

Done:
    DEBUGMSG(DEBUG, "Sucessfully fetched content\n");
    close(sock);
    return 0;
}

// eof
