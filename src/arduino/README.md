# ccn-lite for Arduino, README.md

## Device:
  * seeeduino and Arduino-Uno (both with atmega328, 2K RAM)

## Tools:

  * gcc-avr
  * ino

## How to run:

  * Change the board model in the Makefile

  * At the prompt type

    > % make all upload serial

  * which will output the DEBUGMSGs. Use the + and - key to change the verbosity level

<pre><code>
    Terminal ready
    >>
    mem addr of relay = 494

    Use '+' and '-' to change verbosity
    0.003 I  debug level now at 3 (I)

    0.040 I  This is 'ccn-lite-relay' for Arduino
    0.090 I    ccnl-core: 2015-04-26
    0.125 I    compile time: Apr 27 2015 18:00:12
    0.174 I    compile options: SUITE_IOTTLV, 
    0.220 I    using suite iot2014
    0.253 I  configuring the relay
    0.286 I  config done, starting the loop
    8.294 D  debug level now at 4 (D)
    9.296 V  debug level now at 5 (V)
    10.297 T  debug level now at 6 (T)
    10.298 T  ageing t=10
    11.300 T  ageing t=11
    12.301 T  ageing t=12
</code></pre>

eof
