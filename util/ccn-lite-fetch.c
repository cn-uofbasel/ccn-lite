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
    len = ccnl_ccntlv_fillInterestWithHdr(name, &offset, out);
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
    if (len <= sizeof(struct ccnx_tlvhdr_ccnx201409_s))
        return -1;
    struct ccnx_tlvhdr_ccnx201409_s *hp = (struct ccnx_tlvhdr_ccnx201409_s*)buf;

    if (hp->version != CCNX_TLV_V0)
        return -1;

    unsigned short hdrlen = ntohs(hp->hdrlen);
    unsigned short payloadlen = ntohs(hp->payloadlen);

    if (hdrlen + payloadlen > len)
        return -1;
    buf += hdrlen;
    len -= hp->hdrlen;


    if(hp->packettype == CCNX_PT_ContentObject)
        return 1;
    else
        return 0;
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
fetch_content_object_for_name(char* name, int suite, unsigned char *out, int out_len, int *len, float wait, int sock, struct sockaddr sa) {

    int (*mkInterest)(struct ccnl_prefix_s*, int*, unsigned char*, int);
    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        mkInterest = ccnl_ccnb_fillInterest;
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
        printf("unknown suite\n");
        exit(-1);
    }


    char temp_name[1024];
    strcpy(temp_name, name);
    struct ccnl_prefix_s *prefix;

    prefix = ccnl_URItoPrefix(name, suite, NULL);
    int nonce = random();
    *len = mkInterest(prefix, &nonce, out, out_len);

    if (sendto(sock, out, *len, 0, &sa, sizeof(sa)) < 0) {
        perror("sendto");
        myexit(1);
    }
    if (block_on_read(sock, wait) <= 0) {
        printf("timeout after block_on_read");
        return -1;
    }
    *len = recv(sock, out, out_len, 0);

    return 0;
}

// int
// findChunkInfo(unsigned char *packetData, int datalen) {
//     int len = 0, typ;
//     int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3, contlen;
//     struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
//     struct ccnl_prefix_s *prefix;
//     unsigned char *finalBlockId = 0;
//     int finalBlockId_len = 0;
//     unsigned char *content = 0, *cp = packetData;

//     if (ccnl_ndntlv_dehead(&packetData, &datalen, &typ, &len))
//     return -1;
//     buf = ccnl_ndntlv_extract(packetData - cp,
//                   &packetData, &datalen,
//                   &scope, &mbf, &minsfx, &maxsfx, finalBlockId, &finalBlockId_len,
//                   &prefix, &nonce, &ppkl, &content, &contlen);
//     if (!buf) {
//         printf("parsing error or no prefix\n"); 
//         return -1;
//     } 
//     if (typ == NDN_TLV_Interest) {
//         printf("parsed interest %s\n", ccnl_prefix_to_path(prefix));
//     } else { // data packet with content -------------------------------------
//         printf("parsed content [%s -> '%s'] with finalBlockId: %s\n", ccnl_prefix_to_path(prefix), packetData, "not impl");
//     }

//     return 0;
// }

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[64*1024];
    int len, opt, sock = 0, suite = CCNL_SUITE_NDNTLV;
    char *udp = NULL, *ux = NULL;
    struct sockaddr sa;
    float wait = 3.0;
    // int (*isContent)(unsigned char*, int);

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

    char urlOrig[1024];
    strcpy(urlOrig, argv[optind]);

    char urlTemp[1024];

    unsigned char *content = 0;
    int contlen;


    unsigned char **dataOfChunks = 0;
    int *datalenOfChunks = 0;
    int nChunks = -1;

    int do_fetch_next = 1;
    while(do_fetch_next) {

        strcpy(urlTemp, urlOrig);
        if(dataOfChunks) {
            for(int x = 0; x < nChunks; x++) {
                if(!dataOfChunks[x]) {
                    sprintf(urlTemp + strlen(urlTemp), "/c%d",x);
                    break;
                } 
            }
        }
        if(fetch_content_object_for_name(urlTemp, suite, out, sizeof(out), &len, wait, sock, sa) < 0) {
            printf("timeout, retry not implemented, exit\n");
            exit(1);
        }
        // *** parse content *******************************
        unsigned char *t = (unsigned char*)out; 
        unsigned char **data = &t;
        int *datalen = &len;

        int typ;
        int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3;
        struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
        struct ccnl_prefix_s *prefix;
        unsigned char finalBlockId[1*1024];
        int finalBlockId_len = -1;
        unsigned char *cp = *data;


        if (ccnl_ndntlv_dehead(data, datalen, &typ, &len)) {
            printf("could not dehead\n");
            return -1;
        }
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
            if(finalBlockId_len > 0) {

                finalBlockId[finalBlockId_len] = 0;

                nChunks = atoi((const char *)&finalBlockId[1]) + 1;
                if(!dataOfChunks) {
                    dataOfChunks = calloc(nChunks, sizeof(unsigned char*));
                    datalenOfChunks = calloc(nChunks, sizeof(int));
                }

                int cmp_ind = prefix->compcnt-1;
                int cmp_len = prefix->complen[cmp_ind] - 1;
                char *buf[cmp_len];
                memmove(buf, prefix->comp[cmp_ind] + 1, cmp_len);

                int chunkNum = atoi((const char *)buf);

                dataOfChunks[chunkNum] = malloc(contlen * sizeof(unsigned char));
                memcpy(dataOfChunks[chunkNum], content, contlen);
                datalenOfChunks[chunkNum] = contlen;

                do_fetch_next = 0;
                for(int x = 0; x < nChunks; x++) {
                    if(!dataOfChunks[x]) {
                        do_fetch_next = 1;
                    } 
                }
            } else {
                goto Done;
            }
        }
    }

Done:
    if(!dataOfChunks) {
        write(1, content, contlen);
    } else {
        for(int x = 0; x < nChunks; x++) {
            write(1, dataOfChunks[x], datalenOfChunks[x]);
        }
    }
    close(sock);
    return 0; // avoid a compiler warning
}

// eof
