PROOT  = $(shell pwd)
SRCDIR = $(PROOT)/src
BINDIR = $(PROOT)/bin
OBJDIR = $(PROOT)/obj
LIBDIR = $(PROOT)/lib
EXTDIR = $(PROOT)/external

CC = gcc
CXX = g++
LD = ld
AR = ar

OPT=-O0
CFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c99 -g -pg $(OPT)
LIBS   = -L$(LIBDIR) -lmem -lbtn -lncurses
INCLUDE = -I$(PROOT)/inc -I$(EXTDIR)/libmem/inc -I$(EXTDIR)/libbtn/inc

LIB_DEPEND = $(LIBDIR)/libmem.a $(LIBDIR)/libbtn.a

MODULE = dkstr

SRCMAIN = $(SRCDIR)/$(MODULE).c
SOURCES += $(shell find $(SRCDIR) -name '*.c' ! -wholename $(SRCMAIN))
BIN = $(MODULE)

OBJECTS  = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/$(MODULE)/%.o)
OBJMAIN  = $(SRCMAIN:$(SRCDIR)/%.c=$(OBJDIR)/$(MODULE)/%.o)

all: $(LIB_DEPEND) $(BINDIR)/$(BIN)

$(LIBDIR)/libmem.a:
	mkdir -p $(LIBDIR)
	+$(MAKE) -C external/libmem
	cp external/libmem/lib/libmem.a $(LIBDIR)/

$(LIBDIR)/libbtn.a:
	mkdir -p $(LIBDIR)
	+$(MAKE) -C external/libbtn
	cp external/libbtn/lib/libbtn.a $(LIBDIR)/

$(OBJDIR)/$(MODULE)/%.o:$(SRCDIR)/%.cpp
	mkdir -p $(@D)	# generate the directory
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $(@)

$(OBJDIR)/$(MODULE)/%.o:$(SRCDIR)/%.c
	mkdir -p $(@D)	# generate the directory
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $(@)

$(BINDIR)/$(BIN): $(OBJECTS) $(OBJMAIN)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BINDIR)/$(BIN) $(OBJECTS) $(OBJMAIN) $(LIBS)
	@echo "Program built"

test: $(BINDIR)/$(BIN)
	@$(BINDIR)/$(BIN)

debug: $(BINDIR)/$(BIN)
	gdb $(BINDIR)/$(BIN)

clean:
	@#rm -rf cscope
	rm -rf $(BINDIR)
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)
	+$(MAKE) clean -C external/libmem

.PHONY: all clean test debug
