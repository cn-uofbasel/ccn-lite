LIBNAME = libccnl-nfn.a
#SUITEFLAGS = -DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB \
#			 -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC
#USEFLAGS = -DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING -DNEEDS_PREFIX_MATCHING
NFNFLAGS = -DUSE_NFN -DNEEDS_PACKET_CRAFTING
CCNLCFLAGS = ${USEFLAGS} ${SUITEFLAGS} ${NFNFLAGS}
INCLUDES = ../ccnl-core/include ../ccnl-unix/include ../ccnl-addons/include


CC ?= gcc

INCLUDE = include
SRC = src
LIB = lib
OBJ = obj

DIRS = $(OBJ) $(LIB)

INCLUDES += $(INCLUDE)
IFLAGS = $(foreach d,$(INCLUDES),-I$d)
CFLAGS = -Wall -Werror -std=c99 -g $(IFLAGS) $(CCNLCFLAGS)

ARFLAGS = -static -o
AR = libtool

SOURCES=$(wildcard $(SRC)/*.c)
OBJECTS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

LIBRARY = $(LIB)/$(LIBNAME)

all: ${LIBRARY}

#$(OBJECTS): $(SOURCES) | $(OBJ)
#	 $(CC) $(CFLAGS) -c -o $@ $<
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBRARY): $(OBJECTS) | $(LIB)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(OBJ):
	mkdir -p $(OBJ)

$(LIB):
	mkdir -p $(LIB)

clean:
	rm -rf $(DIRS)