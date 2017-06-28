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
                           int datalen, char *res)
{
    char name[CCNL_MAX_PACKET_SIZE];
    int len = 0, i;
    struct timespec ts;
    long timestamp_milli;

    for (i = 0; i < prefix->compcnt; ++i) {
        len += sprintf(name+len, "/%s", prefix->comp[i]);
    }

    len = 0;
    len += sprintf(res + len, "{\n");
    len += sprintf(res + len, "\"packetLog\":{\n");

//      len += sprintf(res + len, "\"from\":{\n");
//      len += sprintf(res + len, "\"host\": %d,\n", fromip);
//      len += sprintf(res + len, "\"port\": %d\n", fromport);
//      len += sprintf(res + len, "\"type\": \"NFNNode\", \n");
//      len += sprintf(res + len, "\"prefix\": \"docrepo1\"\n");
//      len += sprintf(res + len, "},\n"); //from

    len += sprintf(res + len, "\"to\":{\n");
    len += sprintf(res + len, "\"host\": \"%s\",\n", toip);
    len += sprintf(res + len, "\"port\": %d\n", toport);
    len += sprintf(res + len, "},\n"); //to

    len += sprintf(res + len, "\"isSent\": %s,\n", "true");

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
    len += sprintf(res + len, "\"timestamp\": %lu,\n", timestamp_milli);

    len += sprintf(res + len, "\"packet\":{\n");
    len += sprintf(res + len, "\"type\": \"%s\",\n", data != NULL ? "content" : "interest" );
    len += sprintf(res + len, "\"name\": \"%s\"\n", name);

    if(data){
            //size_t newlen;
            //char *newdata = base64_encode(data, datalen, &newlen);

            len += sprintf(res + len, ",\"data\": \"%s\"\n", data);
    }
    len += sprintf(res + len, "}\n"); //packet

    len += sprintf(res + len, "}\n"); //packetlog
    len += sprintf(res + len, "}\n");

    return len;
}

int
ccnl_ext_nfnmonitor_udpSendTo(int sock, char *address, int port,
                               char *data, int len)
{
    struct sockaddr_in si_other;

    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(port);
    if (inet_aton(address, &si_other.sin_addr)==0) {
          fprintf(stderr, "inet_aton() failed\n");
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
#ifdef USE_IPV4
        int l = ccnl_ext_nfnmonitor_record(inet_ntoa(face->peer.ip4.sin_addr),
                                           ntohs(face->peer.ip4.sin_port),
                                           pr, data, len, monitorpacket);
#elif defined(USE_IPV6)
        char str[INET6_ADDRSTRLEN];
        int l = ccnl_ext_nfnmonitor_record((char*) inet_ntop(AF_INET6,
                                                             face->peer.ip6.sin6_addr.s6_addr,
                                                             str,
                                                             INET6_ADDRSTRLEN),
                                           ntohs(face->peer.ip6.sin6_port), pr,
                                           data, len, monitorpacket);
#endif
        ccnl_ext_nfnmonitor_sendToMonitor(ccnl, monitorpacket, l);
    }

    TRACEOUT();
    return 0;
}

#endif // USE_NFN_MONITOR

// eof
