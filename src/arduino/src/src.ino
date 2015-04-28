// ccnlite/src.ino

#define LED_PIN 13

#include "../../ccn-lite-arduino.c"

void setup()
{
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(9600);
    strcpy_P(logstr, PSTR(">>"));
    Serial.println(logstr);
    strcpy_P(logstr, PSTR("mem addr of relay = "));
    Serial.print(logstr);
    Serial.println((int) &theRelay);
    Serial.println();

    debug_level = WARNING;
    strcpy_P(logstr, PSTR("Use '+' and '-' to change verbosity, 'd' for heap dump"));
    Serial.println(logstr);
    debug_delta(1);
    Serial.println();

    ccnl_arduino_init(&theRelay);
}

void loop()
{
    unsigned long timeout;

    while (Serial.available()) {
        char c = Serial.read();
        switch (c) {
          case '+': debug_delta(1); break;
          case '-': debug_delta(0); break;
          case 'd': debug_memdump(); break;
          default:  break;
       }
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
