/*
 * @f ccn-lite-arduino.c
 * @b a small server (and relay) for the Arduino platform
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
 * 2015-04-25 created
 */

// configuration section


// assign different MAC addresses for each sensor, as a sensor's reading
// will be accessible under "cli:/<mac-addr-in-hex>/temp"
unsigned char mac_addr[] = {0x55, 0x42, 0x41, 0x53, 0x45, 0x4c};

// ----------------------------------------------------------------------

#define CCNL_ARDUINO
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN

/*
    Flash (and RAM) memory is tight on the Arduino Uno: carefully
    enable the minimal subset of compile time options.
    What worked for me (May 2015), for example, was

    USE_DEBUG, USE_MALLOC, USE_IPV4, USE_NDNTLV
  or
    USE_DEBUG, USE_LOGGING, USE_IPV4, USE_NDNTLV
  or
    USE_HMAC256, USE_IPV4, USE_NDNTLV

  but
    USE_DEBUG, USE_HMAC256, USE_IPV4, USE_NDNTLV
  was too big, while
    USE_DEBUG, USE_HMAC256, USE_IPV4, USE_IOTTLV
  worked again.

    See also the section with the DEBUGMSG_OFF() etc defines, which is
    another place to tweak.
 */

//#define USE_CCNxDIGEST
#define USE_DEBUG                      // must select this for USE_MGMT
// #define USE_DEBUG_MALLOC
#define USE_DUP_CHECK
//#define USE_FRAG
//#define USE_LINKLAYER
//#define USE_HMAC256
//#define USE_HTTP_STATUS
#define USE_IPV4
//#define USE_LOGGING
//#define USE_MGMT
//#define USE_NACK
//#define USE_NFN
//#define USE_NFN_NSTRANS
//#define USE_NFN_MONITOR
//#define USE_SCHEDULER
//#define USE_STATS
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
GetTemp(void) // http://playground.arduino.cc/Main/InternalTemperatureSensor
{
    unsigned int wADC;
    double t;

    // The internal temperature has to be used
    // with the internal reference of 1.1V.
    // Channel 8 can not be selected with
    // the analogRead function yet.

    // Set the internal reference and mux.
    ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
    ADCSRA |= _BV(ADEN);  // enable the ADC

    delay(20);            // wait for voltages to become stable.

    ADCSRA |= _BV(ADSC);  // Start the ADC

    // Detect end-of-conversion
    while (bit_is_set(ADCSRA,ADSC));

    // Reading register "ADCW" takes care of how to read ADCL and ADCH.
    wADC = ADCW;

    // The offset of 324.31 could be wrong. It is just an indication.
    // t = (wADC - 324.31 ) / 1.22;
    t = (wADC - 338) / 1.22;

    // The returned temperature is in degrees Celcius.
    return (t);
}


// ----------------------------------------------------------------------
// dummy types

struct in_addr {
    unsigned long s_addr;
};

struct sockaddr_in {
    unsigned char sa_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
};

struct sockaddr {
    unsigned char sa_family;
    unsigned char sa_bits[sizeof(struct sockaddr_in)];
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

typedef long ssize_t;

// ----------------------------------------------------------------------

#include <assert.h>

#include "ccnl-defs.h"
#include "ccnl-core.h"
#include "ccnl-ext.h"

extern char __heap_start;
extern char *__brkval;

// scratchpad memory
static char logstr[128];
#define LOGSTROFFS 36  // where to put a %s parameter for the sprintf_P

// buffer to hold incoming and outgoing packet
static unsigned char packetBuffer[160];

#ifdef USE_HMAC256
// choose a key that is at least 32 bytes long
const char secret_key[] PROGMEM = "some secret secret secret secret";
unsigned char keyval[64];
unsigned char keyid[32];
#endif

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
    unsigned long l = ntohl(a.s_addr);
    //    static char result[16];
    //    sprintf_P(result, PSTR("%ld.%lu.%lu.%lu"),
    //        (l>>24), (l>>16)&0x0ff, (l>>8)&0x0ff, l&0x0ff);
    //    return result;

    sprintf_P(logstr, PSTR("%ld.%lu.%lu.%lu"),
              (l>>24), (l>>16)&0x0ff, (l>>8)&0x0ff, l&0x0ff);
    return logstr;
}

#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
#include "ccnl-ext-logging.c"

#define ccnl_app_RX(x,y)                do{}while(0)
#define ccnl_print_stats(x,y)           do{}while(0)
#define cache_strategy_remove(...)      0

char* ccnl_addr2ascii(sockunion *su);

// selectively enable/disable logging messages per module

#define DEBUGMSG_CORE(...) DEBUGMSG_OFF(__VA_ARGS__)
#define DEBUGMSG_CFWD(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_CUTL(...) DEBUGMSG_OFF(__VA_ARGS__)
#define DEBUGMSG_MAIN(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_PCNX(...) DEBUGMSG_OFF(__VA_ARGS__)
#define DEBUGMSG_PIOT(...) DEBUGMSG_OFF(__VA_ARGS__)
#define DEBUGMSG_PNDN(...) DEBUGMSG_OFF(__VA_ARGS__)

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
#include "ccnl-ext-crypto.c"
*/
//#include "ccnl-ext-frag.c"

// ----------------------------------------------------------------------

static struct ccnl_relay_s theRelay;

