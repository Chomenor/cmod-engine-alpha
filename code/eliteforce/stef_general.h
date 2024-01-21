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

// For general cmod stuff; included at the end of qcommon.h

#ifdef STEF_CVAR_DEFS
void Stef_CvarDefsInit(void);

#define CVAR_DEF( name, value, flags ) extern cvar_t *name;
#include "stef_cvar_defs.h"
#undef CVAR_DEF
#endif

#if defined( STEF_LOGGING_SYSTEM ) || defined( STEF_LOGGING_CORE )
#include "stef_logging.h"
#endif

#ifdef STEF_VM_EXTENSIONS
qboolean VMExt_HandleVMSyscall( intptr_t *args, vmIndex_t vm_type, vm_t *vm,
		void *( *VM_ArgPtr )( intptr_t intValue ), intptr_t *retval );
#endif

#ifdef STEF_CLIENT_ALT_SWAP_SUPPORT
void ClientAltSwap_CGameInit( void );
void ClientAltSwap_ModifyCommand( usercmd_t *cmd );
void ClientAltSwap_SetState( qboolean swap );
#endif

#ifdef STEF_SUPPORT_STATUS_SCORES_OVERRIDE
void SV_StatusScoresOverride_Reset( void );
int SV_StatusScoresOverride_AdjustScore( int defaultScore, int clientNum );
#endif

#ifdef STEF_LUA_SUPPORT
#define STEF_LUA_EVENT_POST_FRAME "com_post_frame"
#define STEF_LUA_EVENT_CONSOLE_COMMAND "com_console_cmd"
#define STEF_LUA_EVENT_LOG_PRINT "com_logprint"
#define STEF_LUA_EVENT_LOG_FRAME "com_logframe"

extern qboolean stef_lua_suppress_print_event;
extern qboolean stef_lua_logframes_enabled;
void Stef_Lua_Init( void );
void Stef_Lua_PushString( const char *key, const char *value );
void Stef_Lua_PushInteger( const char *key, int value );
void Stef_Lua_PushBoolean( const char *key, qboolean value );
qboolean Stef_Lua_InitEventCall( const char *name );
qboolean Stef_Lua_RunEventCall( void );
qboolean Stef_Lua_FinishEventCall( void );
qboolean Stef_Lua_SimpleEventCall( const char *name );
#endif

#ifdef STEF_LUA_SERVER
void SV_Lua_Init( void );
#endif

#ifdef STEF_MARIO_MOD_FIX
extern qboolean Stef_MarioModFix_ModActive;
void Stef_MarioModFix_OnVMCreate( vmIndex_t vmType, const fsc_file_t *sourceFile );
#endif
