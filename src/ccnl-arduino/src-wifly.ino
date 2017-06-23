// ccn-lite/src/arduino/src-wifly.ino

// for the WiFly shield v3.0 (Seeed studio)

// DOES NOT WORK YET !

#define ARDUINO_SHIELD_WIFLY

// ----------------------------------------------------------------------

#ifdef ARDUINO_SHIELD_WIFLY

#include <WiFly.h>
#include <SoftwareSerial.h>

SoftwareSerial uart(2, 3);
WiFly wifi(&uart);

#define SSID   "HUAWEI-E5220-2c3c"
#define PASSWD "yourpasswd"
#define AUTH   WIFLY_AUTH_WPA2_PSK   // or WIFLY_AUTH_WPA1, WIFLY_AUTH_WEP, WIFLY_AUTH_OPEN

unsigned long time_point = 0;

void setup()
{
    uart.begin(9600);
    Serial.begin(9600);

  Serial.println("--------- WIFLY TEST --------");

  // wait for initilization of wifly
  delay(1000);

  uart.begin(9600);     // WiFly UART Baud Rate: 9600

  wifi.reset();

  Serial.println("Join " SSID );
  if (wifi.join(SSID, PASSWD, AUTH)) {
    Serial.println("OK");
  } else {
    Serial.println("Failed");
  }

  Serial.println("setup");

  wifi.sendCommand("set ip proto 0");
  wifi.sendCommand("set ip host 0.0.0.0");
  wifi.sendCommand("set ip remote 0");
  wifi.sendCommand("set ip local 8888");
  wifi.sendCommand("save");
  wifi.sendCommand("reboot");
  delay(1000);

/*
  wifi.sendCommand("save");
  Serial.println("5");
  wifi.sendCommand("reboot");
  Serial.println("6");
*/

  delay(200);
//  wifi.sendCommand("get ip");

//  wifi.clear();

  wifi.commandMode();
//  delay(500);

/*
  wifi.sendCommand("set ip proto 0");
  wifi.sendCommand("set ip host 0.0.0.0");
  wifi.sendCommand("set ip remote 0");
  wifi.sendCommand("set ip local 8888");

set ip proto 0
set ip host 0.0.0.0
set ip remote 0
set ip local 8888
save
reboot

*/
//  wifi.sendCommand("save");
//  wifi.sendCommand("reboot");

//  wifi.dataMode();
//  delay(300);

}

int cnt = 0;

void loop() {
  unsigned char buffer[200];

  while (wifi.available()) {
    Serial.print((char)wifi.read());
  }
  while (Serial.available()) {
    wifi.print((char)Serial.read());
  }

/*
    int len = wifi.available();
    Serial.print("len=");
    Serial.println(len);
    len = wifi.receive((uint8_t *) buffer, sizeof(buffer), 100);
//    Serial.write((char*) buffer);
    Serial.print("len2=");
    Serial.println(len);
  }
*/

/*
  while (Serial.available()) {
    strcpy(buffer, "ok, done!\n");
    wifi.send(buffer, strlen(buffer));
  }
*/

/*
  if ((millis() - time_point) > 1000) {
    time_point = millis();
    wifi.send("hey hey hey\r\n",100);
  }
*/
}

#endif

// ======================================================================

#ifdef ETHERNET_SHIELD

#include <SD.h>
#include <SPI.h>          // needed for Arduino versions later than 0018
#include <EthernetV2_0.h>
#include <EthernetUdpV2_0.h>
#include <avr/io.h>

#define LED_PIN    13
#define W5200_CS   10   // Ethernet controller SPI CS pin
#define SDCARD_CS   4   // SD card SPI CS pin

EthernetUDP Udp;

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

//  Ethernet.begin(mac_addr);                           // DHCP
    Ethernet.begin(mac_addr, IPAddress(192,168,2,222)); // manual assignment
    Udp.begin(LOCAL_UDP_PORT);

    ccnl_arduino_init(&theRelay, mac_addr,
                      (unsigned long int) Ethernet.localIP(),
                      htons(LOCAL_UDP_PORT), &Udp, secret_key);
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

#endif // ETHERNET_SHIELD

// eof
