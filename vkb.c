/*
 * main routine
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tk.h>

#ifndef Tk_Main
#define Tk_Main(argc, argv, proc) \
    Tk_MainEx(argc, argv, proc, Tcl_CreateInterp())
#endif

/*
 */

#define DEF_INST_CHAN_NO	"0"
#define DEF_DRUM_CHAN_NO	"9"
#define DEF_OCTAVES		"3"

/*
 * prototypes
 */

static int usage(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_on(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_off(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_start_note(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_stop_note(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_control(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_program(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_bender(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_chorus_mode(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int seq_reverb_mode(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]);
static int vkb_app_init(Tcl_Interp *interp);

/*
 * local common variables
 */
static void *private;
static int seq_opened = 0;
static int seq_bank = 0, seq_preset = 0;
static int seq_bend = 0;
static vkb_oper_t *oper;

#ifdef HAVE_LADCCA	
cca_client_t * cca_client = NULL;
#endif
 
/*
 * main routine
 */

int main(int argc, char **argv)
{
	char **nargv;
	int c, nargc;

#ifdef HAVE_LADCCA	
	cca_client = cca_init (cca_extract_args (&argc, &argv),
	                       "vkeybd",
	                       CCA_Use_Alsa);
#endif /* HAVE_LADCCA */

	nargc = argc + 1;
	if ((nargv = (char**)malloc(sizeof(char*) * nargc)) == NULL) {
		fprintf(stderr, "vkeybd: can't malloc\n");
		exit(1);
	}
	nargv[0] = "-f";
	nargv[1] = VKB_TCLFILE;
	for (c = 1; c < argc; c++)
		nargv[c+1] = argv[c];

	/* copy the default device operator */
	if (vkb_num_devices <= 0) {
		fprintf(stderr, "vkeybd: no device is defined\n");
		exit(1);
	}

	/* call Tk main routine */
	Tk_Main(nargc, nargv, vkb_app_init);

	return 0;
}


/*
 * print usage
 */
static int
usage(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	int i;

	fprintf(stderr, "vkeybd -- virtual keyboard\n");
	fprintf(stderr, "  version %s\n", VERSION_STR);
	fprintf(stderr, "Copyright (c) 1997-2000 Takashi Iwai\n");
	fprintf(stderr, "usage: vkeybd [-options]\n");
	fprintf(stderr, "* general options:\n");
	fprintf(stderr, "  --device mode  : specify output device (default = %s)\n", vkb_device[0]->name);
	fprintf(stderr, "  --keymap file  : use own keymap file\n");
	fprintf(stderr, "  --preset file  : use preset list file\n");
	fprintf(stderr, "  --channel number : specify instrument channel (0-15) (default = %s)\n", DEF_INST_CHAN_NO);
	fprintf(stderr, "  --drum number : specify drum channel (0-15) (default = %s)\n", DEF_DRUM_CHAN_NO);
	fprintf(stderr, "  --octave number : specify octaves of displaying keys (default = %s)\n", DEF_OCTAVES);
	fprintf(stderr, "* output devices:\n");

	for (i = 0; i < vkb_num_devices; i++) {
		vkb_optarg_t *p;
		fprintf(stderr, "  %s = %s\n", vkb_device[i]->name, vkb_device[i]->desc);
		if (! (p = vkb_device[i]->opts))
			continue;
		for (; p->name; p++)
			fprintf(stderr, "    %s (default = %s)\n",
				p->desc, p->defval);
	}
	return TCL_OK;
}



/*
 * Tcl commands to sequencer device
 */

/* open device */
static int
seq_on(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	int i;
	const char *var;
	
	if (seq_opened)
		return TCL_OK;
	if (! (var = Tcl_GetVar2(interp, "optvar", "device", TCL_GLOBAL_ONLY))) {
		vkb_error(interp, "no output device defined");
		return TCL_ERROR;
	}
	oper = NULL;
	for (i = 0; i < vkb_num_devices && vkb_device[i]; i++) {
		if (strcmp(vkb_device[i]->name, var) == 0) {
			oper = vkb_device[i]->oper;
			break;
		}
	}
	if (! oper) {
		vkb_error(interp, "no output device defined");
		return TCL_ERROR;
	}
	
	if (argc > 1 && vkb_device[i]->delayed_open)
		return TCL_OK;

	if (oper->open(interp, &private)) {
		seq_opened = 1;
		Tcl_SetVar(interp, "seqswitch", "1", TCL_GLOBAL_ONLY);
		if (oper->program)
			oper->program(interp, private, seq_bank, seq_preset);
		if (oper->bender)
			oper->bender(interp, private, seq_bend);
	}

	return TCL_OK;
}

/* close device */
static int
seq_off(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	if (seq_opened) {
		oper->close(interp, private);
		seq_opened = 0;
		Tcl_SetVar(interp, "seqswitch", "0", TCL_GLOBAL_ONLY);
	}
	return TCL_OK;
}

static int
seq_start_note(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	int note, vel;
	if (argc < 3)
		return TCL_ERROR;
	if (! seq_opened)
		return TCL_OK;
	note = atoi(argv[1]);
	vel = atoi(argv[2]);
	if (oper->noteon)
		oper->noteon(interp, private, note, vel);
	return TCL_OK;
}

static int
seq_stop_note(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	int note, vel;
	if (argc < 3)
		return TCL_ERROR;
	if (! seq_opened)
		return TCL_OK;
	note = atoi(argv[1]);
	vel = atoi(argv[2]);
	if (oper->noteoff)
		oper->noteoff(interp, private, note, vel);
	return TCL_OK;
}

static int
seq_control(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	int type, val;
	if (argc < 3)
		return TCL_ERROR;
	if (! seq_opened)
		return TCL_OK;
	type = atoi(argv[1]);
	val = atoi(argv[2]);
	if (oper->control)
		oper->control(interp, private, type, val);
	return TCL_OK;
}

static int
seq_program(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	if (argc < 3)
		return TCL_ERROR;
	seq_bank = atoi(argv[1]);
	seq_preset = atoi(argv[2]);
	if (! seq_opened)
		return TCL_OK;
	if (oper->program)
		oper->program(interp, private, seq_bank, seq_preset);
	return TCL_OK;
}

static int
seq_bender(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	if (argc < 2)
		return TCL_ERROR;
	seq_bend = atoi(argv[1]);
	if (! seq_opened)
		return TCL_OK;
	if (oper->bender)
		oper->bender(interp, private, seq_bend);
	return TCL_OK;
}

static int
seq_chorus_mode(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	if (argc < 2)
		return TCL_ERROR;
	if (seq_on(clientData, interp, argc, argv) != TCL_OK)
		return TCL_ERROR;
	if (oper->chorus_mode)
		oper->chorus_mode(interp, private, atoi(argv[1]));
	return TCL_OK;
}

static int
seq_reverb_mode(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
	if (argc < 2)
		return TCL_ERROR;
	if (seq_on(clientData, interp, argc, argv) != TCL_OK)
		return TCL_ERROR;
	if (oper->reverb_mode)
		oper->reverb_mode(interp, private, atoi(argv[1]));
	return TCL_OK;
}


/*
 * Misc. functions
 */

void
vkb_error(Tcl_Interp *ip, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fputs("ERROR: ", stderr);
	vfprintf(stderr, fmt, ap);
	putc('\n', stderr);
	va_end(ap);
}

int
vkb_get_int(Tcl_Interp *ip, char *opt, int *ret)
{
	const char *var;
	if ((var = Tcl_GetVar2(ip, "optvar", opt, TCL_GLOBAL_ONLY)) != NULL && *var) {
		*ret = atoi(var);
		return 1;
	}
	return 0;
}

/*
 * Initialize Tcl/Tk components
 */
static int
vkb_app_init(Tcl_Interp *interp)
{
	int i;

	if (Tcl_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}
	if (Tk_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}

	Tcl_CreateCommand(interp, "usage", usage,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqOn", seq_on,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqOff", seq_off,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqStartNote", seq_start_note,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqStopNote", seq_stop_note,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqControl", seq_control,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqProgram", seq_program,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqBender", seq_bender,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqChorusMode", seq_chorus_mode,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "SeqReverbMode", seq_reverb_mode,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

	for (i = 0; i < vkb_num_devices; i++) {
		vkb_optarg_t *p;
		if (! (p = vkb_device[i]->opts))
			continue;
		for (; p->name; p++) {
			Tcl_SetVar2(interp, "optvar", p->name, p->defval, TCL_GLOBAL_ONLY);
		}
	}

	Tcl_SetVar2(interp, "optvar", "device", vkb_device[0]->name, TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "optvar", "libpath", VKBLIB_DIR, TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "optvar", "channel", DEF_INST_CHAN_NO, TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "optvar", "drum", DEF_DRUM_CHAN_NO, TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "optvar", "octave", DEF_OCTAVES, TCL_GLOBAL_ONLY);

	return TCL_OK;
}

