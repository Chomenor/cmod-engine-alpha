/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2017 Noah Metzger (chomenor@gmail.com)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifdef STEF_CVAR_DEFS
#define CVAR_DEF( name, value, flags ) cvar_t *name;
#include "stef_cvar_defs.h"
#undef CVAR_DEF

/*
==================
Stef_CvarDefsInit
==================
*/
void Stef_CvarDefsInit(void) {
	#define CVAR_DEF( name, value, flags ) name = Cvar_Get( #name, value, flags );
	#include "stef_cvar_defs.h"
	#undef CVAR_DEF
}
#endif
