// ccn-lite/src/rfduino/src/src.ino

// #include <pgmspace.h>
#include <RFduinoBLE.h>
#include <Arduino.h>
#include <libRFduino.h>
#include <variant.h>
#include <nrf51.h>

extern "C" {
  #include "/home/tschudin/ccnlite/ccn-lite-20150510/src/ccn-lite-rfduino.c"
//  #include "../../ccn-lite-rfduino.c"
}

char *msg = "hello worl123456789012345678901234567890";
uint8_t uuid[16] = {0xff, 0xC5, 0x6D, 0xB5, 0xDF, 0xFB, 0x48, 0xD2, 0xB0, 0x60, 0xD0, 0xF5, 0xA7, 0x10, 0x96,
0xff}; //Custom iBeacon UUID

void setup()
{
  unsigned char *cp;
  int i;
  
  Serial.begin(9600);
  strcpy_P(logstr, PSTR(">>"));
  Serial.println(logstr);

#ifdef USE_DEBUG
  debug_level = WARNING;
  debug_delta(1);
#endif

  // start BLE ...
  RFduinoBLE.iBeacon = true;
  RFduinoBLE.deviceName = "ccn-lite-rfd";
  memcpy(RFduinoBLE.iBeaconUUID, uuid, sizeof(RFduinoBLE.iBeaconUUID));
  RFduinoBLE.iBeaconMajor = 1234; //Custom iBeacon Major
  RFduinoBLE.iBeaconMinor = 5678; //Custom iBeacon Minor

  RFduinoBLE.begin();

  ccnl_rfduino_init(&theRelay);

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

  /*
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
  */

  timeout = 100; // ccnl_arduino_run_events(&theRelay);
  delay(timeout);

  /*
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
  */

}

void RFduinoBLE_onAdvertisement(bool start)
{
    Serial.print(start);
    Serial.println(" adv");
}

/*
void RFduinoBLE_onRSSI(int rssi)
{
    Serial.print(rssi);
    Serial.println(" rssi");
}
*/

void RFduinoBLE_onConnect()
{
    Serial.println("connect");
    RFduinoBLE.send(msg, strlen(msg));
}

void RFduinoBLE_onReceive(char *data, int len)
{
    sprintf(logstr, "received %d bytes", len);
    Serial.println(logstr);
}

void RFduinoBLE_onDisconnect()
{
    Serial.println("disconnect");
}

// eof

