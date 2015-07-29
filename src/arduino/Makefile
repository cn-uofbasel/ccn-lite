# Makefile

ARDUINO_PKG?=arduino
ARDUINO_ARCH?=avr
ARDUINO_BOARD?=uno

.PHONY: build verify upload mkdir setboard setccnlhome clean

build: verify
verify: mkdir setboard setccnlhome
	arduino --verify --pref build.path=build -v src/src.ino

upload: mkdir setboard setccnlhome
	arduino --upload --pref build.path=build -v src/src.ino

#serial: Not yet available on Arduino IDE command line

mkdir:
	mkdir -p build

setboard:
	arduino --board "${ARDUINO_PKG}:${ARDUINO_ARCH}:${ARDUINO_BOARD}" --save-prefs

setccnlhome:
	sed "s|\(\s*#define CCN_LITE_C\) \".\+/src/\(ccn-lite-.\+\.c\)\"|\1 \"${CCNL_HOME}/src/\2\"|" src/src.ino > src/src.ino.sed
	mv src/src.ino.sed src/src.ino

clean:
	rm -rf ./build
