/*
 * @f ccnl-ext-nfnmonitor.c
 * @b CCN lite, logging support
 *
 * Copyright (C) 2014, Christopher Scherb, University of Basel
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
 * 2014-04-27 created
 */

#ifdef USE_NFN_MONITOR

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <sys/time.h>

#ifdef __MACH__
# include <mach/clock.h>
# include <mach/mach.h>
#endif

#include "ccnl-core.h"
#include "util/base64.c"

int
ccnl_ext_nfnmonitor_record(char* toip, int toport,
                           struct ccnl_prefix_s *prefix, unsigned char *data,
                           int datalen, char *res, unsigned int reslen)
{
    char name[CCNL_MAX_PACKET_SIZE];
    char *tmpBuf = name;
    unsigned int remLen = CCNL_ARRAY_SIZE(name), totalLen = 0;
    int i;
    struct timespec ts;
    long timestamp_milli;

    // Build name
    for (i = 0; i < prefix->compcnt; ++i) {
        tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "/%.*s",
                               prefix->complen[i], prefix->comp[i]);
    }
    if (!tmpBuf)
        DEBUGMSG(ERROR, "Encoding error occured while constructing name from prefix.\n");

    if (totalLen >= CCNL_ARRAY_SIZE(name))
        DEBUGMSG(ERROR, "Name buffer has not enough capacity for prefix. Available: %zu, needed: %u\n",
                 CCNL_ARRAY_SIZE(name), totalLen + 1);


    // Build res
    tmpBuf = res;
    remLen = reslen;
    totalLen = 0;

    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "{\n");
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"packetLog\":{\n");

//      tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"from\":{\n");
//      tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"host\": %d,\n", fromip);
//      tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"port\": %d\n", fromport);
//      tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"type\": \"NFNNode\", \n");
//      tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"prefix\": \"docrepo1\"\n");
//      tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "},\n"); //from

    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"to\":{\n");
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"host\": \"%s\",\n", toip);
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"port\": %d\n", toport);
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "},\n"); //to

    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"isSent\": %s,\n", "true");

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif

    timestamp_milli = ((ts.tv_sec) * 1000000000 + (ts.tv_nsec ));
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"timestamp\": %lu,\n",
                           timestamp_milli);

    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"packet\":{\n");
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"type\": \"%s\",\n",
                           data != NULL ? "content" : "interest" );
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "\"name\": \"%s\"\n", name);

    if(data){
            //size_t newlen;
            //char *newdata = base64_encode(data, datalen, &newlen);
            tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, ",\"data\": \"%s\"\n",
                                   data);
    }
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "}\n"); //packet

    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "}\n"); //packetlog
    tmpBuf = ccnl_snprintf(tmpBuf, &remLen, &totalLen, "}\n");

    if (!tmpBuf) {
        DEBUGMSG(ERROR, "An encoding error occured while constructing monitor recording.\n");
        return -2;
    }

    if (totalLen >= reslen) {
        DEBUGMSG(ERROR, "Result buffer has not enough capacity for monitor recording. Available: %u, needed: %u\n",
                 reslen, totalLen + 1);
        return -1;
    }

    return totalLen;
}

int
ccnl_ext_nfnmonitor_udpSendTo(int sock, char *address, int port,
                               char *data, int len)
{
    struct sockaddr_in si_other;
    if (ccnl_setIpSocketAddr(&si_other, address, port) == 0) {
          fprintf(stderr, "ccnl_setIpSocketAddr() failed\n");
          exit(1);
    }
    return sendto(sock, data, len, 0,
                (const struct sockaddr *)&si_other, sizeof(si_other));
}

int
ccnl_ext_nfnmonitor_sendToMonitor(struct ccnl_relay_s *ccnl,
                                  char *content, int contentlen)
{
    char *address = "127.0.0.1";
    int port = 10666, s = ccnl->ifs[0].sock;
    //int s = socket(PF_INET, SOCK_DGRAM, 0);

    return ccnl_ext_nfnmonitor_udpSendTo(s, address, port, content, contentlen);
}

int
ccnl_nfn_monitor(struct ccnl_relay_s *ccnl,
                 struct ccnl_face_s *face,
                 struct ccnl_prefix_s *pr,
                 unsigned char *data,
                 int len)
{
    TRACEIN();

    if (face) {
        char monitorpacket[CCNL_MAX_PACKET_SIZE];
        int l = ccnl_ext_nfnmonitor_record(inet_ntoa(face->peer.ip4.sin_addr),
                                           ntohs(face->peer.ip4.sin_port),
                                           pr, data, len, monitorpacket,
                                           CCNL_ARRAY_SIZE(monitorpacket));
        ccnl_ext_nfnmonitor_sendToMonitor(ccnl, monitorpacket, l);
    }

    TRACEOUT();
    return 0;
}

#endif // USE_NFN_MONITOR

// eof
