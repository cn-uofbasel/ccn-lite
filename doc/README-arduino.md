# CCN-lite for Arduino

This README describes the installation and usage of CCN-lite for the
family of Arduino boards ([Arduino
Uno](https://www.arduino.cc/en/Main/ArduinoBoardUno) and
[AtMega328](http://www.atmel.com/devices/atmega328.aspx), 32KiB Flash, 2 KiB
RAM) and the [RFduino](http://www.rfduino.com/) (128KiB Flash, 32 KiB RAM).

Because each of these devices come with a different setup including different
communication shields, this leads to many different profiles that need to be
supported by CCN-lite. Currently, working setups include:
* Uno with Ethernet shield (UDP)
* AtMega328 with Ethernet shield (UDP)
* RFduino with Bluetooth Low Energy

Planned but not yet supported setups include:
* Uno with WiFly

The platform-specific code for Arduino is located in the sub-directory
`$CCNL_HOME/src/arduino`. The main CCN-lite sources get included (`#include`)
during the Arduino build process.

## Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up the
CCN-lite sources and relevant environment variables.

Moreover, the installation for Arduino boards depends on `gcc-avr` and
[`ino`](http://inotool.org/). For Ubuntu, you can install `gcc-avr` using the
following command:

```bash
sudo apt-get install gcc-avr
```

Building CCN-lite for RFduino requires the [Arduino IDE](http://arduino.cc/).


## Installation for Arduino boards

*See [below](#installation-for-rfduino) for the installation for RFduino.*

1.  Change into the source directory related to Arduino:

    ```bash
    cd $CCNL_HOME/src/arduino
    ```

2.  Change the `$BOARD` variable in the `Makefile` to match your board:

    ```bash
    sed -i 's/^BOARD=.*$/BOARD=<yourboard>/' Makefile
    ```

3.  Copy the `src-*.ino` file matching your shield into the `src` directory.
    For example, using the Ethernet shield, type:

    ```bash
    cp src-ethernet.ino src/src.ino
    ```

4.  Build CCN-lite and upload the binaries:

    ```bash
    make all upload
    ```

5.  Connect to your device and see the debug output:

    ```bash
    make serial
    ```

    Use the keys `+` and `-` to increase and decrease the verbosity level. Press
    `d` to dump the heap.

    <pre><code>Terminal ready
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
    [V] 5.554   request=&lt;/63636e6c6974/temp&gt;
    9.579 @@@ memory dump starts
    9.580 @@@ mem 0x49c    23 Bytes, allocated by ccnl-os-time.c:229 @8.579
    9.622 @@@ mem 0x50f    28 Bytes, allocated by ccnl-core.c:141 @5.552
    9.698 @@@ mem 0x4e8    25 Bytes, allocated by ccnl-core-util.c:637 @0.551
    9.779 @@@ mem 0x4c1    25 Bytes, allocated by ccnl-core-util.c:634 @0.551
    9.861 @@@ memory dump ends</code></pre>


## Installation for RFduino

1.  Install the RFduino support for the Arduino IDE according to the GitHub
    README:

    https://github.com/RFduino/RFduino/blob/master/README.md

    Follow the instructions but add one additional step: After you have configured your additional board manager for RFduino, *first*
    download the "Arduino SAM Boards" (32bits ARM Cortex-M3). Only then download and install "RFduino Boards".

2.  Change the absolute path of the `#include` of `src/ccn-lite-rfduino.c` in
    `src/arduino/rfduino/rfduino.ino` to match your CCN-lite home directory
    `$CCNL_HOME`:

    ```bash
    cd $CCNL_HOME/src/arduino/rfduino
    sed "s|\(^\s*#define CCN_LITE_RFDUINO_C\).*$|\1 \"$CCNL_HOME/src/ccn-lite-rfduino.c\"|" rfduino.ino > rfduino.ino.sed
    mv rfduino.ino.sed rfduino.ino
    ```

    This is because the Arduino IDE does not support relative `#include` paths.

3.  Compile and upload the code by running `verify/compile` and `upload`.

4.  Connect to your device by using the Serial Monitor (inside the IDE:
    `Tools > Serial Monitor`) or on the command line with `ino`:

    ```bash
    ino serial
    ```


## Compile time options

The Arduino's flash memory of 32 KiB limits the number of CCN-lite extensions
that can be included at the same time. For example, `malloc()` debugging support
cannot be enabled at the same time as HMAC signature generation. Note that size
is not an issue for the RFduino because of its 128 KiB flash memory.

To enable or disable compile time options, comment out specific macro
definitions in the form of:

```C
#define USE_*
```

## Predefined content

To simplify debugging and testing the relays they already contain predefined content: the CPU's internal temperature. Both `ccn-lite-arduino.c` and `ccn-lite-rfduino.c` define under which name the content is addressed:

 - `ccn-lite-arduino.c`: `/aabbccddeeff/temp`, where the device's MAC address is the first name component. The MAC address is reported in the initial output of the serial console.
 - `ccn-lite-rfduino.c`: `/TinC` and `/TinF`, returning the temperature in Celcius and Farenheit, respectively.
