# Name: Makefile
# Project: hid-data example
# Author: Christian Starkjohann
# Creation Date: 2008-04-11
# Tabsize: 4
# Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
# This Revision: $Id$

# Please read the definitions below and edit them as appropriate for your
# system:

# Use the following 3 lines on Unix and Mac OS X:
#USBFLAGS=   `libusb-config --cflags`
#USBLIBS=    `libusb-config --libs`
LIBUSB_CONFIG=/opt/local/bin/libusb-legacy-config
#LIBUSB_CONFIG=libusb-config
# Use the following 3 lines on Unix (uncomment the framework on Mac OS X):
USBFLAGS = `$(LIBUSB_CONFIG) --cflags`
USBLIBS = `$(LIBUSB_CONFIG) --libs`
EXE_SUFFIX=

# Use the following 3 lines on Windows and comment out the 3 above:
#USBFLAGS=
#USBLIBS=    -lhid -lusb -lsetupapi
#EXE_SUFFIX= .exe

CC=				gcc
CFLAGS=			-O -Wall $(USBFLAGS)
LIBS=			$(USBLIBS)

OBJ=		blinkmusb-tool.o hiddata.o
PROGRAM=	blinkmusb-tool$(EXE_SUFFIX)

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) -o $(PROGRAM) $(OBJ) $(LIBS)

strip: $(PROGRAM)
	strip $(PROGRAM)

clean:
	rm -f $(OBJ) $(PROGRAM)

.c.o:
	$(CC) $(ARCH_COMPILE) $(CFLAGS) -c $*.c -o $*.o
