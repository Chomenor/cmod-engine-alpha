/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2023 Noah Metzger (chomenor@gmail.com)

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

#ifdef STEF_LUA_SUPPORT
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/stef_interface.h"

lua_State *stef_lua_state;

// While lua is calling Com_Printf, prevent Com_Printf from making potentially
// recursive event calls back to lua.
qboolean stef_lua_suppress_print_event;

// For performance reasons, don't enable debug logging frame events unless specifically
// enabled by lua script.
qboolean stef_lua_logframes_enabled;

#ifdef STEF_LOGGING_SYSTEM
void Logging_GetLuaFrameInfo( int frameNum );
#endif
void Stef_Lua_CvarEmitList( void );

static char *stef_lua_parseBuffer;
static const char *stef_lua_parsePtr;

/*
=======================================================================

MISC

=======================================================================
*/

/*
=================
Stef_Lua_ResetParse
=================
*/
static void Stef_Lua_ResetParse( void ) {
	if ( stef_lua_parseBuffer ) {
		free( stef_lua_parseBuffer );
	}
	stef_lua_parseBuffer = NULL;
	stef_lua_parsePtr = NULL;
}

/*
=======================================================================

CALLS FROM LUA SYSTEM

=======================================================================
*/

/*
=================
Stef_Lua_Print
=================
*/
void Stef_Lua_Print( const char *msg, int length ) {
	char buffer[4096];
	if ( length >= sizeof( buffer ) - 1 ) {
		length = sizeof( buffer ) - 1;
	}
	Com_Memcpy( buffer, msg, length );
	buffer[length] = '\0';

	length = strlen( buffer );
	if ( length > 0 && buffer[length - 1] == '\n' ) {
		--length;
		buffer[length] = '\0';
	}

	if ( length > 0 ) {
		stef_lua_suppress_print_event = qtrue;
		Com_Printf( "lua: %s\n", buffer );
		stef_lua_suppress_print_event = qfalse;
	}
}

/*
=================
Stef_Lua_Printf
=================
*/
void Stef_Lua_Printf( const char *fmt, ... ) {
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	stef_lua_suppress_print_event = qtrue;
	Com_Printf( "lua: %s\n", msg );
	stef_lua_suppress_print_event = qfalse;
}

/*
=======================================================================

CALLS FROM LUA SCRIPTS

=======================================================================
*/

/*
=================
Stef_Lua_PrintCmd
=================
*/
static int Stef_Lua_PrintCmd( lua_State *L ) {
	const char *text = lua_tostring( L, 1 );
	if ( text && *text ) {
		Com_Printf( "%s", text );
	}
	return 0;
}

/*
=================
Stef_Lua_RunFile
=================
*/
static int Stef_Lua_RunFile( lua_State *L ) {
	const char *path = lua_tostring( L, 1 );
	if ( !path ) {
		lua_pushstring( L, "com.run_file invalid path parameter" );
		lua_error( L );
	} else {
		char *data = NULL;
		int length = FS_ReadFile( path, (void **)&data );
		if ( length > 0 ) {
			if ( luaL_loadstring( L, data ) ) {
				lua_error( L );
			} else {
				lua_call( L, 0, 1 );
			}
		} else {
			char buffer[256];
			Com_sprintf( buffer, sizeof( buffer ), "com.run_file failed to read '%s'", path );
			lua_pushstring( L, buffer );
			lua_error( L );
		}
	}
	return 1;
}

/*
=================
Stef_Lua_ReadFile
=================
*/
static int Stef_Lua_ReadFile( lua_State *L ) {
	const char *path = lua_tostring( L, 1 );
	char *data = NULL;
	int length = FS_ReadFile( path, (void **)&data );
	if ( length > 0 ) {
		lua_pushlstring( L, data, length );
	} else {
		lua_pushlstring( L, "", 0 );
	}
	lua_pushboolean( L, length >= 0 ? 1 : 0 );
	return 2;
}

