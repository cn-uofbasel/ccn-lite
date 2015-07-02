# CCN-lite for Android

This README describes the preliminary port of CCN-lite for Android devices. The
port is a fully functional CCN/NDN forwarder with Bluetooth Low Energy support.

It mostly is a C program, containing Java code only for the user interface and
compiled using the Android's Native Developer Kit. Check the
[release page](https://github.com/cn-uofbasel/ccn-lite/releases) of CCN-lite for
a precompiled binary for all Android-supported CPUs.

The platform-specific code for Android is located in the sub-directory
`$CCNL_HOME/src/android`. The main CCN-lite sources get included (`#include`)
during the Android build process.


## Remarks

The main use of CCN-lite for Android is for local demonstrations and Bluetooth
Low Energy debugging. For example, the relay can serve content. The debugging
output is shown in the application's main screen with an option to change the
debugging level interactively. Additionally, the HTTP status console also works
and is available at port 8080.

Currently known issues include:
 - Screen refresh has issues when the screen orientation changes
 - Reconnecting to a Bluetooth device is not fully supported
 - The CCN-lite Android port currently requires SDK 18. Ideally, earlier versions
   should be supported as well if Bluetooth Low Energy is not required.


## Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up
the CCN-lite sources and relevant environment variables.

To build CCN-lite for Android, the
[Android SDK](https://developer.android.com/sdk) and
[NDK](https://developer.android.com/tools/sdk/ndk) are both required. Do not
forget to adapt/define your environment variables `$PATH` and `$ANDROID_HOME`.


## Installation

1.  Change to the Android directory of CCN-lite and start the Android GUI:

    ```bash
    cd $CCNL_HOME/src/android
    android &
    ```

    Select the three Android SDKs "Tools", "Platform-tools" and "Build-tools".
    Optionally, you can include the ARM EABI v7a System image as well.

    See `AndroidManifest.xml` for the already defined Android project.

2.  Build the native code of CCN-lite using the NDK:

    ```bash
    ndk-build
    ```

    Notice that you may have to specify which target. Use `android list targets`
    for a list of possible targets.

3.  Build the Android application:

    ```bash
    ant debug
    ```

4.  Install the build on your device or the emulator:

    ```bash
    adb install -r bin/ccn-lite-android-debug.apk
    ```

    Notice that your device must have USB debugging enabled and be connected to
    your development environment via USB.

5.  Start your Bluetooth Low Energy device.

6.  Start the CCN-lite application.
