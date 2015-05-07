# ccn-lite for Arduino, README.md

## Device:

  * seeeduino or Arduino-Uno (both with atmega328, 2K RAM)
  * Ethernet shield

## Tools:

  * gcc-avr
  * ino

## How to run:

  * Change the board model in the Makefile

  * At the prompt type

    > % make all upload serial

  * which will output the DEBUGMSGs.

  * Use the + and - key to change the verbosity level

<pre><code>
    Terminal ready
    >>
    [I] 0.301 This is 'ccn-lite-arduino'
    [I] 0.301   ccnl-core: 2015-05-07
    [I] 0.313   compile time: May  7 2015 19:05:15
    [I] 0.363   compile options: DEBUG, DEBUG_MALLOC, LOGGING, SUITE_IOTTLV, 
    [I] 0.441   using suite iot2014
    [I] 0.475 configuring the relay
    [I] 0.509   UDP port 192.168.2.222/6360
    [I] 0.552   temp sensor at lci:/63636e6c6974/temp
    [I] 0.605 Use '+', '-' to change verbosity, 'd' for heap dump
    [V] 5.554   request=</63636e6c6974/temp>
    9.579 @@@ memory dump starts
    9.580 @@@ mem 0x49c    23 Bytes, allocated by ../ccnl-os-time.c:229 @8.579
    9.622 @@@ mem 0x50f    28 Bytes, allocated by ../ccnl-core.c:141 @5.552
    9.698 @@@ mem 0x4e8    25 Bytes, allocated by ../ccnl-core-util.c:637 @0.551
    9.779 @@@ mem 0x4c1    25 Bytes, allocated by ../ccnl-core-util.c:634 @0.551
    9.861 @@@ memory dump ends
</code></pre>

eof