/*
=================
Stef_Lua_CheckFileExists
=================
*/
static int Stef_Lua_CheckFileExists( lua_State *L ) {
	const char *path = lua_tostring( L, 1 );
	lua_pushboolean( L, path && FS_FOpenFileRead( path, NULL, qfalse ) ? 1 : 0 );
	return 1;
}

/*
=================
Stef_Lua_OpenFileSVRead
=================
*/
static int Stef_Lua_OpenFileSVRead( lua_State *L ) {
	const char *path = lua_tostring( L, 1 );
	fileHandle_t handle = FS_INVALID_HANDLE;
	if ( path ) {
		FS_SV_FOpenFileRead( path, &handle );
	}
	if ( handle != FS_INVALID_HANDLE ) {
		lua_pushinteger( L, handle );
	} else {
		lua_pushnil( L );
	}
	return 1;
}

/*
=================
Stef_Lua_OpenFileSVAppend

Useful for log creation.
=================
*/
static int Stef_Lua_OpenFileSVAppend( lua_State *L ) {
	const char *path = lua_tostring( L, 1 );
	fileHandle_t handle = FS_INVALID_HANDLE;
	if ( path ) {
		handle = FS_SV_FOpenFileAppend( path );
	}
	if ( handle != FS_INVALID_HANDLE ) {
		lua_pushinteger( L, handle );
	} else {
		lua_pushnil( L );
	}
	return 1;
}

/*
=================
Stef_Lua_WriteHandle
=================
*/
static int Stef_Lua_WriteHandle( lua_State *L ) {
	fileHandle_t handle = lua_tointeger( L, 1 );
	size_t length = 0;
	const char *data = lua_tolstring( L, 2, &length );
	if ( handle > 0 && length > 0 ) {
		FS_Write( data, length, handle );
	}
	return 0;
}

/*
=================
Stef_Lua_FlushHandle
=================
*/
static int Stef_Lua_FlushHandle( lua_State *L ) {
	fileHandle_t handle = lua_tointeger( L, 1 );
	if ( handle > 0 ) {
		FS_Flush( handle );
	}
	return 0;
}

/*
=================
Stef_Lua_CloseHandle
=================
*/
static int Stef_Lua_CloseHandle( lua_State *L ) {
	fileHandle_t handle = lua_tointeger( L, 1 );
	if ( handle > 0 ) {
		FS_FCloseFile( handle );
	}
	return 0;
}

/*
=================
Stef_Lua_ListFiles
=================
*/
static int Stef_Lua_ListFiles( lua_State *L ) {
	const char *path = lua_tostring( L, 1 );
	if ( !path ) {
		lua_pushstring( L, "com.list_files invalid path parameter" );
		lua_error( L );
	} else {
		int i;
		int numFiles = 0;
		char **files = FS_ListFiles( path, lua_tostring( L, 2 ), &numFiles );
		lua_createtable( L, numFiles, 0 );
		for ( i = 0; i < numFiles; ++i ) {
			lua_pushstring( L, files[i] );
			lua_rawseti( L, -2, i + 1 );
		}
		FS_FreeFileList( files );
	}
	return 1;
}

/*
=================
Stef_Lua_FsAutoRefresh
=================
*/
static int Stef_Lua_FsAutoRefresh( lua_State *L ) {
#ifdef NEW_FILESYSTEM
	FS_AutoRefresh();
#else
	Com_Printf( "Lua com.fs_auto_refresh failed due to build wthout NEW_FILESYSTEM.\n" );
#endif
	return 0;
}

/*
=================
Stef_Lua_RunCmd
=================
*/
static int Stef_Lua_RunCmd( lua_State *L ) {
	const char *text = lua_tostring( L, 1 );
	const char *modeArg = lua_tostring( L, 2 );
	cbufExec_t execType = EXEC_INSERT;

	if ( modeArg ) {
		if ( !Q_stricmp( modeArg, "insert" ) ) {
			execType = EXEC_INSERT;
		} else if ( !Q_stricmp( modeArg, "append" ) ) {
			execType = EXEC_APPEND;
		} else if ( !Q_stricmp( modeArg, "now" ) ) {
			execType = EXEC_NOW;
		} else {
			Logging_Printf( LP_CONSOLE, "WARNINGS", "lua com.cmd_exec: invalid mode\n" );
		}
	}

	if ( text ) {
		Cbuf_ExecuteText( execType, text );
		if ( execType == EXEC_APPEND ) {
			Cbuf_ExecuteText( EXEC_APPEND, "\n" );
		}
	}

	return 0;
}

