# ccn-lite for Android, README.md

This is a preliminary port for Android devices.

## How to compile:

  * install the Android SDK and NDK, create a target

  * command line actions:
<pre><code>
  % /opt/android-ndk-r10d/ndk-build
  % ant debug
  % adb install -r bin/ccn-lite-android-debug.apk
</code></pre>

## What works (somehow):

  * init code

  * DEBUGMSGs are written to /sdcard/ccn-lite.log

  * Surprise: the http status console serves the web page! Just connect to your Android device at port 8080


## Wish list:

  * DEBUGMSGs shown on the Android screen, scrollabla

  * GUI with three buttons: '+'/'-' for increase/decrease of debug level, 'd' for heap memory dump

eof
