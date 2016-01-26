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
#define USE_DUP_CHECK
//#define USE_FRAG
#define USE_LINKLAYER
//#define USE_HMAC256
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
#define USE_SUITE_CCNTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
//#define USE_SUITE_LOCALRPC
//#define USE_UNIXSOCKET
//#define USE_SIGNATURES

#define NEEDS_PREFIX_MATCHING
#define NEEDS_PACKET_CRAFTING

double
GetTemp(int scale)
{
    return RFduino_temperature(scale);
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

uint16_t ntohs(uint16_t val)
{
    return (val<<8) | ((val >> 8) & 0x0ff);
}

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
static unsigned char packetBuffer[160]; // space enough for signature bytes

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
#define DEBUGMSG_EFRA(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_MAIN(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_PCNX(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_PIOT(...) DEBUGMSG_ON(__VA_ARGS__)
#define DEBUGMSG_PNDN(...) DEBUGMSG_ON(__VA_ARGS__)

#define DEBUGMSG(...)      DEBUGMSG_OFF(__VA_VARGS__)

#define local_producer(...) 0

#include "ccnl-core.c"
#include "ccnl-ext-frag.c"
#include "ccnl-ext-hmac.c"

/*
#include "ccnl-ext-http.c"
#include "ccnl-ext-localrpc.c"
#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-nfn.c"
#include "ccnl-ext-nfnmonitor.c"
#include "ccnl-ext-sched.c"
#include "ccnl-ext-crypto.c"
*/

// ----------------------------------------------------------------------

static struct ccnl_relay_s theRelay;

void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

void tempInX(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
             struct ccnl_prefix_s *p, struct ccnl_buf_s *buf)
{
    int offset = sizeof(packetBuffer), i, len;
    double d;
    unsigned char *cp, c;

    DEBUGMSG_MAIN(VERBOSE, "tempInX %s bytes\n", ccnl_prefix_to_path(p));

    if (p->compcnt != 1)
        return;

#ifdef USE_FRAG
    if (from && !from->frag)
        from->frag = ccnl_frag_new(CCNL_FRAG_BEGINEND2015,
                                   relay->ifs[from->ifndx].mtu);
#endif

    cp = p->comp[0];
#ifdef USE_SUITE_CCNTLV
    if (p->suite == CCNL_SUITE_CCNTLV)
      c = cp[7];
    else
#endif
    c = cp[3];
    d = GetTemp(c == 'C' ? CELSIUS : FAHRENHEIT);

    sprintf_P(logstr, PSTR("%03u %d.%1d"), (millis() / 1000) % 1000,
              (int)d, (int)(10*(d - (int)d)));

    if (strlen(logstr) > 8) // so that with ndn2013, the reply fits in 20B
        logstr[8] = '\0';

    switch (p->suite) {

#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
#ifdef USE_HMAC256
        ccnl_ccntlv_prependSignedContentWithHdr(p,
                   (unsigned char*) logstr, strlen(logstr),
                   NULL, NULL, keyval, keyid, &offset,
                   (unsigned char*) packetBuffer);
#else
        ccnl_ccntlv_prependContentWithHdr(p, (unsigned char*) logstr,
                                 strlen(logstr), NULL, NULL,
                                 &offset, (unsigned char*) packetBuffer);

/*
        {
            cp = packetBuffer + offset;
            len = sizeof(packetBuffer) - offset;
            DEBUGMSG_MAIN(INFO, "len = %d\n", len);
            while (len > 0) {
                DEBUGMSG_MAIN(INFO, "%02x %02x %02x %02x\n",
                              cp[0], cp[1], cp[2], cp[3]);
                len -= 4;
                cp += 4;
            }
        }
*/
#endif // USE_HMAC256
        break;
#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        ccnl_iottlv_prependReply(p, (unsigned char*) logstr,
                                 strlen(logstr), &offset, NULL, NULL,
                                 (unsigned char*) packetBuffer);
//        ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offset, packetBuffer);
        break;
#endif

#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
#ifdef USE_HMAC256
        ccnl_ndntlv_prependSignedContent(p,
                   (unsigned char*) logstr, strlen(logstr),
                   NULL, NULL, keyval, keyid, &offset,
                   (unsigned char*) packetBuffer);
#else
        ccnl_ndntlv_prependContent(p, (unsigned char*) logstr,
                                   strlen(logstr), NULL, NULL,
                                   &offset, (unsigned char*) packetBuffer);
#endif // USE_HMAC256
        break;
#endif

    default:
        break;
    }

    DEBUGMSG_MAIN(VERBOSE, "  reply pkt starts with %02x %02x %02x\n",
                  packetBuffer[offset], packetBuffer[offset+1],
                  packetBuffer[offset+2]);

    cp = packetBuffer + offset;
    len = sizeof(packetBuffer) - offset;

    ccnl_core_suites[(int)p->suite].RX(relay, NULL, &cp, &len);

    TRACEOUT();
}

