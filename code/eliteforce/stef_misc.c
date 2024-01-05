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

#if defined ( STEF_MARIO_MOD_FIX )
#include "../filesystem/fslocal.h"
#else
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#endif

#ifdef STEF_CLIENT_ALT_SWAP_SUPPORT
#define EF_BUTTON_ATTACK 1
#define EF_BUTTON_ALT_ATTACK 32

static qboolean clientAltSwapActive;

/*
==================
ClientAltSwap_CGameInit
==================
*/
void ClientAltSwap_CGameInit( void ) {
	// Don't leave settings from a previous mod
	clientAltSwapActive = qfalse;
}

/*
==================
ClientAltSwap_ModifyCommand
==================
*/
void ClientAltSwap_ModifyCommand( usercmd_t *cmd ) {
	if ( clientAltSwapActive ) {
		if ( cmd->buttons & EF_BUTTON_ALT_ATTACK ) {
			cmd->buttons &= ~EF_BUTTON_ALT_ATTACK;
			cmd->buttons |= EF_BUTTON_ATTACK;
		} else if ( cmd->buttons & EF_BUTTON_ATTACK ) {
			cmd->buttons |= ( EF_BUTTON_ATTACK | EF_BUTTON_ALT_ATTACK );
		}
	}
}

/*
==================
ClientAltSwap_SetState
==================
*/
void ClientAltSwap_SetState( qboolean swap ) {
	clientAltSwapActive = swap;
}
#endif

#ifdef STEF_MARIO_MOD_FIX
qboolean Stef_MarioModFix_ModActive = qfalse;

/*
==================
Stef_MarioModFix_OnVMCreate
==================
*/
void Stef_MarioModFix_OnVMCreate( vmIndex_t vmType, const fsc_file_t *sourceFile ) {
	if ( vmType == VM_GAME ) {
		Stef_MarioModFix_ModActive = qfalse;

		if ( sourceFile && sourceFile->filesize == 672876 ) {
			unsigned int size = 0;
			char *data = FS_ReadData( sourceFile, NULL, &size, __func__ );
			if ( data ) {
				unsigned int hash = Com_BlockChecksum( data, size );
				FS_FreeData( data );
				if ( hash == 2630475216u ) {
					Com_Printf( "Enabling engine workaround for Mario Mod connection issues.\n" );
					Stef_MarioModFix_ModActive = qtrue;
				}
			}
		}
	}
}
#endif
