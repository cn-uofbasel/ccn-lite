LIBNAME = libccnl-addons.a
CCNLCFLAGS = -DCCNL_UNIX 


CC ?= gcc

INCLUDE = include
SRC = src
LIB = lib
OBJ = obj

DIRS = $(OBJ) $(LIB)

INCLUDES = -I$(INCLUDE)
CFLAGS = -Wall -Werror -std=c99 -g $(INCLUDES) $(CCNLCFLAGS)

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