/*
 * operator for AWE32/64 on OSS
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

#define USE_NO_CONST

#include "vkb.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef __FreeBSD__
#  include <machine/soundcard.h>
#  include <awe_voice.h>
#elif defined(linux)
#  include <linux/soundcard.h>
#  include <linux/awe_voice.h>
#endif

/*
 * functions
 */
static int seq_open(Tcl_Interp *ip, void **private_return);
static void seq_close(Tcl_Interp *ip, void *private);
static void program(Tcl_Interp *ip, void *private, int bank, int preset);
static void note_on(Tcl_Interp *ip, void *private, int note, int vel);
static void note_off(Tcl_Interp *ip, void *private, int note, int vel);
static void control(Tcl_Interp *ip, void *private, int type, int val);
static void bender(Tcl_Interp *ip, void *private, int bend);
static void chorus_mode(Tcl_Interp *ip, void *private, int mode);
static void reverb_mode(Tcl_Interp *ip, void *private, int mode);

/*
 * OSS sequencer stuff
 */
int seqfd;
SEQ_DEFINEBUF(128);

void
seqbuf_dump(void)
{
	if (_seqbufptr) {
		if (write(seqfd, _seqbuf, _seqbufptr) == -1) {
			perror("write sequencer");
			exit(1);
		}
	}
	_seqbufptr = 0;
}

/*
 * definition of device information
 */

static vkb_oper_t awe_oper = {
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

#define SEQUENCER_DEV	"/dev/sequencer"

static vkb_optarg_t awe_opts[] = {
	{"seqdev", SEQUENCER_DEV, "--seqdev device : OSS sequencer device file"},
	{"seqidx", "-1", "--seqidx device : speicfy synth index"},
	{NULL},
};

vkb_devinfo_t awe_devinfo = {
	"awe",		/* id */
	"OSS AWE32/64 Synth",	/* name */
	1,		/* delayed open */
	&awe_oper,	/* operators */
	awe_opts,	/* options */
};


/*
 * for OSS interface
 */

SEQ_USE_EXTBUF();
extern int seqfd;
static int awe_dev;

/*
 * open device
 */
static int
seq_open(Tcl_Interp *ip, void **private_return)
{
	const char *var;
	const char *seqdev = SEQUENCER_DEV;
	int nrsynths;
	struct synth_info card_info;
	int i;

	if ((var = Tcl_GetVar2(ip, "optvar", "seqdev", TCL_GLOBAL_ONLY)) != NULL && *var)
		seqdev = var;

	if ((seqfd = open(seqdev, O_WRONLY, 0)) < 0) {
		vkb_error(ip, "can't open sequencer device '%s'", seqdev);
		return 0;
	}

	if (ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &nrsynths) == -1) {
		vkb_error(ip, "there is no soundcard installed");
		close(seqfd);
		return 0;
	}

	awe_dev = -1;
	vkb_get_int(ip, "seqidx", &awe_dev);
	if (awe_dev >= 0) {
		card_info.device = awe_dev;
		if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &card_info) == -1) {
			vkb_error(ip, "cannot get info on soundcard %d", awe_dev);
			close(seqfd);
			return 0;
		}
		if (card_info.synth_type != SYNTH_TYPE_SAMPLE
		    || card_info.synth_subtype != SAMPLE_TYPE_AWE32) {
			vkb_error(ip, "invalid synth device number %d\n", awe_dev);
			close(seqfd);
			return 0;
		}
	} else {
		/* auto-probe */
		for (i = 0; i < nrsynths; i++) {
			card_info.device = i;
			if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &card_info) == -1) {
				vkb_error(ip, "cannot get info on soundcard %d", i);
				close(seqfd);
				return 0;
			}
			if (card_info.synth_type == SYNTH_TYPE_SAMPLE
			    && card_info.synth_subtype == SAMPLE_TYPE_AWE32) {
				awe_dev = i;
				break;
			}
		}
		if (awe_dev < 0) {
			vkb_error(ip, "No AWE synth device is found\n");
			close(seqfd);
			return 0;
		}
	}

	/* use MIDI channel mode */
	AWE_SET_CHANNEL_MODE(awe_dev, AWE_PLAY_MULTI);
	/* toggle drum flag if bank #128 is received */
	AWE_MISC_MODE(awe_dev, AWE_MD_TOGGLE_DRUM_BANK, 1);
	
	return 1;
}

/*
 * close device
 */
static void
seq_close(Tcl_Interp *ip, void *private)
{
	close(seqfd);
}

static void
program(Tcl_Interp *ip, void *private, int bank, int preset)
{
	SEQ_CONTROL(awe_dev, 0, CTL_BANK_SELECT, bank);
	SEQ_SET_PATCH(awe_dev, 0, preset);
	/* no dumpbuf */
}

static void
note_on(Tcl_Interp *ip, void *private, int note, int vel)
{
	SEQ_START_NOTE(awe_dev, 0, note, vel);
	SEQ_DUMPBUF();
}

static void
note_off(Tcl_Interp *ip, void *private, int note, int vel)
{
	SEQ_STOP_NOTE(awe_dev, 0, note, vel);
	SEQ_DUMPBUF();
}

static void
control(Tcl_Interp *ip, void *private, int type, int val)
{
	SEQ_CONTROL(awe_dev, 0, type, val);
	SEQ_DUMPBUF();
}

static void
bender(Tcl_Interp *ip, void *private, int bend)
{
	SEQ_BENDER(awe_dev, 0, bend + 8192);
	SEQ_DUMPBUF();
}

static void
chorus_mode(Tcl_Interp *ip, void *private, int mode)
{
	AWE_CHORUS_MODE(awe_dev, mode);
	SEQ_DUMPBUF();
}

static void
reverb_mode(Tcl_Interp *ip, void *private, int mode)
{
	AWE_REVERB_MODE(awe_dev, mode);
	SEQ_DUMPBUF();
}