/*
=================
Stef_Lua_CvarGetString
=================
*/
static int Stef_Lua_CvarGetString( lua_State *L ) {
	const char *name = lua_tostring( L, -1 );
	lua_pushstring( L, name ? Cvar_VariableString( name ) : "" );
	return 1;
}

/*
=================
Stef_Lua_CvarGetInteger
=================
*/
static int Stef_Lua_CvarGetInteger( lua_State *L ) {
	const char *name = lua_tostring( L, -1 );
	lua_pushinteger( L, name ? Cvar_VariableIntegerValue( name ) : 0 );
	return 1;
}

/*
=================
Stef_Lua_CvarRegister

Set the default value or flags for cvar.
=================
*/
static int Stef_Lua_CvarRegister( lua_State *L ) {
	const char *name = lua_tostring( L, 1 );
	const char *value = lua_tostring( L, 2 );
	int flags = lua_tointeger( L, 3 );
	if ( name && *name && value ) {
		Cvar_Get( name, value, flags );
	}
	return 0;
}

/*
=================
Stef_Lua_CvarList
=================
*/
static int Stef_Lua_CvarList( lua_State *L ) {
	lua_newtable( L );
	Stef_Lua_CvarEmitList();
	return 1;
}

/*
=================
Stef_Lua_CvarForceSet
=================
*/
static int Stef_Lua_CvarForceSet( lua_State *L ) {
	const char *name = lua_tostring( L, 1 );
	const char *value = lua_tostring( L, 2 );
	if ( name && *name && value ) {
		Cvar_Set( name, value );
	}
	return 0;
}

/*
=================
Stef_Lua_CvarForceRestart

Only safe to call when no QVM is running.
=================
*/
static int Stef_Lua_CvarForceRestart( lua_State *L ) {
	Cvar_Restart( qtrue );
	return 0;
}

/*
=================
Stef_Lua_Argc
=================
*/
static int Stef_Lua_Argc( lua_State *L ) {
	lua_pushinteger( L, Cmd_Argc() );
	return 1;
}

/*
=================
Stef_Lua_Argv
=================
*/
static int Stef_Lua_Argv( lua_State *L ) {
	int index = lua_tointeger( L, -1 );
	lua_pushstring( L, Cmd_Argv( index ) );
	return 1;
}

/*
=================
Stef_Lua_Cmdstr
=================
*/
static int Stef_Lua_Cmdstr( lua_State *L ) {
	lua_pushstring( L, Cmd_Cmd() );
	return 1;
}

/*
=================
Stef_Lua_SetParseString
=================
*/
static int Stef_Lua_SetParseString( lua_State *L ) {
	const char *string = lua_tostring( L, 1 );
	Stef_Lua_ResetParse();
	if ( string && *string ) {
		size_t length = strlen( string );
		stef_lua_parseBuffer = malloc( length + 1 );
		memcpy( stef_lua_parseBuffer, string, length );
		stef_lua_parseBuffer[length] = '\0';
		stef_lua_parsePtr = stef_lua_parseBuffer;
	}
	return 0;
}