#ifdef USE_SUITE_CCNTLV
static const char theSuite = CCNL_SUITE_CCNTLV;
#elif defined(USE_SUITE_IOTTLV)
static const char theSuite = CCNL_SUITE_IOTTLV;
#elif defined(USE_SUITE_NDNTLV)
static const char theSuite = CCNL_SUITE_NDNTLV;
#endif

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

    DEBUGMSG_MAIN(INFO, "  outgoing data=<%s> to=%s\n",
                  ccnl_prefix_to_path(pkt->pfx),
                  ccnl_addr2ascii(&from->peer));

    relay->ifs[from->ifndx].sock->beginPacket(
        from->peer.ip4.sin_addr.s_addr,from->peer.ip4.sin_port);
    relay->ifs[from->ifndx].sock->write(packetBuffer + offset,
                                        sizeof(packetBuffer) - offset);
    relay->ifs[from->ifndx].sock->endPacket();

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
#ifdef USE_LINKLAYER
    case AF_PACKET:
        rc = ccnl_eth_sendto(ifc->sock,
                             dest->linklayer.sll_addr,
                             ifc->addr.linklayer.sll_addr,
                             buf->data, buf->datalen);
        DEBUGMSG(DEBUG, "eth_sendto %s returned %d\n",
                 ll2ascii(dest->linklayer.sll_addr), rc);
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

// returns the number of millisecs to sleep
unsigned long
ccnl_arduino_run_events(struct ccnl_relay_s *relay)
{
    long int usec = ccnl_run_events();

    return usec >= 0 ? usec/1000 : 10;
}


// ----------------------------------------------------------------------

struct ccnl_prefix_s sensor;
int sensor_len[2];
#ifdef USE_SUITE_CCNTLV
char sensor_mac[16]; // ascii (hex) representation + 4 CCNx TL bytes
char* sensor_comp[2] = {sensor_mac, "\x00\x01\x00\x04temp"};
#else
char sensor_mac[12];
char* sensor_comp[2] = {sensor_mac, "temp"};
#endif

void
debug_delta(char up)
{
#ifdef USE_DEBUG
    if (up && debug_level < TRACE)
        debug_level++;
    else if (!up && debug_level > FATAL)
        debug_level--;

    DEBUGMSG_MAIN(FATAL, "debug level now at %d (%c)\n",
                  debug_level, ccnl_debugLevelToChar(debug_level));
#endif
}

int
ccnl_arduino_init(struct ccnl_relay_s *relay, unsigned char *mac,
                  unsigned long int IPaddr, unsigned short port,
                  EthernetUDP *udp)
{
    int i;

    Serial.println(F("This is 'ccn-lite-arduino'"));
    Serial.println(F("  ccnl-core: " CCNL_VERSION));
    Serial.println(F("  compile time: " __DATE__ " "  __TIME__));
    Serial.print(  F("  compile options: "));
    strcpy_P(logstr, compile_string);
    Serial.println(logstr);
    Serial.print(  F("  using suite "));
    Serial.println(ccnl_suite2str(theSuite));

    debug_delta(1);
    DEBUGMSG_MAIN(INFO, "configuring the relay\n");

    ccnl_core_init();

    theRelay.ifcount = 1;
    theRelay.ifs[0].addr.ip4.sa_family = AF_INET;
    theRelay.ifs[0].addr.ip4.sin_addr.s_addr = IPaddr;
    theRelay.ifs[0].addr.ip4.sin_port = port;
    theRelay.ifs[0].sock = udp;

    // init interfaces here
    DEBUGMSG_MAIN(INFO, "  UDP port %s\n",
                  ccnl_addr2ascii(&theRelay.ifs[0].addr));

    relay->max_cache_entries = 0;
    relay->max_pit_entries = CCNL_DEFAULT_MAX_PIT_ENTRIES;
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);

    sensor.suite = theSuite;
#ifdef USE_SUITE_CCNTLV
    sensor_mac[1] = 1;
    sensor_mac[3] = 12;
    for (i = 0; i < 6; i++)
        sprintf_P(sensor_mac + 4 + 2*i, PSTR("%02x"), mac[i]);
    sensor_len[0] = sizeof(sensor_mac);
    sensor_len[1] = 8; // 4 + strlen("temp");
#else
    for (i = 0; i < 6; i++)
        sprintf_P(sensor_mac + 2*i, PSTR("%02x"), mac[i]);
    sensor_len[0] = sizeof(sensor_mac);
    sensor_len[1] = strlen(sensor_comp[1]);
#endif
    sensor.comp = (unsigned char**) sensor_comp;
    sensor.complen = sensor_len;
    sensor.compcnt = 2;
    DEBUGMSG_MAIN(INFO, "  temp sensor at lci:%s\n", ccnl_prefix_to_path(&sensor));

#ifdef USE_HMAC256
    strcpy_P(logstr, secret_key);
    ccnl_hmac256_keyval((unsigned char*)logstr, strlen(logstr), keyval);
    ccnl_hmac256_keyid((unsigned char*)logstr, strlen(logstr), keyid);
#endif

    DEBUGMSG_MAIN(INFO, "  brkval=%p, stackptr=%p (%d bytes free)",
              __brkval, (char *)AVR_STACK_POINTER_REG,
                  (char *)AVR_STACK_POINTER_REG - __brkval);

#ifndef USE_DEBUG
    Serial.println(F("logging not available"));
#endif

    return 0;
}

// eof
