#!/bin/bash

RM := rm -f
CP := cp
MKDIR := mkdir -p
DESTDIR ?=
PREFIX ?= /usr
INSTALL_LOCATION=$(DESTDIR)$(PREFIX)
CC ?= gcc
CFLAGS := -O2 -DGLES2=1
CLIBS := -lSDL2 -lfreeimage -lGL

#
SRC = $(wildcard *.c)
OBJS = $(notdir $(subst .c,.o,$(SRC)))
TARGET ?= swp
VERSION := 0.9.0


vpath %.c .
vpath %.o .
VPATH := .



all : $(TARGET)
	@echo "Making $(TARGET) \n"

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(CLIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@


install : all
	@echo "Installing wallpaper.\n"
	$(MKDIR) $(INSTALL_LOCATION)/bin
	$(CP) $(TARGET) $(INSTALL_LOCATION)/bin


distribution :
	$(RM) -r $(TARGET)-$(VERSION)
	$(MKDIR) $(TARGET)-$(VERSION)
	$(CP) *.h *.c Makefile README.md *.1 $(TARGET)-$(VERSION)
	tar cf - $(TARGET)-$(VERSION) | gzip -c > $(TARGET)-$(VERSION).tar.gz
	$(RM) -r $(TARGET)-$(VERSION)

clean :
	$(RM) *.o
