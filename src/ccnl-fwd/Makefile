LIBNAME = libccnl-fwd.a
INCLUDES = ../ccnl-addons/include ../ccnl-pkt/include ../ccnl-core/include ../ccnl-unix/include ../ccnl-nfn/include


CC ?= gcc

INCLUDE = include
SRC = src
LIB = lib
OBJ = obj

DIRS = $(OBJ) $(LIB)

INCLUDES += $(INCLUDE)
IFLAGS = $(foreach d,$(INCLUDES),-I$d)

ARFLAGS = -static -o
AR = libtool

SOURCES=$(wildcard $(SRC)/*.c)
OBJECTS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

LIBRARY = $(LIB)/$(LIBNAME)

all: ${LIBRARY}

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) $(CCNL_FLAGS) $(IFLAGS) -c $< -o $@

$(LIBRARY): $(OBJECTS) | $(LIB)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(OBJ):
	mkdir -p $(OBJ)

$(LIB):
	mkdir -p $(LIB)

clean: 
	rm -rf $(DIRS)