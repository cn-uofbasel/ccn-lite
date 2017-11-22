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

To build CCN-lite for Android, install Android Studio.


## Installation

1.  Open CMakeList.txt from CCNL_HOME/src/ccnl-android/native add replace ".." with absolut pathes

1.  Import CCNL_HOME/src/ccnl-android to Android Studio
    From an open project, select Tools > Android > SDK Manager from the menu bar.
    Click the SDK Tools tab.
    Check the boxes next to LLDB, CMake, and NDK.

2.  Right-click on the module you would like to link to your native library, such as the app module, and select Link C++ Project with Gradle from the menu. (Gradle Scropts/build.gradle (Module: app))
    Add the CMakeList.txt from CCNL_HOME/src/ccnl-android/native.

3.  Build the APK using Android Studio

4.  Install the build on your device using Android Studio or use the Emulator

    Notice that your device must have USB debugging enabled and be connected to
    your development environment via USB.

5.  Start your Bluetooth Low Energy device.

6.  Start the CCN-lite application.
