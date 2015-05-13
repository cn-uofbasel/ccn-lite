/*
 * @f ccn-lite-rfduino.c
 * @b a small server (and relay) for the RFDuino
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-05-12 created
 */

// configuration section

// choose a key that is at least 32 bytes long
const char secret_key[] PROGMEM = "some secret secret secret secret";

// ----------------------------------------------------------------------

#define CCNL_ARDUINO

// needed in the sha256 library code:
#define LITTLE_ENDIAN  1234
#define BYTE_ORDER     LITTLE_ENDIAN
#define ETH_ALEN       6

// RFduino specific (bad macro definition in their pgmspace.h file)
#undef sprintf_P
#define sprintf_P(...) sprintf(__VA_ARGS__)


//#define USE_CCNxDIGEST
#define USE_DEBUG                      // must select this for USE_MGMT
#define USE_DEBUG_MALLOC
//#define USE_FRAG
#define USE_ETHERNET
#define USE_HMAC256
//#define USE_HTTP_STATUS
//#define USE_IPV4
#define USE_LOGGING
//#define USE_MGMT
//#define USE_NACK
//#define USE_NFN
//#define USE_NFN_NSTRANS
//#define USE_NFN_MONITOR
//#define USE_SCHEDULER
//#define USE_SUITE_CCNB                 // must select this for USE_MGMT
// #define USE_SUITE_CCNTLV
// #define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
//#define USE_SUITE_LOCALRPC
//#define USE_UNIXSOCKET
//#define USE_SIGNATURES

#define NEEDS_PREFIX_MATCHING
#define NEEDS_PACKET_CRAFTING

double
GetTemp(void)
{
    return RFduino_temperature(CELSIUS);
}

void
ccnl_rfduino_get_MAC_addr(unsigned char *addr) // 6 bytes
{
    unsigned char *cp = ((unsigned char*) NRF_FICR->DEVICEADDR + ETH_ALEN - 1);
    byte i;

    if (!addr)
        return;

    *addr++ = *cp-- | 0xc0;
    for (i = 0; i < 5; i++)
        *addr++ = *cp--;
}

// scratchpad memory
static char logstr[128];
#define LOGSTROFFS 36  // where to put a %s parameter for the sprintf_P

// ----------------------------------------------------------------------
// dummy types

typedef struct XYZ {
    int a;
} EthernetUDP;

struct in_addr {
    unsigned long s_addr;
};

struct sockaddr_in {
    unsigned char sa_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
};

struct sockaddr_ll {
    unsigned char sa_family;
    unsigned short sll_protocol;
    unsigned char sll_addr[ETH_ALEN];
};

struct sockaddr {
    unsigned char sa_family;
    unsigned char sa_bits[8];
};

#define AF_PACKET 1
#define AF_INET   2

# define ntohs(i)  (((i<<8) & 0xff00) | ((i>>8) & 0x0ff))
# define htons(i)  ntohs(i)

unsigned long
ntohl(unsigned long l) {
    unsigned char r[4];
    unsigned char i = 4;
    while (i-- > 0) {
        r[i] = l & 0xff;
        l >>= 8;
    }
    return *(unsigned long*)r;
}

#define htonl(i) ntohl(i)

// ----------------------------------------------------------------------

#include <assert.h>

#include "ccnl-defs.h"
#include "ccnl-core.h"
#include "ccnl-ext.h"

extern char __heap_start;
extern char *__brkval;


// buffer to hold incoming and outgoing packet
static unsigned char packetBuffer[160];

unsigned char keyval[64];
unsigned char keyid[32];

// ----------------------------------------------------------------------

char*
ccnl_arduino_getPROGMEMstr(const char* s)
{
    strcpy_P(logstr + LOGSTROFFS, s);
    return logstr + LOGSTROFFS;
}

char*
inet_ntoa(struct in_addr a)
{
    static char result[16];
    unsigned long l = ntohl(a.s_addr);

    sprintf_P(result, PSTR("%ld.%lu.%lu.%lu"),
            (l>>24), (l>>16)&0x0ff, (l>>8)&0x0ff, l&0x0ff);
    return result;
}

#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
#include "ccnl-ext-logging.c"

#define ccnl_app_RX(x,y)                do{}while(0)
#define ccnl_print_stats(x,y)           do{}while(0)

char* ccnl_addr2ascii(sockunion *su);

// selectively enable/disable logging messages per module

#define DEBUGMSG_CORE(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_CFWD(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_CUTL(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_MAIN(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_PCNX(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_PIOT(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_PNDN(...) DEBUGMSG_ON(__VA_ARGS__)

#define DEBUGMSG(...)      DEBUGMSG_OFF(__VA_VARGS__)

extern struct ccnl_prefix_s sensor;

int local_producer(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt);

