/*
 * operator for ALSA sequencer
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
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/asoundlib.h>


/*
 * functions
 */
static int seq_open(Tcl_Interp *ip, void **private_return);
static void seq_close(void *private);
static void note_on(void *private, int note, int vel);
static void note_off(void *private, int note, int vel);
static void control(void *private, int type, int val);
static void program(void *private, int bank, int type);
static void bender(void *private, int bend);
static void send_event(int do_flush);


/*
 * definition of device information
 */

static vkb_oper_t alsa_oper = {
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

static vkb_optarg_t alsa_opts[] = {
	{"addr", "subscriber", "--addr client:port or 'subscriber' : ALSA sequencer destination"},
	{NULL},
};

vkb_devinfo_t alsa_devinfo = {
	"alsa",		/* device id */
	"ALSA sequencer",	/* name */
	0,		/* delayed open */
	&alsa_oper,	/* operators */
	alsa_opts,	/* command line options */
};

/*
 */

static snd_seq_t *seq_handle = NULL;
static int my_client, my_port;
static int seq_client, seq_port;
static int inst_chan_no, drum_chan_no, chan_no;

/*
 * parse address string
 */

#define ADDR_DELIM	".:"

static int parse_addr(char *arg, int *client, int *port)
{
	char *p;

	if (isdigit(*arg)) {
		if ((p = strpbrk(arg, ADDR_DELIM)) == NULL)
			return -1;
		*client = atoi(arg);
		*port = atoi(p + 1);
	} else {
		if (*arg == 's' || *arg == 'S') {
			*client = SND_SEQ_ADDRESS_SUBSCRIBERS;
			*port = 0;
		} else
			return -1;
	}
	return 0;
}


/*
 * open device
 */

static int
seq_open(Tcl_Interp *ip, void **private_return)
{
	char *var;
	unsigned int caps;

	/* copy from Tcl variables */
	if ((var = Tcl_GetVar2(ip, "optvar", "addr", TCL_GLOBAL_ONLY)) != NULL) {
		if (parse_addr(var, &seq_client, &seq_port) < 0) {
			vkb_error(ip, "invalid argument --addr %s\n", var);
			return 0;
		}
	}
	
	if (snd_seq_open(&seq_handle, SND_SEQ_OPEN) < 0) {
		vkb_error(ip, "can't open sequencer device");
		return 0;
	}

	/* get my client id */
	my_client = snd_seq_client_id(seq_handle);

	/* set client info */
	snd_seq_set_client_name(seq_handle, "Virtual Keyboard");
	snd_seq_set_client_group(seq_handle, "input");

	/* create port */
	caps = SND_SEQ_PORT_CAP_READ;
	if (seq_client == SND_SEQ_ADDRESS_SUBSCRIBERS)
		caps |= SND_SEQ_PORT_CAP_SUBS_READ;
	my_port = snd_seq_create_simple_port(seq_handle, "Keyboard", caps,
					     SND_SEQ_PORT_TYPE_APPLICATION);
	if (my_port < 0) {
		vkb_error(ip, "can't create port\n");
		snd_seq_close(seq_handle);
		return 0;
	}

	if (seq_client != SND_SEQ_ADDRESS_SUBSCRIBERS) {
		/* subscribe to MIDI port */
		if (snd_seq_connect_to(seq_handle, my_port, seq_client, seq_port) < 0) {
			vkb_error(ip, "can't subscribe to MIDI port (%d:%d)\n", seq_client, seq_port);
			snd_seq_close(seq_handle);
			return 0;
		}
	}

	if ((var = Tcl_GetVar2(ip, "optvar", "channel", TCL_GLOBAL_ONLY)) != NULL)
		inst_chan_no = atoi(var);
	if ((var = Tcl_GetVar2(ip, "optvar", "drum", TCL_GLOBAL_ONLY)) != NULL)
		drum_chan_no = atoi(var);
	chan_no = inst_chan_no;

	{
		char tmp[128];
		sprintf(tmp, "wm title . \"Virtual Keyboard ver.1.7 \\[%d:%d\\]\"", my_client, my_port);
		Tcl_Eval(ip, tmp);
	}

	return 1;
}


static void
seq_close(void *private)
{
	snd_seq_close(seq_handle);
}


/*
 */

static snd_seq_event_t ev;

static void
send_event(int do_flush)
{
	snd_seq_ev_set_direct(&ev);
	snd_seq_ev_set_source(&ev, my_port);
	snd_seq_ev_set_dest(&ev, seq_client, seq_port);

	snd_seq_event_output(seq_handle, &ev);
	if (do_flush)
		snd_seq_flush_output(seq_handle);
}

static void
note_on(void *private, int note, int vel)
{
	snd_seq_ev_set_noteon(&ev, chan_no, note, vel);
	send_event(1);
}

static void
note_off(void *private, int note, int vel)
{
	snd_seq_ev_set_noteoff(&ev, chan_no, note, vel);
	send_event(1);
}

static void
control(void *private, int type, int val)
{
	snd_seq_ev_set_controller(&ev, chan_no, type, val);
	send_event(1);
}

static void
program(void *private, int bank, int preset)
{
	if (bank == 128)
		chan_no = drum_chan_no;
	else {
		chan_no = inst_chan_no;
		snd_seq_ev_set_controller(&ev, 0, 0, bank);
		send_event(0);
	}

	snd_seq_ev_set_pgmchange(&ev, chan_no, preset);
	send_event(0);
}

static void
bender(void *private, int bend)
{
	snd_seq_ev_set_pitchbend(&ev, chan_no, bend);
	send_event(1);
}
