BINNAME = ccn-lite-relay

LIBS = ../ccnl-addons/lib/libccnl-addons.a \
	   ../ccnl-pkt/lib/libccnl-pkt.a \
	   ../ccnl-fwd/lib/libccnl-fwd.a \
	   ../ccnl-unix/lib/libccnl-unix.a \
	   ../ccnl-core/lib/libccnl-core.a

EXTLIBS = -lssl -lcrypto

INCLUDES = ../ccnl-addons/include \
		   ../ccnl-pkt/include \
		   ../ccnl-fwd/include \
		   ../ccnl-unix/include \
		   ../ccnl-core/include


CC ?= gcc

SRC = .
BIN = bin
OBJ = obj

DIRS = $(OBJ) $(BIN)

IFLAGS = $(foreach d,$(INCLUDES),-I$d)

SOURCES=$(wildcard $(SRC)/*.c)
OBJECTS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

BINARY = $(BIN)/$(BINNAME)

all: ${BINARY}

$(BINARY): $(SOURCES) | $(BIN)
	$(CC) -o $(BINARY) $(CFLAGS) $(CCNL_FLAGS) $(IFLAGS) $(SOURCES) $(LIBS) $(EXTLIBS)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN):
	mkdir -p $(BIN)

clean:
	rm -rf $(DIRS)