/*
=================
Stef_Lua_ParseToken

Returns next token, or nil at end of iteration.

Setting optional allowLineBreaks parameter to false causes newlines to trigger an
empty string return at the location of the newline (not nil).
=================
*/
static int Stef_Lua_ParseToken( lua_State *L ) {
	const char *result = "";
	qboolean allowLineBreaks = ( !lua_isboolean( L, 1 ) || lua_toboolean( L, 1 ) ) ? qtrue : qfalse;
	if ( stef_lua_parsePtr ) {
		result = COM_ParseExt( &stef_lua_parsePtr, allowLineBreaks );
		if ( !stef_lua_parsePtr || !*stef_lua_parsePtr ) {
			Stef_Lua_ResetParse();
		}
	}
	if ( *result || stef_lua_parsePtr ) {
		lua_pushstring( L, result );
	} else {
		lua_pushnil( L );
	}
	return 1;
}

/*
=================
Stef_Lua_GetLogFrame
=================
*/
static int Stef_Lua_GetLogFrame( lua_State *L ) {
	int frameNum = lua_tointeger( L, -1 );
	lua_newtable( L );
#ifdef STEF_LOGGING_SYSTEM
	Logging_GetLuaFrameInfo( frameNum );
#endif
	return 1;
}

/*
=================
Stef_Lua_SetLogFrameEventsEnabled
=================
*/
static int Stef_Lua_SetLogFrameEventsEnabled( lua_State *L ) {
	stef_lua_logframes_enabled = lua_toboolean( L, 1 ) ? qtrue : qfalse;
	return 0;
}

/*
=================
Stef_Lua_SendPacket
=================
*/
static int Stef_Lua_SendPacket( lua_State *L ) {
	netadr_t adr;
	size_t adrLen = 0;
	const char *adrBytes = lua_tolstring( L, 1, &adrLen );
	size_t msgLen = 0;
	const char *msg = lua_tolstring( L, 2, &msgLen );

	if ( adrLen != sizeof( adr ) ) {
		Logging_Printf( LP_CONSOLE, "WARNINGS", "lua sv.send_packet: invalid address\n" );
		return 0;
	}

	Com_Memcpy( &adr, adrBytes, sizeof( adr ) );

	// Just use NS_SERVER for now; it shouldn't matter except for debug purposes
	NET_SendPacket( NS_SERVER, (int)msgLen, msg, &adr );
	return 0;
}

/*
=================
Stef_Lua_SetupInterace
=================
*/
static void Stef_Lua_SetupInterface( lua_State *L ) {
	lua_newtable( L );

	#define ADD_FUNCTION( name, function ) \
		lua_pushcfunction( L, function ); \
		lua_setfield( L, -2, name );
	ADD_FUNCTION( "print", Stef_Lua_PrintCmd );
	ADD_FUNCTION( "run_file", Stef_Lua_RunFile );
	ADD_FUNCTION( "read_file", Stef_Lua_ReadFile );
	ADD_FUNCTION( "check_file_exists", Stef_Lua_CheckFileExists );
	ADD_FUNCTION( "handle_open_sv_read", Stef_Lua_OpenFileSVRead );
	ADD_FUNCTION( "handle_open_sv_append", Stef_Lua_OpenFileSVAppend );
	ADD_FUNCTION( "handle_write", Stef_Lua_WriteHandle );
	ADD_FUNCTION( "handle_flush", Stef_Lua_FlushHandle );
	ADD_FUNCTION( "handle_close", Stef_Lua_CloseHandle );
	ADD_FUNCTION( "list_files", Stef_Lua_ListFiles );
	ADD_FUNCTION( "fs_auto_refresh", Stef_Lua_FsAutoRefresh );
	ADD_FUNCTION( "cmd_exec", Stef_Lua_RunCmd );
	ADD_FUNCTION( "cvar_get_string", Stef_Lua_CvarGetString );
	ADD_FUNCTION( "cvar_get_integer", Stef_Lua_CvarGetInteger );
	ADD_FUNCTION( "cvar_register", Stef_Lua_CvarRegister );
	ADD_FUNCTION( "cvar_list", Stef_Lua_CvarList );
	ADD_FUNCTION( "cvar_force_set", Stef_Lua_CvarForceSet );
	ADD_FUNCTION( "cvar_force_restart", Stef_Lua_CvarForceRestart );
	ADD_FUNCTION( "argc", Stef_Lua_Argc );
	ADD_FUNCTION( "argv", Stef_Lua_Argv );
	ADD_FUNCTION( "cmdstr", Stef_Lua_Cmdstr );
	ADD_FUNCTION( "parse_set_string", Stef_Lua_SetParseString );
	ADD_FUNCTION( "parse_get_token", Stef_Lua_ParseToken );
	ADD_FUNCTION( "log_frame_get", Stef_Lua_GetLogFrame );
	ADD_FUNCTION( "log_frame_set_events_enabled", Stef_Lua_SetLogFrameEventsEnabled );
	ADD_FUNCTION( "send_packet", Stef_Lua_SendPacket );

	#define ADD_STRING_CONSTANT( name, value ) \
		lua_pushstring( L, value ); \
		lua_setfield( L, -2, name );
	lua_newtable( L );
	ADD_STRING_CONSTANT( "post_frame", STEF_LUA_EVENT_POST_FRAME );
	ADD_STRING_CONSTANT( "console_cmd", STEF_LUA_EVENT_CONSOLE_COMMAND );
	ADD_STRING_CONSTANT( "console_print", STEF_LUA_EVENT_CONSOLE_PRINT );
	ADD_STRING_CONSTANT( "log_frame", STEF_LUA_EVENT_LOG_FRAME );
	lua_setfield( L, -2, "events" );

	lua_setglobal( L, "com" );
}

