// ccn-lite/src/arduino/src-ethernet.ino

// for the W5200 Ethernet Shield (Seeed studio, v2.4)

#include <SD.h>
#include <SPI.h>          // needed for Arduino versions later than 0018
#include <EthernetV2_0.h>
#include <EthernetUdpV2_0.h>
#include <avr/io.h>

#define W5200_CS   10   // Ethernet controller SPI CS pin
#define SDCARD_CS   4   // SD card SPI CS pin

#define IP_ADDR          IPAddress(192,168,2,222)
#define LOCAL_UDP_PORT1  6363   // local UDP port to listen on
#define LOCAL_UDP_PORT2  9695   // local UDP port to listen on


EthernetUDP Udp;

#include "../../ccn-lite-arduino.c"

void setup()
{
    Serial.begin(9600);
    strcpy_P(logstr, PSTR(">>"));
    Serial.println(logstr);

#ifdef USE_DEBUG
    debug_level = WARNING;
#endif

    // make sure that the SD card is not selected for the SPI port
    // by setting the CS pin to HIGH
    pinMode(SDCARD_CS,OUTPUT);
    digitalWrite(SDCARD_CS,HIGH);  // disable SD card
    pinMode(W5200_CS, OUTPUT);
    digitalWrite(W5200_CS, LOW);   //Enable Ethernet

//  Ethernet.begin(mac_addr);                           // DHCP
    Ethernet.begin(mac_addr, IP_ADDR); // manual assignment
    Udp.begin(LOCAL_UDP_PORT1);

    ccnl_arduino_init(&theRelay, mac_addr,
                      (unsigned long int) Ethernet.localIP(),
                      htons(LOCAL_UDP_PORT1), &Udp);
#ifdef USE_DEBUG
#ifdef USE_DEBUG_MALLOC
    Serial.println(F("  use '+', '-' to change verbosity, 'd' for heap dump\n"));
#else
    Serial.println(F("  use '+', '-' to change verbosity\n"));
#endif
#endif
}

void loop()
{
    unsigned long len;
    EthernetUDP *udp = theRelay.ifs[0].sock;

    while (Serial.available()) {
        char c = Serial.read();
        switch (c) {
        case '+': debug_delta(1); break;
        case '-': debug_delta(0); break;
#ifdef USE_DEBUG_MALLOC
        case 'd': debug_memdump(); break;
#endif
        default:
            Serial.println();
            break;
       }
    }

    len = udp->parsePacket();
    if (len > 0) {
        struct sockaddr sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_family = AF_INET;

        len = udp->read(packetBuffer, sizeof(packetBuffer));
        ((struct sockaddr_in*) (&sa))->sin_addr.s_addr = udp->remoteIP();
        ((struct sockaddr_in*) (&sa))->sin_port = udp->remotePort();
        ccnl_core_RX(&theRelay, 0, packetBuffer, len, &sa, sizeof(sa));
    }

    delay(ccnl_arduino_run_events(&theRelay));
}

// eof
