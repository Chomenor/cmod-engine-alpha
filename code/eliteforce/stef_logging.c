/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2020 Noah Metzger (chomenor@gmail.com)

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

#if defined( STEF_LOGGING_SYSTEM ) || defined( STEF_LOGGING_CORE )
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifdef STEF_LOGGING_SYSTEM
#define MAX_LOGGING_FRAMES 64

/* ******************************************************************************** */
// Debug Frame Handling
/* ******************************************************************************** */

typedef struct {
	char name[128];
	char info_str[256];
} logging_frame_storage_t;

typedef struct {
	const char *name;

	// Game entity / player num (-1 for none)
	int entity_num;

	// Extra values that can be used to convey frame-specific debug info
	const char *conditions;
	const char *info_str;
	int info_value;
} logging_frame_t;

// Extra static string storage
static logging_frame_storage_t logging_frame_storage[MAX_LOGGING_FRAMES];

// If an error occurs, 'freeze' the debug state so error dumps will reflect the state
// when things first started to go wrong
static qboolean logging_frozen;
static char logging_frozen_reason[1024];

static int logging_frame_count = 0;
static logging_frame_t logging_frames[MAX_LOGGING_FRAMES];

/*
==================
Logging_GetStack

Generates a string representation of current logged call stack.
==================
*/
void Logging_GetStack( char *buffer, unsigned int size ) {
	int i;
	*buffer = '\0';

	for ( i = 0; i < logging_frame_count; ++i ) {
		if ( i ) {
			Q_strcat( buffer, size, "/" );
		}
		Q_strcat( buffer, size, logging_frames[i].name );
	}
}

/*
==================
Logging_PrintStack

Useful for temporary debugging purposes.
==================
*/
void Logging_PrintStack( loggingPrintType_t printType, const char *conditions ) {
	char buffer[MAXPRINTMSG];
	Logging_GetStack( buffer, sizeof( buffer ) );
	Logging_Printf( printType, conditions, "Debug Stack: %s\n", buffer );
}

/*
==================
Logging_RegisterCrash

Handle a crash condition. Only one crash will be logged per application instance.
==================
*/
void QDECL Logging_RegisterCrash( const char *fmt, ... ) {
	if ( !logging_frozen ) {
		va_list argptr;
		logging_frozen = qtrue;
		va_start( argptr, fmt );
		Q_vsnprintf( logging_frozen_reason, sizeof( logging_frozen_reason ), fmt, argptr );
		va_end( argptr );
		Logging_Printf( LP_CONSOLE, "WARNINGS!1 CRASH", "WARNING: freezing frame logging state - %s\n", logging_frozen_reason );
		Logging_PrintStack( LP_CONSOLE, "WARNINGS!1 CRASH" );
	}
}

/*
==================
Logging_FrameLogEntry

Log function entry in frame table.
==================
*/
void Logging_FrameEntry( const char *log_conditions, const char *name, int entity_num, int info_value, qboolean reallocate_name ) {
	if ( logging_frozen ) {
		return;
	}

	if ( logging_frame_count >= MAX_LOGGING_FRAMES ) {
		// abort here before a potential stack overflow, which is a hard crash that can break the normal
		// error message routines
		Com_Error( ERR_FATAL, "logging frame overflow - possible recursive loop" );
		return;
	}

	logging_frame_t *frame = &logging_frames[logging_frame_count];
	logging_frame_storage_t *storage = &logging_frame_storage[logging_frame_count];

	frame->entity_num = entity_num;
	frame->conditions = log_conditions;
	frame->info_value = info_value;
	frame->info_str = 0;

	if ( reallocate_name ) {
		Q_strncpyz( storage->name, name, sizeof( storage->name ) );
		frame->name = storage->name;
	} else {
		frame->name = name;
	}

	++logging_frame_count;

#ifdef STEF_LUA_EVENTS
	if ( stef_lua_logframes_enabled && Stef_Lua_InitEventCall( STEF_LUA_EVENT_LOG_FRAME ) ) {
		Stef_Lua_PushInteger( "position", logging_frame_count );
		if ( Stef_Lua_RunEventCall() ) {
			Stef_Lua_FinishEventCall();
		}
	}
#endif
}

/*
==================
Logging_FrameExit

Log function exit in frame table.
==================
*/
void Logging_FrameExit( const char *name ) {
	if ( logging_frozen ) {
		return;
	}

	if ( logging_frame_count < 1 ) {
		Logging_RegisterCrash( "frame underflow" );
		return;
	}

	--logging_frame_count;
	logging_frame_t *frame = &logging_frames[logging_frame_count];

	if ( name != frame->name && Q_stricmp( name, frame->name ) ) {
		Logging_RegisterCrash( "inconsistent frame exit name - entry(%s) exit(%s)", frame->name, name );
		return;
	}

#ifdef STEF_LUA_EVENTS
	if ( stef_lua_logframes_enabled && Stef_Lua_InitEventCall( STEF_LUA_EVENT_LOG_FRAME ) ) {
		Stef_Lua_PushInteger( "position", logging_frame_count );
		if ( Stef_Lua_RunEventCall() ) {
			Stef_Lua_FinishEventCall();
		}
	}
#endif
}

