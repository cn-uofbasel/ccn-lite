// ccnlite/src.ino

#include <SD.h>
#include <SPI.h>          // needed for Arduino versions later than 0018
#include <EthernetV2_0.h>
#include <EthernetUdpV2_0.h>
#include <avr/io.h>

#define W5200_CS  10   // Ethernet controller SPI CS pin
#define SDCARD_CS 4    // SD card SPI CS pin

// assign different MAC addresses for each sensor, as a sensor's reading
// will be accessible under "cli:/<mac-addr-in-hex>/temp"
byte mac[] = {0x55, 0x42, 0x41, 0x53, 0x45, 0x4c};

// choose a key that is at least 32 bytes long
const char key[] PROGMEM = "some secret secret secret secret";

#define LOCALPORT  6360   // local UDP port to listen on
EthernetUDP Udp;

#define LED_PIN 13

#include "../../ccn-lite-arduino.c"

void setup()
{
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(9600);
    strcpy_P(logstr, PSTR(">>"));
    Serial.println(logstr);

#ifdef USE_DEBUG
    debug_level = WARNING;
    debug_delta(1);
#endif

    // make sure that the SD card is not selected for the SPI port
    // by setting the CS pin to HIGH
    pinMode(SDCARD_CS,OUTPUT);
    digitalWrite(SDCARD_CS,HIGH);  // disable SD card
    pinMode(W5200_CS, OUTPUT);
    digitalWrite(W5200_CS, LOW);   //Enable Ethernet

//  Ethernet.begin(mac);                           // DHCP
    Ethernet.begin(mac, IPAddress(192,168,2,222)); // manual assignment
    Udp.begin(LOCALPORT);

    ccnl_arduino_init(&theRelay, mac, key,
                      (unsigned long int) Ethernet.localIP(),
                      htons(LOCALPORT), &Udp);

#ifdef USE_DEBUG_MALLOC
    DEBUGMSG_ON(INFO, "Use '+', '-' to change verbosity, 'd' for heap dump\n");
#else
    DEBUGMSG_ON(INFO, "Use '+', '-' to change verbosity\n");
#endif
}

void loop()
{
    unsigned long timeout, len;
    EthernetUDP *udp = theRelay.ifs[0].sock;

    while (Serial.available()) {
        char c = Serial.read();
        Serial.println();
        switch (c) {
          case '+': debug_delta(1); break;
          case '-': debug_delta(0); break;
#ifdef USE_DEBUG_MALLOC
          case 'd': debug_memdump(); break;
#endif
          default:  break;
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
 
    timeout = ccnl_arduino_run_events(&theRelay);

    while (timeout > 20) {
      digitalWrite(LED_PIN, HIGH);
      delay(20);
      digitalWrite(LED_PIN, LOW);
      if (timeout <= 20)
          break;
      timeout -= 20;
      if (timeout < 100) {
          delay(timeout);
          break;
      } else {
          delay(100);
          timeout -= 100;
      }
    }
}

// eof
