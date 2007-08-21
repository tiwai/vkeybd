/*
 * definition of operation table
 *
 * Virtual Tiny Keyboard
 *
 * Copyright (c) 1999-2000 by Takashi Iwai
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

#include <stdio.h>
#include "vkb.h"

#define numberof(ary)	(sizeof(ary)/sizeof(ary[0]))

#ifdef VKB_USE_ALSA
extern vkb_devinfo_t alsa_devinfo;
#endif

#ifdef VKB_USE_AWE
extern vkb_devinfo_t awe_devinfo;
#endif

#ifdef VKB_USE_MIDI
extern vkb_devinfo_t midi_devinfo;
#endif

vkb_devinfo_t *vkb_device[] = {
#ifdef VKB_USE_ALSA
	&alsa_devinfo,
#endif
#ifdef VKB_USE_AWE
	&awe_devinfo,
#endif
#ifdef VKB_USE_MIDI
	&midi_devinfo,
#endif
};

int vkb_num_devices = numberof(vkb_device);