#include "ccnl-core.c"
#include "ccnl-ext-hmac.c"
#include "ccnl-ext-http.c"

/*
#include "ccnl-ext-localrpc.c"
#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-nfn.c"
#include "ccnl-ext-nfnmonitor.c"
#include "ccnl-ext-sched.c"
#include "ccnl-ext-frag.c"
#include "ccnl-ext-crypto.c"
*/

// ----------------------------------------------------------------------

static struct ccnl_relay_s theRelay;

#ifdef USE_SUITE_CCNTLV
static const char theSuite = CCNL_SUITE_CCNTLV;
#elif defined(USE_SUITE_IOTTLV)
static const char theSuite = CCNL_SUITE_IOTTLV;
#elif defined(USE_SUITE_NDNTLV)
static const char theSuite = CCNL_SUITE_NDNTLV;
#endif

struct timeval*
ccnl_run_events()
{
    static struct timeval now;
    long usec;

    gettimeofday(&now, 0);
    while (eventqueue) {
        struct ccnl_timer_s *t = eventqueue;

        usec = timevaldelta(&(t->timeout), &now);
        if (usec >= 0) {
            now.tv_sec = usec / 1000000;
            now.tv_usec = usec % 1000000;
            return &now;
        }
        if (t->fct)
            (t->fct)(t->node, t->intarg);
        else if (t->fct2)
            (t->fct2)(t->aux1, t->aux2);
        eventqueue = t->next;
        ccnl_free(t);
    }

    return NULL;
}

void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

int
local_producer(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
               struct ccnl_pkt_s *pkt)
{
    int offset = sizeof(packetBuffer);
    double d;

    DEBUGMSG_MAIN(VERBOSE, "local_producer %d bytes\n", pkt->buf->datalen);

    if (ccnl_prefix_cmp(&sensor, NULL, pkt->pfx, CMP_EXACT))
        return 0;

    d = GetTemp();
    sprintf_P(logstr, PSTR("@%s  %d.%1d C"), timestamp(),
              (int)d, (int)(10*(d - (int)d)));
    switch (pkt->pfx->suite) {

#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
#ifdef USE_HMAC256
        ccnl_ccntlv_prependSignedContentWithHdr(pkt->pfx,
                   (unsigned char*) logstr, strlen(logstr),
                   NULL, NULL, keyval, keyid, &offset,
                   (unsigned char*) packetBuffer);
#else
        ccnl_ccntlv_prependContentWithHdr(pkt->pfx, (unsigned char*) logstr,
                                 strlen(logstr), NULL, NULL,
                                 &offset, (unsigned char*) packetBuffer);
#endif // USE_HMAC256
        break;
#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        ccnl_iottlv_prependReply(pkt->pfx, (unsigned char*) logstr,
                                 strlen(logstr), &offset, NULL, NULL,
                                 (unsigned char*) packetBuffer);
        ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offset, packetBuffer);
        break;
#endif

#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
#ifdef USE_HMAC256
        ccnl_ndntlv_prependSignedContent(pkt->pfx,
                   (unsigned char*) logstr, strlen(logstr),
                   NULL, NULL, keyval, keyid, &offset,
                   (unsigned char*) packetBuffer);
#else
        ccnl_ndntlv_prependContent(pkt->pfx, (unsigned char*) logstr,
                                   strlen(logstr), NULL, NULL,
                                   &offset, (unsigned char*) packetBuffer);
#endif // USE_HMAC256
        break;
#endif

    default:
        break;
    }

#ifdef XXX
    relay->ifs[from->ifndx].sock->beginPacket(
        from->peer.ip4.sin_addr.s_addr,from->peer.ip4.sin_port);
    relay->ifs[from->ifndx].sock->write(packetBuffer + offset,
                                        sizeof(packetBuffer) - offset);
    relay->ifs[from->ifndx].sock->endPacket();
#endif

    free_packet(pkt);

    TRACEOUT();

    return 1;
}

// ----------------------------------------------------------------------
void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf)
{
// forwarding not yet implemented for Arduino

/*
    int rc;

    if (dest) {
    switch(dest->sa.sa_family) {
    case AF_INET:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ip4, sizeof(struct sockaddr_in));
        DEBUGMSG(DEBUG, "udp sendto %s/%d returned %d\n",
                 inet_ntoa(dest->ip4.sin_addr), ntohs(dest->ip4.sin_port), rc);
        break;
#ifdef USE_ETHERNET
    case AF_PACKET:
        rc = ccnl_eth_sendto(ifc->sock,
                             dest->eth.sll_addr,
                             ifc->addr.eth.sll_addr,
                             buf->data, buf->datalen);
        DEBUGMSG(DEBUG, "eth_sendto %s returned %d\n",
                 eth2ascii(dest->eth.sll_addr), rc);
        break;
#endif
#ifdef USE_UNIXSOCKET
    case AF_UNIX:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ux, sizeof(struct sockaddr_un));
        DEBUGMSG(DEBUG, "unix sendto %s returned %d\n",
                 dest->ux.sun_path, rc);
        break;
#endif
    default:
        DEBUGMSG(WARNING, "unknown transport\n");
        break;
    }
    rc = 0; // just to silence a compiler warning (if USE_DEBUG is not set)
*/
    DEBUGMSG(WARNING, "unknown transport\n");
}

