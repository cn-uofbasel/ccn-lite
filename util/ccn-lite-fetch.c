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

#include "ccnl-common.c"

// ----------------------------------------------------------------------

char *unix_path;

void
myexit(int rc)
{
    if (unix_path)
        unlink(unix_path);
    exit(rc);
}

// ----------------------------------------------------------------------

int
ccntlv_mkInterest(struct ccnl_prefix_s *name, int *dummy,
                  unsigned char *out, int outlen)
{
    int len, offset;

    offset = outlen;
    len = ccnl_ccntlv_fillInterestWithHdr(name, -1, &offset, out);
    if (len > 0)
        memmove(out, out + offset, len);

    return len;
}

int
ndntlv_mkInterest(struct ccnl_prefix_s *name, int *nonce,
                  unsigned char *out, int outlen)
{
    int len, offset;

    offset = outlen;
    len = ccnl_ndntlv_fillInterest(name, -1, nonce, &offset, out);
    if (len > 0)
        memmove(out, out + offset, len);

    return len;
}



// ----------------------------------------------------------------------

int
udp_open()
{
    int s;
    struct sockaddr_in si;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("udp socket");
        exit(1);
    }
    si.sin_addr.s_addr = INADDR_ANY;
    si.sin_port = htons(0);
    si.sin_family = PF_INET;
    if (bind(s, (struct sockaddr *)&si, sizeof(si)) < 0) {
        perror("udp sock bind");
        exit(1);
    }

    return s;
}

int
ux_open()
{
static char mysockname[200];
 int sock, bufsize;
    struct sockaddr_un name;

    sprintf(mysockname, "/tmp/.ccn-lite-peek-%d.sock", getpid());
    unlink(mysockname);

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        exit(1);
    }
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, mysockname);
    if (bind(sock, (struct sockaddr *) &name,
             sizeof(struct sockaddr_un))) {
        perror("binding path name to datagram socket");
        exit(1);
    }

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    unix_path = mysockname;
    return sock;
}

// ----------------------------------------------------------------------

int
block_on_read(int sock, float wait)
{
    fd_set readfs;
    struct timeval timeout;
    int rc;

    FD_ZERO(&readfs);
    FD_SET(sock, &readfs);
    timeout.tv_sec = wait;
    timeout.tv_usec = 1000000.0 * (wait - timeout.tv_sec);
    rc = select(sock+1, &readfs, NULL, NULL, &timeout);
    if (rc < 0)
        perror("select()");
    return rc;
}

#ifdef USE_SUITE_CCNB
int ccnb_isContent(unsigned char *buf, int len)
{
    int num, typ;

    if (len < 0 || ccnl_ccnb_dehead(&buf, &len, &num, &typ))
        return -1;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ)
        return 0;
    return 1;
}
#endif

#ifdef USE_SUITE_CCNTLV
int ccntlv_isObject(unsigned char *buf, int len)
{
    unsigned int typ, vallen;
    struct ccnx_tlvhdr_ccnx201311_s *hp = (struct ccnx_tlvhdr_ccnx201311_s*)buf;

    if (len <= sizeof(struct ccnx_tlvhdr_ccnx201311_s))
        return -1;
    if (hp->version != CCNX_TLV_V0)
        return -1;
    if ((sizeof(struct ccnx_tlvhdr_ccnx201311_s)+ntohs(hp->hdrlen)+ntohs(hp->msglen) > len))
        return -1;
    buf += sizeof(struct ccnx_tlvhdr_ccnx201311_s) + ntohs(hp->hdrlen);
    len -= sizeof(struct ccnx_tlvhdr_ccnx201311_s) + ntohs(hp->hdrlen);
    if (ccnl_ccntlv_dehead(&buf, &len, &typ, &vallen))
        return -1;
    if (hp->msgtype != CCNX_TLV_TL_Object || typ != CCNX_TLV_TL_Object)
        return 0;
    return 1;
}
#endif

#ifdef USE_SUITE_NDNTLV
int ndntlv_isData(unsigned char *buf, int len)
{
    int typ, vallen;

    if (len < 0 || ccnl_ndntlv_dehead(&buf, &len, &typ, &vallen))
        return -1;
    if (typ != NDN_TLV_Data)
        return 0;
    return 1;
}
#endif

