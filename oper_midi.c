/*
 * operator for MIDI device
 *
 * Virtual Tiny Keyboard
 *
 * Copyright (c) 1997-2000 by Takashi Iwai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define USE_COMPAT_CONST

#include "vkb.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifdef __FreeBSD__
#  include <machine/soundcard.h>
#elif defined(linux)
#  include <linux/soundcard.h>
#endif


/*
 * functions
 */
static int seq_open(Tcl_Interp *ip, void **private_return);
static void seq_close(Tcl_Interp *ip, void *private);
static void note_on(Tcl_Interp *ip, void *private, int note, int vel);
static void note_off(Tcl_Interp *ip, void *private, int note, int vel);
static void program(Tcl_Interp *ip, void *private, int bank, int preset);
static void control(Tcl_Interp *ip, void *private, int type, int val);
static void bender(Tcl_Interp *ip, void *private, int bend);
static void chorus_mode(Tcl_Interp *ip, void *private, int mode);
static void reverb_mode(Tcl_Interp *ip, void *private, int mode);


/*
 * definition of device information
 */

static vkb_oper_t midi_oper = {
	seq_open,
	seq_close,
	program,
	note_on,
	note_off,
	control,
	bender,
	chorus_mode,
	reverb_mode,
};

#define MIDI_DEV	"/dev/midi"

static vkb_optarg_t midi_opts[] = {
	{"mididev", MIDI_DEV, "--mididev device : OSS midi device file"},
	{NULL},
};

vkb_devinfo_t midi_devinfo = {
	"midi",		/* id */
	"MIDI device",	/* name */
	1,		/* delayed open */
	&midi_oper,	/* operators */
	midi_opts,	/* options */
};


/*
 * for OSS interface
 */

static FILE *fp;
static int chan_no;

/*
 */

static int
seq_open(Tcl_Interp *ip, void **private_return)
{
	char *mididev = MIDI_DEV;
	char *var;

	var = Tcl_GetVar2(ip, "optvar", "mididev", TCL_GLOBAL_ONLY);
	if (var != NULL && *var)
		mididev = var;
	if ((fp = fopen(mididev, "w")) == NULL) {
		vkb_error(ip, "can't open midi device file '%s'\n", mididev);
		return 0;
	}

	return 1;
}

static void
seq_close(Tcl_Interp *ip, void *private)
{
	fclose(fp);
}

/*
 */

static void
note_on(Tcl_Interp *ip, void *private, int note, int vel)
{
	putc(MIDI_NOTEON | chan_no, fp);
	putc(note, fp);
	putc(vel, fp);
	fflush(fp);
}

static void
note_off(Tcl_Interp *ip, void *private, int note, int vel)
{
	putc(MIDI_NOTEOFF | chan_no, fp);
	putc(0xf8, fp);
	putc(note, fp);
	putc(vel, fp);
	fflush(fp);
}

static void
program(Tcl_Interp *ip, void *private, int bank, int preset)
{
	vkb_get_int(ip, "channel", &chan_no);
	if (bank == 128)
		vkb_get_int(ip, "drum", &chan_no);
	else {
		putc(MIDI_CTL_CHANGE | chan_no, fp);
		putc(CTL_BANK_SELECT, fp);
		putc(bank, fp);
	}
	putc(MIDI_PGM_CHANGE | chan_no, fp);
	putc(preset, fp);
	/* no dumpbuf here */
}

static void
control(Tcl_Interp *ip, void *private, int type, int val)
{
	putc(MIDI_CTL_CHANGE | chan_no, fp);
	putc(type, fp);
	putc(val, fp);
	fflush(fp);
}

static void
bender(Tcl_Interp *ip, void *private, int bend)
{
	bend += 8192;
	putc(MIDI_PITCH_BEND | chan_no, fp);
	putc(bend & 0x7f, fp);
	putc((bend >> 7) & 0x7f, fp);
	fflush(fp);
}

static void
chorus_mode(Tcl_Interp *ip, void *private, int mode)
{
	static unsigned char sysex[11] = {
		0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x38, 0, 0, 0xf7,
	};
	int i;
	sysex[8] = mode;
	for (i = 0; i < 11; i++)
		putc(sysex[i], fp);
}

static void
reverb_mode(Tcl_Interp *ip, void *private, int mode)
{
	static unsigned char sysex[11] = {
		0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x30, 0, 0, 0xf7,
	};
	int i;
	sysex[8] = mode;
	for (i = 0; i < 11; i++)
		putc(sysex[i], fp);
}

