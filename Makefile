#
# Makefile for vkeybd
# copyright (c) 1997-2000 by Takashi Iwai
#

VERSION = 0.1.14

#
# installation directory
#
PREFIX = /usr/local
# binary and Tcl script are put there
BIN_DIR = $(PREFIX)/bin
# man page
MAN_SUFFIX = 1
MAN_DIR = $(PREFIX)/share/man

#
# preset and keyboard file are put here
#
VKBLIB_DIR = $(PREFIX)/share/vkeybd

#
# device selections -- multiple avaialble
# to disable the device, set value 0 (do not comment out!)
#
USE_AWE = 1
USE_MIDI = 1
USE_ALSA = 1
USE_LADCCA = 0

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
# LADCCA stuff
#
ifeq (1,$(USE_LADCCA))
LADCCACFLAGS = $(shell pkg-config --cflags ladcca-1.0) \
	       $(shell pkg-config --exists ladcca-1.0 && echo "-DHAVE_LADCCA" )
LADCCALIBS   = $(shell pkg-config --libs ladcca-1.0)
DEVICES += $(LADCCACFLAGS)
EXTRALIB += $(LADCCALIBS)
endif

#----------------------------------------------------------------
# dependencies
#----------------------------------------------------------------

VKB_TCLFILE = $(VKBLIB_DIR)/vkeybd.tcl

CFLAGS = -Wall -O -DVKB_TCLFILE=\"$(VKB_TCLFILE)\" \
	-DVKBLIB_DIR=\"$(VKBLIB_DIR)\"\
	-DVERSION_STR=\"$(VERSION)\"\
	$(DEVICES) $(XINC) $(TCLINC) $(TKINC) $(LADCCACFLAGS)

TARGETS = vkeybd sftovkb

all: $(TARGETS)

vkeybd: vkb.o vkb_device.o $(DEVOBJS) $(EXTRAOBJS)
	$(CC) -o $@ $^ $(TKLIB) $(TCLLIB) $(XLIB) $(EXTRALIB) -lm

sftovkb: sftovkb.o sffile.o malloc.o fskip.o
	$(CC) -o $@ $^ -lm

install: $(TARGETS) vkeybd.tcl vkeybd.list
	mkdir -p $(DESTDIR)$(BIN_DIR)
	install -c -s vkeybd $(DESTDIR)$(BIN_DIR)
	install -c -s sftovkb $(DESTDIR)$(BIN_DIR)
	rm -f $(DESTDIR)$(BIN_DIR)/vkeybd.tcl
	mkdir -p $(DESTDIR)$(VKBLIB_DIR)
	install -c -m 444 vkeybd.tcl $(DESTDIR)$(VKBLIB_DIR)
	install -c -m 444 vkeybd.list $(DESTDIR)$(VKBLIB_DIR)

install-man:
	mkdir -p $(DESTDIR)$(MAN_DIR)/man$(MAN_SUFFIX)
	install -c -m 444 vkeybd.man $(DESTDIR)$(MAN_DIR)/man$(MAN_SUFFIX)/vkeybd.$(MAN_SUFFIX)

install-all: install install-man

clean:
	rm -f *.o $(TARGETS)
