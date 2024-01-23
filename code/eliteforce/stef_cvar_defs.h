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

#ifdef STEF_SERVER_ALT_SWAP_SUPPORT
// Enable handler for compatibility with client alt fire swap features.
CVAR_DEF( sv_altSwapSupport, "1", CVAR_LATCH )
#endif

#ifdef STEF_MIN_SNAPS
CVAR_DEF( sv_minSnaps, "50", CVAR_ARCHIVE );
#endif

#ifdef STEF_MODEL_NAME_LENGTH_LIMIT
// Maximum allowed length of player model string.
CVAR_DEF( sv_maxModelLength, "48", 0 )
#endif

#ifdef STEF_SV_PINGFIX
CVAR_DEF( sv_pingFix, "1", 0 )
#endif
