# Makefile

ARDUINO_PKG?=arduino
ARDUINO_ARCH?=avr
ARDUINO_BOARD?=uno

.PHONY: build verify upload mkdir setboard clean

build: verify
verify: mkdir setboard
	arduino --verify --pref build.path=build -v src/src.ino

upload: mkdir setboard
	arduino --upload --pref build.path=build -v src/src.ino

#serial: Not yet available on Arduino IDE command line

mkdir:
	mkdir -p build

setboard:
	arduino --board "${ARDUINO_PKG}:${ARDUINO_ARCH}:${ARDUINO_BOARD}" --save-prefs

clean:
	rm -rf ./build