/*
==================
Logging_FrameCount

Returns current number of frames on debug stack.
==================
*/
int Logging_FrameCount( void ) {
	if ( logging_frozen ) {
		return 0;
	}
	return logging_frame_count;
}

/*
==================
Logging_FrameJump

Resets number of frames to specified count, to handle jump situations.
==================
*/
void Logging_FrameJump( int count, const char *reason ) {
	if ( logging_frozen ) {
		return;
	}

	if ( count > logging_frame_count ) {
		Logging_RegisterCrash( "attempted to reset frame forwards - current(%i) target(%i)", logging_frame_count, count );
		return;
	}

	logging_frame_count = count;

#ifdef STEF_LUA_EVENTS
	if ( stef_lua_logframes_enabled && Stef_Lua_InitEventCall( STEF_LUA_EVENT_LOG_FRAME ) ) {
		Stef_Lua_PushInteger( "position", logging_frame_count );
		if ( Stef_Lua_RunEventCall() ) {
			Stef_Lua_FinishEventCall();
		}
	}
#endif
}

#ifdef STEF_LUA_SUPPORT
/*
==================
Logging_GetLuaFrameInfo
==================
*/
void Logging_GetLuaFrameInfo( int frameNum ) {
	if ( !logging_frozen && frameNum > 0 && frameNum <= logging_frame_count ) {
		const logging_frame_t *frame = &logging_frames[frameNum - 1];
		Stef_Lua_PushString( "name", frame->name );
		if ( frame->conditions && *frame->conditions ) {
			Stef_Lua_PushString( "conditions", frame->conditions );
		}
		if ( frame->entity_num >= 0 ) {
			Stef_Lua_PushInteger( "entity", frame->entity_num );
		}
	}
}
#endif
#endif

/* ******************************************************************************** */
// Event Logging
/* ******************************************************************************** */

/*
==================
Logging_PrintExt

For printlevel LP_INFO trailing newline is automatically inserted, but for
LP_DEVELOPER and LP_CONSOLE it must be included in the parameter.
==================
*/
void Logging_PrintExt( loggingPrintType_t printlevel, const char *conditions, int entity_num, const char *msg ) {
#if defined( STEF_LUA_EVENTS ) && defined( STEF_LOGGING_SYSTEM )
	if ( !stef_lua_suppress_print_event && Stef_Lua_InitEventCall( STEF_LUA_EVENT_CONSOLE_PRINT ) ) {
		Stef_Lua_PushString( "text", msg );
		Stef_Lua_PushInteger( "printlevel", printlevel >= LP_CONSOLE ? 2 : printlevel >= LP_DEVELOPER ? 1 : 0 );
		if ( conditions && *conditions ) {
			Stef_Lua_PushString( "conditions", conditions );
		}
		if ( entity_num >= 0 ) {
			Stef_Lua_PushInteger( "entitynum", entity_num );
		}
		if ( printlevel != LP_INFO ) {
			Stef_Lua_PushBoolean( "no_auto_newline", qtrue );
		}
		if ( Stef_Lua_RunEventCall() && Stef_Lua_FinishEventCall() ) {
			// lua says it already handled console printing, so don't print again here
			return;
		}

		// don't generate a duplicate event in Com_Printf
		stef_lua_suppress_print_event = qtrue;
		if ( printlevel >= LP_CONSOLE ) {
			Com_Printf( "%s", msg );
		} else if ( printlevel >= LP_DEVELOPER ) {
			Com_DPrintf( "%s", msg );
		}
		stef_lua_suppress_print_event = qfalse;
		return;
	}
#endif

	if ( printlevel >= LP_CONSOLE ) {
		Com_Printf( "%s", msg );
	} else if ( printlevel >= LP_DEVELOPER ) {
		Com_DPrintf( "%s", msg );
	}
}

/*
==================
Logging_PrintfExt
==================
*/
void QDECL Logging_PrintfExt( loggingPrintType_t printType, const char *conditions, int entityNum, const char *fmt, ... ) {
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	Logging_PrintExt( printType, conditions, entityNum, msg );
}

/*
==================
Logging_Printf
==================
*/
void QDECL Logging_Printf( loggingPrintType_t printType, const char *conditions, const char *fmt, ... ) {
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	Logging_PrintExt( printType, conditions, -1, msg );
}

#endif