int
findChunkInfo(unsigned char *packetData, int datalen) {
    int len = 0, typ;
    int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
    struct ccnl_prefix_s *prefix;
    unsigned char *finalBlockId = 0;
    int finalBlockId_len = 0;
    unsigned char *content = 0, *cp = packetData;

    if (ccnl_ndntlv_dehead(&packetData, &datalen, &typ, &len))
    return -1;
    buf = ccnl_ndntlv_extract(packetData - cp,
                  &packetData, &datalen,
                  &scope, &mbf, &minsfx, &maxsfx, finalBlockId, &finalBlockId_len,
                  &prefix, &nonce, &ppkl, &content, &contlen);
    if (!buf) {
        printf("parsing error or no prefix\n"); 
        return -1;
    } 
    if (typ == NDN_TLV_Interest) {
        printf("parsed interest %s\n", ccnl_prefix_to_path(prefix));
    } else { // data packet with content -------------------------------------
        printf("parsed content [%s -> '%s'] with finalBlockId: %s\n", ccnl_prefix_to_path(prefix), packetData, "not impl");
    }

    return 0;
}

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

    printf("started peek\n");
    while ((opt = getopt(argc, argv, "hs:u:w:x:")) != -1) {
        switch (opt) {
        case 's':
            opt = atoi(optarg);
            if (opt < CCNL_SUITE_CCNB || opt >= CCNL_SUITE_LAST)
                goto usage;
            suite = opt;
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
            "  -s SUITE         0=ccnb, 1=ccntlv, 2=ndntlv (default)\n"
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
    printf("before if(ux) with udp %s\n", udp);

    if (ux) { // use UNIX socket
        struct sockaddr_un *su = (struct sockaddr_un*) &sa;
        su->sun_family = AF_UNIX;
        strcpy(su->sun_path, ux);
        sock = ux_open();
    } else { // UDP
        struct sockaddr_in *si = (struct sockaddr_in*) &sa;
        printf("strdup\n");
        udp = strdup(udp);
        si->sin_family = PF_INET;
        si->sin_addr.s_addr = inet_addr(strtok(udp, "/"));
        si->sin_port = htons(atoi(strtok(NULL, "/")));
        printf("open\n");
        sock = udp_open();
        printf("opened\n");
    }
    printf("after ux\n");

    printf("before making uri: %s\n", argv[optind]);
    prefix = ccnl_URItoPrefix(argv[optind], suite, argv[optind+1]);
    for (cnt = 0; cnt < 3; cnt++) {
        int nonce = random();

        len = mkInterest(prefix, &nonce, out, sizeof(out));

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

            // *** parse content *******************************
            unsigned char **data = 0; 
            int *datalen = 0;

            int len = 0, typ;
            int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3, contlen;
            struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
            struct ccnl_prefix_s *prefix;
            unsigned char *finalBlockId = 0;
            int finalBlockId_len = 0;
            unsigned char *content = 0, *cp = *data;

            if (ccnl_ndntlv_dehead(data, datalen, &typ, &len))
            return -1;
            buf = ccnl_ndntlv_extract(*data - cp,
                          data, datalen,
                          &scope, &mbf, &minsfx, &maxsfx, finalBlockId, &finalBlockId_len,
                          &prefix, &nonce, &ppkl, &content, &contlen);
            if (!buf) {
                printf("parsing error or no prefix\n"); 
                goto Done;
            } 
            if (typ == NDN_TLV_Interest) {
                printf("parsed interest %s\n", ccnl_prefix_to_path(prefix));
            } else { // data packet with content -------------------------------------
                printf("parsed content [%s -> '%p'] with finalBlockId: %s\n", ccnl_prefix_to_path(prefix), data, finalBlockId);
            }

            // === parse content ===============================


            rc = isContent(out, len);
            if (rc < 0)
                goto Done;
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

Done:
    close(sock);
    myexit(-1);
    return 0; // avoid a compiler warning
}

// eof
