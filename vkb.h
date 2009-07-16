/*
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

#ifndef VKB_H_DEF
#define VKB_H_DEF

#include <tcl.h>

#ifdef HAVE_LASH
#include <lash/lash.h>
#endif

#ifndef VKB_TCLFILE
#define VKB_TCLFILE "/usr/local/bin/vkeybd.tcl"
#endif

/*
 * device operator
 */
typedef struct vkb_oper_t {
	int (*open)(Tcl_Interp *ip, void **private_return);
	void (*close)(Tcl_Interp *ip, void *private);
	void (*program)(Tcl_Interp *ip, void *private, int bank, int prg); /* bank=128: drum */
	void (*noteon)(Tcl_Interp *ip, void *private, int note, int vel);
	void (*noteoff)(Tcl_Interp *ip, void *private, int note, int vel);
	void (*control)(Tcl_Interp *ip, void *private, int type, int val);
	void (*bender)(Tcl_Interp *ip, void *private, int bend); /* -8192 to 8192 */
	void (*chorus_mode)(Tcl_Interp *ip, void *private, int mode);
	void (*reverb_mode)(Tcl_Interp *ip, void *private, int mode);
} vkb_oper_t;

/*
 * Tcl global option variables
 */
typedef struct vkb_optarg_t {
	char *name;
	char *defval;
	char *desc;
} vkb_optarg_t;

/*
 * device information
 */
typedef struct vkb_devinfo_t {
	char *name;
	char *desc;
	int delayed_open;
	vkb_oper_t *oper;
	vkb_optarg_t *opts;
} vkb_devinfo_t;

extern int vkb_num_devices;
extern vkb_devinfo_t *vkb_device[];

void vkb_error(Tcl_Interp *ip, char *fmt, ...);
int vkb_get_int(Tcl_Interp *ip, char *opt, int *ret);

#ifdef HAVE_LASH
extern lash_args_t *lash_args;
#endif

#endif
