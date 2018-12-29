#!/bin/bash

# Versions
MAJOR := 0
MINOR := 9
PATCH := 3
STATE := rc
VERSION := $(MAJOR).$(MINOR)$(STATE)$(PATCH)
# Utilitys
RM := rm -f
CP := cp
MKDIR := mkdir -p
#
DESTDIR ?=
PREFIX ?= /usr
INSTALL_LOCATION=$(DESTDIR)$(PREFIX)
#
CC ?= gcc
CFLAGS := -O2 -DSWP_STR_VERSION=\"$(VERSION)\" -DSWP_MAJOR=$(MAJOR) -DSWP_MINOR=$(MINOR)
CLIBS := -lSDL2 -lfreeimage -lGL

#
SRC = $(wildcard *.c)
OBJS = $(notdir $(subst .c,.o,$(SRC)))
TARGET ?= swp


all : $(TARGET)
	@echo -n "Finished creating $(TARGET). \n"

$(TARGET) : CFLAGS += -O2 -DNDEBUG
$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(CLIBS)

debug : CFLAGS += -g3 -O0
debug : $(OBJS)
	$(CC) $(CLFAGS) $^ -o $(TARGET) $(CLIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

install : $(TARGET)
	@echo -n "Installing wallpaper.\n"
	$(MKDIR) $(INSTALL_LOCATION)/bin
	$(CP) $(TARGET) $(INSTALL_LOCATION)/bin
	$(CP) swp.bc $(INSTALL_LOCATION)/share/bash-completion/completions/swp

distribution :
	$(RM) -r $(TARGET)-$(VERSION)
	$(MKDIR) $(TARGET)-$(VERSION)
	$(CP) *.h *.c Makefile README.md *.1 $(TARGET)-$(VERSION)
	tar cf - $(TARGET)-$(VERSION) | gzip -c > $(TARGET)-$(VERSION).tar.gz
	$(RM) -r $(TARGET)-$(VERSION)

clean :
	$(RM) *.o


.PHONY: all install distribution clean