/*
=======================================================================

CALLS TO LUA

=======================================================================
*/

/*
=================
Stef_Lua_PushString

Add a string to a table on the top of current lua stack.
NULL key indicates to push onto array.
=================
*/
void Stef_Lua_PushString( const char *key, const char *value ) {
	if ( value && stef_lua_state && lua_istable( stef_lua_state, -1 ) ) {
		lua_pushstring( stef_lua_state, value );
		if ( key ) {
			lua_setfield( stef_lua_state, -2, key );
		} else {
			lua_rawseti( stef_lua_state, -2, lua_rawlen( stef_lua_state, -2 ) + 1 );
		}
	}
}

/*
=================
Stef_Lua_PushBytes

Adds binary string to a table on the top of current lua stack.
=================
*/
void Stef_Lua_PushBytes( const char *key, const void *value, size_t size ) {
	if ( key && value && stef_lua_state && lua_istable( stef_lua_state, -1 ) ) {
		lua_pushlstring( stef_lua_state, (const char *)value, size );
		lua_setfield( stef_lua_state, -2, key );
	}
}

/*
=================
Stef_Lua_PushInteger

Add an integer to a table on the top of current lua stack.
=================
*/
void Stef_Lua_PushInteger( const char *key, int value ) {
	if ( key && stef_lua_state && lua_istable( stef_lua_state, -1 ) ) {
		lua_pushinteger( stef_lua_state, value );
		lua_setfield( stef_lua_state, -2, key );
	}
}

/*
=================
Stef_Lua_PushBoolean

Add a boolean to a table on the top of current lua stack.
=================
*/
void Stef_Lua_PushBoolean( const char *key, qboolean value ) {
	if ( key && stef_lua_state && lua_istable( stef_lua_state, -1 ) ) {
		lua_pushboolean( stef_lua_state, value ? 1 : 0 );
		lua_setfield( stef_lua_state, -2, key );
	}
}

/*
=================
Stef_Lua_InitEventCall

On success, sets the following stack state, and returns true:
-3 - event table parameter (copy)
-2 - event function
-1 - event table parameter, pre-initialized with name field

Returns false on error.
=================
*/
qboolean Stef_Lua_InitEventCall( const char *name ) {
	if ( !stef_lua_state ) {
		return qfalse;
	}

	lua_newtable( stef_lua_state );
	lua_pushstring( stef_lua_state, name );
	lua_setfield( stef_lua_state, -2, "name" );

	lua_getglobal( stef_lua_state, "com" );
	if ( lua_getfield( stef_lua_state, -1, "eventhandler" ) != LUA_TFUNCTION ) {
		lua_pop( stef_lua_state, 3 );
		return qfalse;
	}
	lua_remove( stef_lua_state, -2 );	// com global

	lua_pushnil( stef_lua_state );
	lua_copy( stef_lua_state, -3, -1 );
	return qtrue;
}

