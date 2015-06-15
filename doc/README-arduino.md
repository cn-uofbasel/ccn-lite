[//]: # (TODO: Link to the homepage of the specific boards?)
[//]: # (TODO: Add 'standard' header to each README?)

This README describes the installation and usage of CCN-lite for the family of Arduino boards (Arduino Uno and AtMega328, 2KiB RAM) and the RFduino (32 KiB RAM).

Because each of these devices come with a different setup including different communication shields, this leads to many different profiles that need to be supported by CCN-lite. Currently, working setups include:
* Uno with Ethernet shield (UDP)
* AtMega 328 with Ethernet shield (UDP)
* RFduino with Bluetooth low energy

Planned but not yet supported setups include:
* Uno with WiFly

# Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up the CCN-lite sources and relevant environment variables.

The installation for Arduino boards depends on `gcc-avr` and `ino` (http://inotool.org/). For Ubuntu, you can install `gcc-avr` using the following command:

```bash
sudo apt-get install gcc-avr
```

Building CCN-lite for RFduino depends on the Arduino IDE (http://arduino.cc/).


# Installation for Arduino boards

1.  Change into the source directory related to Arduino:

    ```bash
    cd $CCNL_HOME/src/arduino
    ```

2.  Change the `$BOARD` variable in the `Makefile` to match your board:

    ```bash
    sed -i 's/^BOARD=.*$/BOARD=<yourboard>/' Makefile
    ```

3.  Copy the `src-*.ino` file matching your shield into the `src` directory. For example, using the Ethernet shield, type:

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

    Use the keys `+` and `-` to increase/decrease the verbosity level. Press `d` to dump the heap.

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


# Installation for RFduino

1.  Install the RFduino support for the Arduino IDE according to:
    http://www.rfduino.com/wp-content/uploads/2014/04/RFduino.Quick_.Start_.Guide_.pdf

2.  Open the file `.../ccn-lite-src/arduino/rfduino/rfduino.ion` in the Arduino IDE

3.  Compile and upload the code by running `verify/compile` and `upload`.

4.  Connect to your device by using the Serial Monitor (Tools > Serial Monitor) or through the ino command line with:

    ```bash
    ino serial
    ```


# Compile time options

[//]: # (TODO: Rework text)

The Arduino's 32KB flash limits the number of ccn-lite extensions that
can be included at the same time. For example, you have to trade
malloc() debugging support for HMAC signature generation.

You can enable/disable compile time options in the file ccn-lite-arduino.c
by commenting out the USE_XXX macros definitions. Not all combinations
will work, not all have been tested for the Arduino environment.

The comment on ccn-lite compile time options in the previous section
in principle also applies for the FRduino, but with 128KB, flash
size is not an issue, really.
