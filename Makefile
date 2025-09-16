#
# Makefile for vkeybd
# copyright (c) 1997-2000 by Takashi Iwai
#

VERSION = 0.1.18g

#
# installation directory
#
PREFIX = /usr/local
# binary and Tcl script are put there
BIN_DIR = $(PREFIX)/bin
# man page
MAN_SUFFIX = 1
MAN_DIR = $(PREFIX)/share/man

# data directory (/usr/share)
DATA_DIR = $(PREFIX)/share

#
# preset and keyboard file are put here
#
VKBLIB_DIR = $(DATA_DIR)/vkeybd

#
# device selections -- multiple avaialble
# to disable the device, set value 0 (do not comment out!)
#
USE_AWE = 1
USE_MIDI = 1
USE_ALSA = 1
USE_LASH = 0

#
# Tcl/Tk library -- depends on your distribution
#
TCL_VERSION = 8.4
TCLLIB = -ltcl$(TCL_VERSION)
TCLINC =
TKLIB = -ltk$(TCL_VERSION)
TKINC =
XLIB = -L/usr/X11R6/lib -lX11
XINC = -I/usr/X11R6/include
EXTRALIB += -ldl

#----------------------------------------------------------------
# device definitions
#----------------------------------------------------------------

# AWE device
ifeq (1,$(USE_AWE))
DEVICES += -DVKB_USE_AWE
DEVOBJS += oper_awe.o
endif

# MIDI device
ifeq (1,$(USE_MIDI))
DEVICES += -DVKB_USE_MIDI
DEVOBJS += oper_midi.o
endif

# ALSA sequencer
ifeq (1,$(USE_ALSA))
DEVICES += -DVKB_USE_ALSA
DEVOBJS += oper_alsa.o
EXTRALIB += -lasound
endif

#
# LASH stuff
#
ifeq (1,$(USE_LASH))
LASHCFLAGS = $(shell pkg-config --cflags lash-1.0) \
	       $(shell pkg-config --exists lash-1.0 && echo "-DHAVE_LASH" )
LASHLIBS   = $(shell pkg-config --libs lash-1.0)
DEVICES += $(LASHCFLAGS)
EXTRALIB += $(LASHLIBS)
endif

#----------------------------------------------------------------
# dependencies
#----------------------------------------------------------------

VKB_TCLFILE = $(VKBLIB_DIR)/vkeybd.tcl

COPTFLAGS = -Wall -O
CFLAGS = $(COPTFLAGS) -DVKB_TCLFILE=\"$(VKB_TCLFILE)\" \
	-DVKBLIB_DIR=\"$(VKBLIB_DIR)\"\
	-DVERSION_STR=\"$(VERSION)\"\
	$(DEVICES) $(XINC) $(TCLINC) $(TKINC) $(LASHCFLAGS)

TARGETS = vkeybd sftovkb

all: $(TARGETS)

vkeybd: vkb.o vkb_device.o $(DEVOBJS) $(EXTRAOBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(TKLIB) $(TCLLIB) $(XLIB) $(EXTRALIB) -lm

sftovkb: sftovkb.o sffile.o malloc.o fskip.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

install: $(TARGETS) vkeybd.tcl vkeybd.list vkeybdmap*
	mkdir -p $(DESTDIR)$(BIN_DIR)
	install -c -m 755 vkeybd $(DESTDIR)$(BIN_DIR)
	install -c -m 755 sftovkb $(DESTDIR)$(BIN_DIR)
	rm -f $(DESTDIR)$(BIN_DIR)/vkeybd.tcl
	mkdir -p $(DESTDIR)$(VKBLIB_DIR)
	install -c -m 444 vkeybd.tcl $(DESTDIR)$(VKBLIB_DIR)
	install -c -m 444 vkeybd.list $(DESTDIR)$(VKBLIB_DIR)
	install -c -m 444 vkeybdmap* $(DESTDIR)$(VKBLIB_DIR)

install-man:
	mkdir -p $(DESTDIR)$(MAN_DIR)/man$(MAN_SUFFIX)
	install -c -m 444 vkeybd.man $(DESTDIR)$(MAN_DIR)/man$(MAN_SUFFIX)/vkeybd.$(MAN_SUFFIX)

install-desktop:
	mkdir -p $(DESTDIR)$(DATA_DIR)/applications
	install -c -m 644 vkeybd.desktop $(DESTDIR)$(DATA_DIR)/applications
	mkdir -p $(DESTDIR)$(DATA_DIR)/pixmaps
	install -c -m 644 pixmaps/*.png $(DESTDIR)$(DATA_DIR)/pixmaps

install-all: install install-man install-desktop

clean:
	rm -f *.o $(TARGETS)
