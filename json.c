#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "ccnl-core.h"

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "util/base64.c"

int
create_packet_log(/*long fromip, int fromport,*/ char* toip, int toport,
	struct ccnl_prefix_s *prefix, char *data, int datalen, char *res)
{
    char name[CCNL_MAX_PACKET_SIZE];
    int len = 0;
    for(int i = 0; i < prefix->compcnt; ++i){
        len += sprintf(name+len, "/%s", prefix->comp[i]);
    }

    len = 0;
    len += sprintf(res + len, "{\n");
    len += sprintf(res + len, "\"packetLog\":{\n");

//	len += sprintf(res + len, "\"from\":{\n");
//	len += sprintf(res + len, "\"host\": %d,\n", fromip);
//	len += sprintf(res + len, "\"port\": %d\n", fromport);
//	len += sprintf(res + len, "\"type\": \"NFNNode\", \n");
//	len += sprintf(res + len, "\"prefix\": \"docrepo1\"\n");
//	len += sprintf(res + len, "},\n"); //from

    len += sprintf(res + len, "\"to\":{\n");
    len += sprintf(res + len, "\"host\": \"%s\",\n", toip);
    len += sprintf(res + len, "\"port\": %d\n", toport);
    len += sprintf(res + len, "},\n"); //to

    len += sprintf(res + len, "\"isSent\": %s,\n", "true");

    struct timespec ts;

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

    long timestamp_milli =
        ((ts.tv_sec) * 1000000000 + (ts.tv_nsec ));
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

int udp_sendto(int sock, char *address, int port, char *data, int len){
    int rc;
    struct sockaddr_in si_other;
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(port);
    if (inet_aton(address, &si_other.sin_addr)==0) {
          fprintf(stderr, "inet_aton() failed\n");
          exit(1);
    }
    rc = sendto(sock, data, len, 0, (const struct sockaddr *)&si_other, sizeof(si_other));
    return rc;
}

int
sendtomonitor(struct ccnl_relay_s *ccnl, char *content, int contentlen){
    char *address = "127.0.0.1";
    int port = 10666;
    //int s = socket(PF_INET, SOCK_DGRAM, 0);
    int s = ccnl->ifs[0].sock;
    return udp_sendto(s, address, port, content, contentlen);
}
