# CCN-lite for Android

This README describes the preliminary port of CCN-lite for Android devices.

The Android port is a fully functional CCN/NDN forwarder with
BlueTooth Low Energy support.

It has a little bit of Java code (for the UI) but otherwise is an
ordinary C program that is compiled with Android's Native Developer
Kit (NDK) -- the app will ship with binary code for all
Android-supported CPUs.

## What you get

* So far, the main use of the CCN-lite Android app is for local demos (the relay can serve content, for example) and for BTLE debugging.
* All CCN-lite debugging output is shown in the app's main screen where the debugging level can be changed intercactively.
* The HTTP status console also works and is available at port 8080.

## Limitations

* Screen refresh is not handled well (when you change screen orientation)
* BlueTooth re-connect is not handled well
* The CCN-lite Android port requires SDK v18: We should also support earlier versions if BTLE is not required.

## Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up
the CCN-lite sources and relevant environment variables. Note that the
glue code for Android is in a sub-directory of the CCN-lite C sources
and that the Android build process does an ```#include``` of the full
C code from that parent directory.

To build CCN-lite for Android, the Android SDK
(https://developer.android.com/sdk) and NDK
(https://developer.android.com/tools/sdk/ndk) are both required.

## Installation (for Ubuntu)

1.  Download the Android SDK and the NDK, select the packages with the
    GUI, and define some environment variables:

    ```bash
    export PATH=$PATH:/opt/android-sdk-linux/tools/
    export ANDROID_HOME=/opt/android-sdk-linux
    export PATH=$PATH:/opt/android-ndk-r10d/
    android &
    ```

    In the GUI, select the three Android SDK "Tools", "Platform-tools" and
    "Build-tools". I also included the ARM EABI v7a System Image.

2.  Change to the Android sub-directory of CCN-lite:

    ```bash
    cd ccn-lite/src/android
    ```

3.  The Android project is already defined, see AndroidManifest.xml

4.  Build the native code of CCN-lite using the NDK (you may have
    to specify which is the target, see ```android list targets```
    for possible values).

    ```bash
    ndk-build
    ```

5.  Build the Android application:

    ```bash
    ant debug
    ```

6.  Install the build on your device or the emulator. Your phone or tablet
    must have USB debugging enabled and be connected to your development
    environment via USB:

    ```bash
    adb install -r bin/ccn-lite-android-debug.apk
    ```

7.  Start your BTLE device *before* you start the CCN-lite app: discovery
    (by the app) is only done at startup time.

eof
