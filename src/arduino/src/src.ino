// ccnlite/src.ino

struct ccnl_relay_s;

#include "../../ccn-lite-arduino.c"

struct ccnl_relay_s theRelay;

#define LED_PIN 13

void setup()
{
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(9600);
    Serial.println("hello world\n");

    ccnl_arduino_init(&theRelay);
}

void loop()
{
    int timeout;

    long addr = (long) &theRelay;

    Serial.print("\nloop ");
    Serial.print(millis()/100.0);
    Serial.print(" relaysize=");
    Serial.print(sizeof(struct ccnl_relay_s));
    Serial.print(" relayaddr=");
    Serial.print(addr);
    Serial.print(" timeout=");

    timeout = ccnl_arduino_run_events(&theRelay);
    Serial.println(timeout);

    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(900);
}

// eof