void
ccnl_close_socket(EthernetUDP *udp)
{
}

// ----------------------------------------------------------------------

unsigned long
ccnl_rfduino_run_events(struct ccnl_relay_s *relay)
{
    struct timeval *timeout;
    timeout = ccnl_run_events();
    if (timeout)
        return 1000 * timeout->tv_sec + timeout->tv_usec / 1000;
    else
        return 10;
}


// ----------------------------------------------------------------------

struct ccnl_prefix_s sensor;
int sensor_len[1];
#ifdef USE_SUITE_CCNTLV
char* sensor_comp[1] = {"\x00\x01\x00\x04TinC"};
#else
char* sensor_comp[1] = {"TinC"};
#endif

void
debug_delta(char up)
{
#ifdef USE_DEBUG
    if (up && debug_level < TRACE)
        debug_level++;
    else if (debug_level > FATAL)
        debug_level--;

    DEBUGMSG(FATAL, "debug level now at %d (%c)\n",
             debug_level, ccnl_debugLevelToChar(debug_level));
#endif
}

struct {
    uint8_t sp_l, sp_h;
} stackPtr;

int
ccnl_rfduino_init(struct ccnl_relay_s *relay)
{
    int i;
    struct sockaddr_ll mac;

    Serial.println(F("This is 'ccn-lite-rfduino'"));
    Serial.println("  ccnl-core: " CCNL_VERSION);
    Serial.println("  compile time: " __DATE__ " "  __TIME__);
    Serial.print(  "  compile options: ");
    strcpy_P(logstr, PSTR(compile_string));
    Serial.println(logstr);
    Serial.print(  "  using suite ");
    Serial.println(ccnl_suite2str(theSuite));

    DEBUGMSG_MAIN(INFO, "configuring the relay\n");

    ccnl_core_init();

/*
    theRelay.ifcount = 1;
    theRelay.ifs[0].addr.ip4.sa_family = AF_INET;
    theRelay.ifs[0].addr.ip4.sin_addr.s_addr = IPaddr;
    theRelay.ifs[0].addr.ip4.sin_port = port;
    theRelay.ifs[0].sock = udp;
*/
    // init interfaces here

    relay->max_cache_entries = 0;
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);

    mac.sa_family = AF_PACKET;
    ccnl_rfduino_get_MAC_addr(mac.sll_addr);
    mac.sll_protocol = htons(0x6363); // it does not matter
    DEBUGMSG_MAIN(INFO, "  MAC addr is %s\n",
                  ccnl_addr2ascii((sockunion*) &mac));

    sensor.suite = theSuite;
#ifdef USE_SUITE_CCNTLV
    sensor_len[0] = 8; // 4 + strlen("TinC");
#else
    sensor_len[0] = strlen(sensor_comp[0]);
#endif
    sensor.comp = (unsigned char**) sensor_comp;
    sensor.complen = sensor_len;
    sensor.compcnt = 1;
    DEBUGMSG_MAIN(INFO, "  temp sensor at lci:%s\n", ccnl_prefix_to_path(&sensor));

#ifdef USE_HMAC256
    strcpy_P(logstr, secret_key);
    ccnl_hmac256_keyval((unsigned char*)logstr, strlen(logstr), keyval);
    ccnl_hmac256_keyid((unsigned char*)logstr, strlen(logstr), keyid);
#endif


/*
{
  asm volatile(
    // Determine where to store the new SP_H and SP_L register
    "ldi r26, lo8(stackPtr)     \n\t"  // X is loaded with the symbol arr
    "ldi r27, hi8(stackPtr)     \n\t"  // which is the address of arr[0].sp_l
    // Here you may want to add 2 * an array index.

    // This part stores the stack pointer low and high byte indirectly
    // using the X registers (r26,27), which is what I want.
    "in    r0, __SP_L__    \n\t" 
    "st    x+, r0          \n\t" 
    "in    r0, __SP_H__    \n\t"
    "st    x+, r0          \n\t"
  ::);
}
*/
/*    sprintf_P(logstr, PSTR("  brkval=%p, stackptr=%p (%d bytes free)"),
              __brkval, (char *)AVR_STACK_POINTER_REG,
              (char *)AVR_STACK_POINTER_REG - __brkval);
    Serial.println(logstr);
*/

#ifndef USE_DEBUG
    Serial.println("logging not available");
#endif

    return 0;
}

// eof
