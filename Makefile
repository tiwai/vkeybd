#
# Makefile for vkeybd
# copyright (c) 1997-2000 by Takashi Iwai
#

VERSION = 0.1.7

#
# installation directory
#
PREFIX = /usr/local
# binary and Tcl script are put there
INSTALL_DIR = $(PREFIX)/bin
# man page
MAN_SUFFIX = 1
MAN_DIR = $(PREFIX)/man/man$(MAN_SUFFIX)

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

#
# Tcl/Tk library -- depends on your distribution
#
TCLLIB = -ltcl
TCLINC =
TKLIB = -L/usr/X11R6/lib -ltk
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


#----------------------------------------------------------------
# dependencies
#----------------------------------------------------------------

VKB_TCLFILE = $(VKBLIB_DIR)/vkeybd.tcl

CFLAGS = -Wall -O -DVKB_TCLFILE=\"$(VKB_TCLFILE)\" \
	-DVKBLIB_DIR=\"$(VKBLIB_DIR)\"\
	-DVERSION_STR=\"$(VERSION)\"\
	$(DEVICES) $(XINC) $(TCLINC) $(TKINC)

TARGETS = vkeybd sftovkb

all: $(TARGETS)

vkeybd: vkb.o vkb_device.o $(DEVOBJS) $(EXTRAOBJS)
	$(CC) -o $@ $^ $(TKLIB) $(TCLLIB) $(XLIB) $(EXTRALIB) -lm

sftovkb: sftovkb.o sffile.o malloc.o fskip.o
	$(CC) -o $@ $^ -lm

install: $(TARGETS) vkeybd.tcl vkeybd.list
	mkdir -p $(INSTALL_DIR)
	install -c -s vkeybd $(INSTALL_DIR)
	install -c -s sftovkb $(INSTALL_DIR)
	rm -f $(INSTALL_DIR)/vkeybd.tcl
	mkdir -p $(VKBLIB_DIR)
	install -c -m 444 vkeybd.tcl $(VKBLIB_DIR)
	install -c -m 444 vkeybd.list $(VKBLIB_DIR)
	install -c -m 444 vkeybd.key $(VKBLIB_DIR)

install-man:
	mkdir -p $(MAN_DIR)
	install -c -m 444 vkeybd.man $(MAN_DIR)/vkeybd.$(MAN_SUFFIX)

install-all: install install-man

clean:
	rm -f *.o $(TARGETS)
