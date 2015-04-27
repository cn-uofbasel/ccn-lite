# ccn-lite for Arduino, README.md

## Device:
  * seeeduino and Arduino-Uno (both with atmega328, 2K RAM)

## Tools:

  * gcc-avr
  * ino

## How to run:

  * change the board model in the Makefile

  * at the prompt type

    > % make all upload serial

  * which will output the DEBUGMSGs:

    > Terminal ready
    > >>
    > mem addr of relay = 448
    > 
    > 0.000 This is ccn-lite-relay
    > 0.001   ccnl-core: 2015-04-26
    > 0.027   compile time: Apr 27 2015 15:30:25
    > 0.073   compile options: SUITE_IOTTLV, 
    > 0.115   using suite iot2014
    > 0.146 configuring the relay
    > 0.177 config done, starting the loop
    > 1.177 ageing t=1
    > 2.178 ageing t=2
    > 3.180 ageing t=3

eof
