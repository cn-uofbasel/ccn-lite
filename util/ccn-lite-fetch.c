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
ccntlv_mkInterest(struct ccnl_prefix_s *name, int *chunknum, int *nonce,
                  unsigned char *out, int outlen)
{
    int len, offset;

    offset = outlen;
    len = ccnl_ccntlv_fillChunkInterestWithHdr(name, chunknum, &offset, out);

    return len;
}

int
ndntlv_mkInterest(struct ccnl_prefix_s *name, int *chunknum, int *nonce,
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
fetch_content_object_for_name_chunk(char* name, 
                                    int *chunknum,
                                    int suite, 
                                    unsigned char *out, int out_len, 
                                    int *len, 
                                    float wait, int sock, struct sockaddr sa) {

    struct ccnl_prefix_s *prefix;
    int (*mkInterest)(struct ccnl_prefix_s*, int*, int*, unsigned char*, int);
    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        DEBUGMSG(99, "CCNB not implemented\n");
        exit(-1);
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        prefix = ccnl_URItoPrefix(name, suite, NULL);
        mkInterest = ccntlv_mkInterest;
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:

        if(chunknum) {
            sprintf(name + strlen(name), "/c%d", *chunknum);
        } 
        prefix = ccnl_URItoPrefix(name, suite, NULL);
        mkInterest = ndntlv_mkInterest;
        break;
#endif
    default:
        DEBUGMSG(99, "unknown suite\n");
        exit(-1);
    }

    char temp_name[1024];
    strcpy(temp_name, name);

    int nonce = random();

    *len = mkInterest(prefix, chunknum, &nonce, out, out_len);

    if (sendto(sock, out, *len, 0, &sa, sizeof(sa)) < 0) {
        perror("sendto");
        myexit(1);
    }
    if (block_on_read(sock, wait) <= 0) {
        DEBUGMSG(99, "timeout after block_on_read");
        return -1;
    }
    *len = recv(sock, out, out_len, 0);

    return 0;
}

int ndntlv_extract_data_and_chunkinfo(unsigned char **data, int *datalen, 
                                      int *chunknum, int *lastchunknum,
                                      unsigned char **content, int *contentlen) {
    int typ, len;
    unsigned char *cp = *data;
    unsigned char finalBlockId[1*1024];
    int finalBlockId_len = -1;
    int mbf=0, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, scope=3;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkl=0;
    struct ccnl_prefix_s *prefix;

    if (ccnl_ndntlv_dehead(data, datalen, &typ, &len)) {
        DEBUGMSG(99, "could not dehead\n");
        return -1;
    }
    buf = ccnl_ndntlv_extract(*data - cp,
                  data, datalen,
                  &scope, &mbf, &minsfx, &maxsfx, finalBlockId, &finalBlockId_len,
                  &prefix, 
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
        if(finalBlockId_len > 0) {
            finalBlockId[finalBlockId_len] = 0;
            *lastchunknum = atoi((const char *)&finalBlockId[1]);

            // Extract the chunknum from the last component having the from 'c<int>' (e.g. c13)
            int lastcmpind = prefix->compcnt-1;
            *chunknum = atoi((const char *)(prefix->comp[lastcmpind] + 1));
        } else {
            DEBUGMSG(99, "Received finalBlockId of size smaller than 1\n");
            return -1;
        }
    }
    return 0;
}
int ccntlv_extract_data_and_chunkinfo(unsigned char **data, int *datalen, 
                                      int *chunknum, int *lastchunknum,
                                      unsigned char **content, int *contentlen) {


    struct ccnl_prefix_s *prefix;
    *datalen -= 8;
    *data += 8;
    int hdrlen = 8;

    if(ccnl_ccntlv_extract(hdrlen,
                           data, datalen,
                           &prefix,
                           0, 0, // keyid/keyidlen
                           chunknum, lastchunknum,
                           content, contentlen) == NULL) {
        fprintf(stderr, "Error in ccntlv_extract\n");
        exit(-1);
    } 
    return 0;
}
int extract_data_and_chunkinfo(unsigned char **data, int *datalen, 
                               int suite, 
                               int *chunknum, int *lastchunknum,
                               unsigned char **content, int *contentlen) {
    int result = -1;
    switch(suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        result = ccntlv_extract_data_and_chunkinfo(data, datalen, chunknum, lastchunknum, content, contentlen);
    break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        result = ndntlv_extract_data_and_chunkinfo(data, datalen, chunknum, lastchunknum, content, contentlen);
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
    int i, len, opt, sock = 0, suite = CCNL_SUITE_NDNTLV;
    char *udp = NULL, *ux = NULL;
    struct sockaddr sa;
    float wait = 3.0;

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

    char url[1024];
    char origUrl[1024];
    strcpy(origUrl, argv[optind]);

    unsigned char *content = 0;
    int contlen;

    unsigned char **dataPerChunk = 0;
    int *datalenOfChunks = 0;
    int numberofchunks = -1;
    int curchunknum = 0;
    int do_fetch_next = 1;


    while(do_fetch_next) {

        if(dataPerChunk) {
            for(int x = 0; x < numberofchunks; x++) {
                if(!dataPerChunk[x]) {
                    curchunknum = x;
                    break;
                } 
            }
        }

        if(curchunknum >= 0) {
            DEBUGMSG(99, "fetching chunk %d...\n", curchunknum);
        } else {
            DEBUGMSG(99, "fetching any chunk...\n");
        }

        strcpy(url, origUrl);
        if(fetch_content_object_for_name_chunk(url, 
                                               curchunknum >= 0 ? &curchunknum : NULL, 
                                               suite, 
                                               out, sizeof(out), 
                                               &len, 
                                               wait, sock, sa) < 0) {
            fprintf(stderr, "timeout, retry not implemented, exit\n");
            exit(1);
        }

        int chunknum = -1, lastchunknum = -1;
        unsigned char *t = &out[0];
        if(extract_data_and_chunkinfo(&t, &len, suite, 
                                      &chunknum, &lastchunknum, 
                                      &content, &contlen) < 0) {
            DEBUGMSG(99, "Could not extract data and chunkinfo\n");
            goto Done;
        } else {
            DEBUGMSG(99, "extracted chunknum %d lastchunknum %d \n", chunknum, lastchunknum);
        }

        if(lastchunknum >= 0) {
            DEBUGMSG(99, "processing chunk %d\n", chunknum);
            numberofchunks = lastchunknum + 1;
            if(!dataPerChunk) {
                dataPerChunk = calloc(numberofchunks, sizeof(unsigned char*));
                datalenOfChunks = calloc(numberofchunks, sizeof(int));
            }
            dataPerChunk[chunknum] = malloc(contlen * sizeof(unsigned char));
            memcpy(dataPerChunk[chunknum], content, contlen);
            datalenOfChunks[chunknum] = contlen;

            do_fetch_next = 0;
            for(i = 0; i < numberofchunks; i++) {
                if(!dataPerChunk[i]) {
                    do_fetch_next = 1;
                } 
            }

        } else {
            if(dataPerChunk) {
                DEBUGMSG(99, "ERROR: Received single, non-chunked content but there were already received chunks for name.\n");
                exit(-1);
            }
            DEBUGMSG(99, "Received single, non-chunked content\n");
            goto Done;
        }
    }

Done:
    if(!dataPerChunk) {
        write(1, content, contlen);
    } else {
        for(int x = 0; x < numberofchunks; x++) {
            write(1, dataPerChunk[x], datalenOfChunks[x]);
        }
    }
    close(sock);
    return 0; // avoid a compiler warning
}

// eof
