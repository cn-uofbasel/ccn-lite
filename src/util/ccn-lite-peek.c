/*
 * @f util/ccn-lite-peek.c
 * @b request content: send an interest, wait for reply, output to stdout
 *
 * Copyright (C) 2013-14, Christian Tschudin, University of Basel
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
 * 2013-04-06  created
 * 2014-06-18  added NDNTLV support
 */

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"
#include "ccnl-socket.c"


// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[64*1024];
    int cnt, len, opt, sock = 0, suite = CCNL_SUITE_NDNTLV;
    char *udp = NULL, *ux = NULL;
    struct sockaddr sa;
    struct ccnl_prefix_s *prefix;
    float wait = 3.0;
    int (*mkInterest)(struct ccnl_prefix_s*, int*, unsigned char*, int);
    int (*isContent)(unsigned char*, int);
    unsigned int chunknum = UINT_MAX;

    while ((opt = getopt(argc, argv, "hn:s:u:w:x:")) != -1) {
        switch (opt) {
        case 'n':
            chunknum = atoi(optarg);
            break;
        case 's':
          /*
            opt = atoi(optarg);
            if (opt < CCNL_SUITE_CCNB || opt >= CCNL_SUITE_LAST)
                goto usage;
            suite = opt;
          */
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
            "  -n CHUNKNUM      positive integer for chunk interest\n"
            "  -s SUITE         SUITE= ccnb, ccnx2014, ndn2013 (default)\n"
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
        if(!udp)
            udp = "127.0.0.1/9695";
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        if(!udp)
            udp = "127.0.0.1/9695";
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        if(!udp)
            udp = "127.0.0.1/6363";
        break;
#endif
        default:
            udp = "127.0.0.1/6363";
        }

    if (!argv[optind]) 
        goto usage;

    srandom(time(NULL));

    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        mkInterest = ccnl_ccnb_fillInterest;
        isContent = ccnb_isContent;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        mkInterest = ccntlv_mkInterest;
        isContent = ccntlv_isObject;
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        mkInterest = ndntlv_mkInterest;
        isContent = ndntlv_isData;
        break;
#endif
    default:
        printf("unknown suite\n");
        exit(-1);
    }

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

    prefix = ccnl_URItoPrefix(argv[optind], suite, argv[optind+1], chunknum == UINT_MAX ? NULL : &chunknum);
/*
    fprintf(stderr, "prefix <%s><%s> became is %s\n",
            argv[optind], argv[optind+1], ccnl_prefix_to_path(prefix));
*/
    for (cnt = 0; cnt < 3; cnt++) {
        int nonce = random();

        len = mkInterest(prefix, 
                         &nonce, 
                         out, sizeof(out));

/*
        {
            int fd = open("pkt.bin", O_CREAT | O_WRONLY);
            write(fd, out, len);
            close(fd);
        }
*/

        if (sendto(sock, out, len, 0, &sa, sizeof(sa)) < 0) {
            perror("sendto");
            myexit(1);
        }

        for (;;) { // wait for a content pkt (ignore interests)
            int rc;

            if (block_on_read(sock, wait) <= 0) // timeout
                break;
            len = recv(sock, out, sizeof(out), 0);

/*
            fprintf(stderr, "received %d bytes\n", len);
            if (len > 0)
                fprintf(stderr, "  suite=%d\n", ccnl_pkt2suite(out, len));
*/

            rc = isContent(out, len);
            if (rc < 0) {
                fprintf(stderr, "error when checking type of packet\n");
                goto done;
            }
            if (rc == 0) { // it's an interest, ignore it
                fprintf(stderr, "skipping non-data packet\n");
                continue;
            }
            write(1, out, len);
            myexit(0);
        }
        if (cnt < 2)
            fprintf(stderr, "re-sending interest\n");
    }
    fprintf(stderr, "timeout\n");

done:
    close(sock);
    myexit(-1);
    return 0; // avoid a compiler warning
}

// eof
