# ccn-lite for Arduino, README.md

This is the README file for the family of Arduino boards (Arduino Uno
and AtMega328, 2KB RAM) and the RFduino (32 KB RAM).

Because each device comes with a different communication shield, this
leads to many profiles:

  * Uno with Ethernet shield (=UDP, works)
  * AtMega328 with Ethernet shield (=UDP, works)
  * Uno with WiFly (does not work yet)
  * Uno with Bluetooth shield (planned)
  * RFduino (includes BTLE, works)
  * etc

Except for the RFduino, we build the Arduino code with command line
tools.  A symbolic link in the src subdirectory is used to switch to a
different profile.

The RFduino code is built with the Arduino IDE and has its own
subdirectory called rfduino.


## Install instructions for the generic Arduino board:

  * Install
    * gcc-avr
    * ino

  * change directory to .../ccn-lite/src/arduino

  * in the Makefile, change the board model

  * copy the desired shield-specific ino-file to the src directory

    > % cp src-ethernet.ino src/src.ino

  * at the UNIX prompt, type

    > % make all upload

  * now you can connect to your device:

    > % make serial

    which will output the DEBUGMSGs.

  * use the + and - key to change the verbosity level

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
    9.580 @@@ mem 0x49c    23 Bytes, allocated by ccnl-os-time.c:229 @8.579
    9.622 @@@ mem 0x50f    28 Bytes, allocated by ccnl-core.c:141 @5.552
    9.698 @@@ mem 0x4e8    25 Bytes, allocated by ccnl-core-util.c:637 @0.551
    9.779 @@@ mem 0x4c1    25 Bytes, allocated by ccnl-core-util.c:634 @0.551
    9.861 @@@ memory dump ends
</code></pre>


### Compile time options

The Arduino's 32KB flash limits the number of ccn-lite extensions that
can be included at the same time. For example, you have to trade
malloc() debugging support for HMAC signature generation.

You can enable/disable compile time options in the file ccn-lite-arduino.c
by commenting out the USE_XXX macros definitions. Not all combinations
will work, not all have been tested for the Arduino environment.


## Install instructions for the RFduino:

  * install the Arduino IDE (1.6.3)

  * from within the IDE, install RFduino support according to http://www.rfduino.com/wp-content/uploads/2014/04/RFduino.Quick_.Start_.Guide_.pdf

  * open the file .../ccn-lite-src/arduino/rfduino/rfduino.ino

  * click "verify/compile" (or type ctrl-R)

  * click "upload" (or type ctrl-U)

  * click tools->Serial Monitor (or type ctrl-shift-M), or use the
  ino tool at the command line:

    > % ino serial

The comment on ccn-lite compile time options in the previous section
in principle also applies for the FRduino, but with 128KB, flash
size is not an issue, really.

eof
