# CCN-lite for Android

This README describes the preliminary port of CCN-lite for Android devices.

## Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up the
CCN-lite sources and relevant environment variables.

To build CCN-lite for Android, the Android SDK (https://developer.android.com/sdk) and NDK (https://developer.android.com/tools/sdk/ndk) are both required.

## Installation

[//]: # (Add step 'create a target'?)

1.  Build the native code of CCN-lite using the NDK:

    ```bash
    ndk-build
    ```

2.  Build the Android application:

    ```bash
    ant debug
    ```

3.  Install the build on your device or the emulator:

    ```bash
    adb install -r bin/ccn-lite-android-debug.apk
    ```


## Features

The current version of CCN-lite for Android provides a very limited set of functions. The following features work:
* Loading and initializing the relay
* All debug messages are writting to `/sdcard/ccn-lite.log`
* The HTTP status console. To test it, connect to your Android device on port 8080

Planned but not yet supported features include:
* Show debug messages in a scrollable container on the screen
* Include three buttons to increase and decrease the debug level as well as dump the heap memory.