/*
=================
Stef_Lua_RunEventCall

Runs lua event function. Should be called after successful Stef_Lua_InitEventCall.
Returns true on success, false on error or no valid return parameter.
On success, event table with potential response values will be a table on top of stack.
On error, stack will be restored to state before Stef_Lua_InitEventCall.
=================
*/
qboolean Stef_Lua_RunEventCall( void ) {
	if ( lua_pcall( stef_lua_state, 1, 1, 0 ) == LUA_OK ) {
		if ( lua_toboolean( stef_lua_state, -1 ) ) {
			lua_pop( stef_lua_state, 1 );	// return parameter
			return qtrue;
		} else {
			// No success response; either script didn't handle the event or no response was needed
			lua_pop( stef_lua_state, 2 );	// return parameter + event table
			return qfalse;
		}
	} else {
		static qboolean warned = qfalse;
		if ( !warned ) {
			stef_lua_suppress_print_event = qtrue;
			Com_Printf( "WARNING: Error calling Lua event handler: %s\nThis warning will only be displayed once.\n",
					lua_tostring( stef_lua_state, -1 ) );
			stef_lua_suppress_print_event = qfalse;
			warned = qtrue;
		}
		lua_pop( stef_lua_state, 2 );	// lua_pcall error response + event table
		return qfalse;
	}
}

/*
=================
Stef_Lua_FinishEventCall

Should be called after successful Stef_Lua_RunEventCall.
Returns whether "suppress" flag was returned by event handler.
Stack will be restored to state before Stef_Lua_InitEventCall.
=================
*/
qboolean Stef_Lua_FinishEventCall( void ) {
	qboolean suppress = qfalse;
	if ( lua_getfield( stef_lua_state, -1, "suppress" ) == LUA_TBOOLEAN &&
			lua_toboolean( stef_lua_state, -1 ) ) {
		suppress = qtrue;
	}
	lua_pop( stef_lua_state, 2 );	// event response table + retrieved 'suppress' parameter
	return suppress;
}

/*
=================
Stef_Lua_SimpleEventCall

Executes a simple event call with no extra parameters.
Returns true if "suppress" flag was returned, false otherwise.
=================
*/
qboolean Stef_Lua_SimpleEventCall( const char *name ) {
	return Stef_Lua_InitEventCall( name ) && Stef_Lua_RunEventCall() && Stef_Lua_FinishEventCall() ? qtrue : qfalse;
}

/*
=================
Stef_Lua_InitState
=================
*/
static void Stef_Lua_InitState( void ) {
	if ( !stef_lua_state ) {
		Com_Printf( "initializing lua...\n" );
		stef_lua_state = luaL_newstate();
		luaL_openlibs( stef_lua_state );
		Stef_Lua_SetupInterface( stef_lua_state );
#ifdef STEF_LUA_SERVER
		SV_Lua_Init();
#endif
	}
}

/*
=================
Stef_Lua_Init

Invoke initial startup script set by command arguments.
Example: application +set lua_startup script.lua
=================
*/
void Stef_Lua_Init( void ) {
	const char *startFile  = Cvar_VariableString( "lua_startup" );
	if ( *startFile ) {
		char *data = NULL;
		int length = FS_ReadFile( startFile, (void **)&data );
		if ( length > 0 ) {
			Stef_Lua_InitState();
			if ( luaL_loadstring( stef_lua_state, data ) || lua_pcall( stef_lua_state, 0, 0, 0 ) ) {
				Com_Printf( "lua_startup: error '%s'\n", lua_tostring( stef_lua_state, -1 ) );
				lua_pop( stef_lua_state, 1 );
			}
		} else {
			Com_Printf( "lua_startup: failed to read '%s'\n", startFile );
		}
	}
}
#endif
