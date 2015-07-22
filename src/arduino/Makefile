# Makefile

# TODO: rename to ARDUINO_BOARD
BOARD?=atmega328
#BOARD?=uno

ifdef ARDUINO_HOME
    ARDUINO_PARAM:=-d $(ARDUINO_HOME)
else
    ARDUINO_PARAM:=
endif

all:
	ino build -m ${BOARD} -v ${ARDUINO_PARAM}

init:
	ino init ${ARDUINO_PARAM}

upload:
	ino upload -m ${BOARD} ${ARDUINO_PARAM}

serial:
	ino serial

clean:
	ino clean
