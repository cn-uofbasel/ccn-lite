// ccn-lite/src/rfduino/src/src.ino

// #include <pgmspace.h>
#include <RFduinoBLE.h>
#include <Arduino.h>
#include <libRFduino.h>
#include <variant.h>
#include <nrf51.h>

extern "C" {
  // unfortunately, the Arduino IDE requires absolute path names:
  #define CCN_LITE_RFDUINO_C "/home/ubuntu/ccn-lite/src/ccn-lite-rfduino.c"
  #include CCN_LITE_RFDUINO_C
}

unsigned char msg[1];

// uint8_t uuid[16] = {0xff, 0xC5, 0x6D, 0xB5, 0xDF, 0xFB, 0x48, 0xD2, 0xB0, 0x60, 0xD0, 0xF5, 0xA7, 0x10, 0x96, 0xff}; //Custom iBeacon UUID

int flag = false;
int announced = 0;

void setup()
{
  unsigned char *cp;
  int i;

//  override_uart_limit = true;
//  Serial.begin(38400);
  Serial.begin(9600);
  Serial.println();
  Serial.println("ccn-lite-rfduino started");

  // start BLE ...
  // (deviceName length plus the advertisement length must be <= 18 bytes)
  RFduinoBLE.deviceName = "ccn-lite";
  RFduinoBLE.advertisementData = "-rfd";
  /*
  RFduinoBLE.iBeacon = true;
  memcpy(RFduinoBLE.iBeaconUUID, uuid, sizeof(RFduinoBLE.iBeaconUUID));
  RFduinoBLE.iBeaconMajor = 1234; //Custom iBeacon Major
  RFduinoBLE.iBeaconMinor = 5678; //Custom iBeacon Minor
  */
  RFduinoBLE.begin();

  ccnl_rfduino_init(&theRelay);
//  msglen = ccnl_fillmsg(msg, sizeof(msg));

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

  if (flag && !announced) {
       delay(800);
       announced = 1;
       Serial.print(RFduinoBLE.send((char*) msg, sizeof(msg)));
       Serial.println(" send done");
   }

   timeout = ccnl_rfduino_run_events(&theRelay);
   delay(timeout);
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
//    RFduinoBLE.send(msg, strlen(msg));
    flag = true;
}

void RFduinoBLE_onReceive(char *data, int len)
{
//    sprintf(logstr, "received %d bytes", len);
//    Serial.println(logstr);

    ccnl_ll_RX(data, len);
}

void RFduinoBLE_onDisconnect()
{
    flag = false;
    announced = 0;
    Serial.println("disconnect");
}

// eof