// ----------------------------------------------------------------------

void
ccnl_ll_RX(char *data, int len)
{
    sockunion su;

    DEBUGMSG_MAIN(DEBUG, "ccnl_ll_RX %d bytes\n", len);

    memset(&su, 0, sizeof(su));
    su.sa.sa_family = AF_PACKET;

    ccnl_core_RX(&theRelay, 0, (unsigned char*) data, len,
                 &su.sa, sizeof(su));
}

void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf)
{
    int rc;

    DEBUGMSG_MAIN(TRACE, "ccnl_ll_TX %d bytes\n", buf ? buf->datalen : -1);

    rc = RFduinoBLE.send((char*) buf->data, buf->datalen);

    if (rc != 1) {
//        DEBUGMSG_MAIN(WARNING, "  send returns %d\n", rc);
        delay(50);
        rc = RFduinoBLE.send((char*) buf->data, buf->datalen);
        if (rc != 1) {
            DEBUGMSG_MAIN(WARNING, "  send returns %d (second attempt)\n", rc);
            return;
        }
    }
    DEBUGMSG_MAIN(TRACE, "  send returns %d\n", rc);
}

void
ccnl_close_socket(EthernetUDP *udp)
{
}

// ----------------------------------------------------------------------

unsigned long
ccnl_rfduino_run_events(struct ccnl_relay_s *relay)
{
    unsigned long timeout = ccnl_run_events();

    if (timeout)
        return timeout / 1000;
    else
        return 10;
}


// ----------------------------------------------------------------------

void
set_sensorName(char *name, int suite)
{
    struct ccnl_prefix_s *p;

    strcpy_P(logstr, name);
    p = ccnl_URItoPrefix(logstr, suite, NULL, NULL);
    DEBUGMSG_MAIN(INFO, "  temp sensor at lci:%s (%s)\n",
           ccnl_prefix_to_path(p), ccnl_suite2str(suite));

    ccnl_set_tap(&theRelay, p, tempInX);
}


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

struct {
    uint8_t sp_l, sp_h;
} stackPtr;

int
ccnl_rfduino_init(struct ccnl_relay_s *relay)
{
    int i;

    Serial.println(F("This is 'ccn-lite-rfduino'"));
    Serial.println("  ccnl-core: " CCNL_VERSION);
    Serial.println("  compile time: " __DATE__ " "  __TIME__);
    Serial.print(  "  compile options: ");
    strcpy_P(logstr, PSTR(compile_string));
    Serial.println(logstr);

#ifdef USE_DEBUG
  debug_level = WARNING;
  debug_delta(1);
#endif

    DEBUGMSG_MAIN(INFO, "configuring the relay\n");

    ccnl_core_init();

    relay->max_cache_entries = 0;
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);

    theRelay.ifcount = 1;
    theRelay.ifs[0].addr.sa.sa_family = AF_PACKET;
    ccnl_rfduino_get_MAC_addr(theRelay.ifs[0].addr.linklayer.sll_addr);
    theRelay.ifs[0].addr.linklayer.sll_protocol = htons(0x2222); // doesn't matter
    theRelay.ifs[0].mtu = 20;
    DEBUGMSG_MAIN(INFO, "  MAC addr is %s\n",
                  ccnl_addr2ascii(&theRelay.ifs[0].addr));

#ifdef USE_SUITE_CCNTLV
    set_sensorName(PSTR("/TinC"), CCNL_SUITE_CCNTLV);
    set_sensorName(PSTR("/TinF"), CCNL_SUITE_CCNTLV);
#endif
#ifdef USE_SUITE_IOTTLV
    set_sensorName(PSTR("/TinF"), CCNL_SUITE_IOTTLV);
    set_sensorName(PSTR("/TinC"), CCNL_SUITE_IOTTLV);
#endif
#ifdef USE_SUITE_NDNTLV
    set_sensorName(PSTR("/TinC"), CCNL_SUITE_NDNTLV);
    set_sensorName(PSTR("/TinF"), CCNL_SUITE_NDNTLV);
#endif

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
