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
static void seq_close(void *private);
static void note_on(void *private, int note, int vel);
static void note_off(void *private, int note, int vel);
static void program(void *private, int bank, int preset);
static void control(void *private, int type, int val);
static void bender(void *private, int bend);


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
	NULL, /* chorus_mode */
	NULL, /* reverb_mode */
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
static int inst_chan_no, drum_chan_no, chan_no;

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
	if ((var = Tcl_GetVar2(ip, "optvar", "channel", TCL_GLOBAL_ONLY)) != NULL)
		inst_chan_no = atoi(var);
	if ((var = Tcl_GetVar2(ip, "optvar", "drum", TCL_GLOBAL_ONLY)) != NULL)
		drum_chan_no = atoi(var);
	chan_no = inst_chan_no;

	return 1;
}

static void
seq_close(void *private)
{
	fclose(fp);
}

/*
 */

static void
note_on(void *private, int note, int vel)
{
	putc(MIDI_NOTEON | chan_no, fp);
	putc(note, fp);
	putc(vel, fp);
	fflush(fp);
}

static void
note_off(void *private, int note, int vel)
{
	putc(MIDI_NOTEOFF | chan_no, fp);
	putc(0xf8, fp);
	putc(note, fp);
	putc(vel, fp);
	fflush(fp);
}

static void
program(void *private, int bank, int preset)
{
	if (bank == 128)
		chan_no = drum_chan_no;
	else {
		chan_no = inst_chan_no;
		putc(MIDI_CTL_CHANGE | chan_no, fp);
		putc(CTL_BANK_SELECT, fp);
		putc(bank, fp);
	}
	putc(MIDI_PGM_CHANGE | chan_no, fp);
	putc(preset, fp);
	// no dumpbuf here
}

static void
control(void *private, int type, int val)
{
	putc(MIDI_CTL_CHANGE | chan_no, fp);
	putc(type, fp);
	putc(val, fp);
	fflush(fp);
}

static void
bender(void *private, int bend)
{
	bend += 8192;
	putc(MIDI_PITCH_BEND | chan_no, fp);
	putc(bend & 0x7f, fp);
	putc((bend >> 7) & 0x7f, fp);
	fflush(fp);
}

