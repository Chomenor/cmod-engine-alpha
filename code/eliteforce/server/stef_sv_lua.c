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

#ifdef STEF_LUA_SERVER
#include "../../server/server.h"
#include "../lua/lua.h"
#include "../lua/lauxlib.h"
#include "../lua/lualib.h"
#include "../lua/stef_interface.h"

extern lua_State *stef_lua_state;

qboolean sv_lua_running_client_command;

#define SERVER_RUNNING ( sv.state != SS_DEAD )
#define CLIENTNUM_VALID( clientNum ) ( SERVER_RUNNING && clientNum >= 0 && clientNum < sv_maxclients->integer )
#define CLIENT_CONNECTED( clientNum ) ( CLIENTNUM_VALID( clientNum ) && svs.clients[clientNum].state >= CS_CONNECTED )

void SV_SendClientGameState( client_t *client );

/*
=======================================================================

MISC

=======================================================================
*/

/*
==================
SV_OpenDownloadError

Sends error message to client.
==================
*/
static void SV_Lua_OpenDownloadError( client_t *cl, const char *errorMessage ) {
	msg_t msg;
	byte msgBuffer[1024];

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) - 8 );
	MSG_WriteLong( &msg, cl->lastClientCommand );

	MSG_WriteByte( &msg, svc_download );
	MSG_WriteShort( &msg, 0 ); // client is expecting block zero
	MSG_WriteLong( &msg, -1 ); // illegal file size

#ifdef ELITEFORCE
	if ( !msg.compat )
#endif
	MSG_WriteString( &msg, errorMessage );

#ifdef ELITEFORCE
	if ( !msg.compat )
#endif
	MSG_WriteByte( &msg, svc_EOF );
	SV_Netchan_Transmit( cl, &msg );
}

/*
=======================================================================

CALLS FROM LUA SCRIPTS

=======================================================================
*/

/*
=================
SV_Lua_GetSVTime
=================
*/
static int SV_Lua_GetSVTime( lua_State *L ) {
	lua_pushinteger( L, sv.time );
	return 1;
}

/*
=================
SV_Lua_GetSVSTime
=================
*/
static int SV_Lua_GetSVSTime( lua_State *L ) {
	lua_pushinteger( L, svs.time );
	return 1;
}

/*
=================
SV_Lua_NetadrToString

Converts netadr object to string.
Returns "invalid" for nil parameter or error, "bot" for bots and "loopback" for local player.
=================
*/
static int SV_Lua_NetadrToString( lua_State *L ) {
	netadr_t adr;
	size_t size = 0;
	const char *data = lua_tolstring( L, 1, &size );
	if ( data && size == sizeof( adr ) ) {
		memcpy( &adr, data, sizeof( adr ) );
		lua_pushstring( L, NET_AdrToStringwPort( &adr ) );
	} else {
		lua_pushstring( L, "invalid" );
	}
	return 1;
}

/*
=================
SV_Lua_GetClientState

Returns client connection state. Returns one of the following values:
"disconnected", "connecting", "primed", "active".
=================
*/
static int SV_Lua_GetClientState( lua_State *L ) {
	int parameterValid = 0;
	int clientNum = lua_tointegerx( L, 1, &parameterValid );
	const char *result = "disconnected";
	if ( parameterValid && CLIENTNUM_VALID( clientNum ) && svs.clients[clientNum].state >= CS_CONNECTED ) {
		if ( svs.clients[clientNum].state >= CS_ACTIVE ) {
			result = "active";
		} else if ( svs.clients[clientNum].state >= CS_PRIMED ) {
			result = "primed";
		} else {
			result = "connecting";
		}
	}
	lua_pushstring( L, result );
	return 1;
}

/*
=================
SV_Lua_GetClientNetadr

Returns netadr object for client, or nil for disconnected or invalid client.
=================
*/
static int SV_Lua_GetClientNetadr( lua_State *L ) {
	int parameterValid = 0;
	int clientNum = lua_tointegerx( L, 1, &parameterValid );
	if ( parameterValid && CLIENTNUM_VALID( clientNum ) && svs.clients[clientNum].state >= CS_CONNECTED ) {
		lua_pushlstring( L, (char *)&svs.clients[clientNum].netchan.remoteAddress,
				sizeof( svs.clients[clientNum].netchan.remoteAddress ) );
	} else {
		lua_pushnil( L );
	}
	return 1;
}

/*
=================
SV_Lua_GetClientUserinfo

Returns client userinfo if valid, or nil for invalid client.
=================
*/
static int SV_Lua_GetClientUserinfo( lua_State *L ) {
	int parameterValid = 0;
	int clientNum = lua_tointegerx( L, 1, &parameterValid );
	if ( parameterValid && CLIENTNUM_VALID( clientNum ) && svs.clients[clientNum].state >= CS_CONNECTED ) {
		lua_pushstring( L, svs.clients[clientNum].userinfo );
	} else {
		lua_pushnil( L );
	}
	return 1;
}

/*
==================
SV_Lua_GetPlayerstateData

Assumes parameter 1 is index and parameter 2 is byte offset.
==================
*/
static int SV_Lua_GetPlayerstateData( lua_State *L ) {
	int indexValid = 0;
	int clientNum = lua_tointegerx( L, 1, &indexValid );
	int offsetValid = 0;
	int offset = lua_tointegerx( L, 2, &offsetValid );
	int size = lua_tointeger( L, 3 );

	if ( !indexValid || !offsetValid || size < 0 ) {
		lua_pushnil( L );
	} else {
		lua_pushlstring( L, (const char *)SV_GameClientNum( clientNum ) + offset, size );
	}

	return 1;
}

/*
==================
SV_Lua_SetPlayerstateData

Assumes parameter 1 is index and parameter 2 is byte offset.
==================
*/
static int SV_Lua_SetPlayerstateData( lua_State *L ) {
	int indexValid = 0;
	int index = lua_tointegerx( L, 1, &indexValid );
	int offsetValid = 0;
	int offset = lua_tointegerx( L, 2, &offsetValid );
	size_t size = 0;
	const char *data = lua_tolstring( L, 3, &size );

	if ( !indexValid || !offsetValid ) {
		lua_pushboolean( L, 0 );
	} else {
		lua_pushboolean( L, 1 );
		memcpy( (char *)SV_GameClientNum( index ) + offset, data, size );
	}

	return 1;
}

/*
=================
SV_Lua_IsClientCSReady

Returns whether client is in active configstring update mode, in which case configstring
changes should be passed via *cs servercmds.
=================
*/
static int SV_Lua_IsClientCSReady( lua_State *L ) {
	int parameterValid = 0;
	int clientNum = lua_tointegerx( L, 1, &parameterValid );
	int result = parameterValid && CLIENTNUM_VALID( clientNum ) && svs.clients[clientNum].state >= CS_ACTIVE &&
			( sv.state == SS_GAME || sv.restarting ) ? 1 : 0;
	lua_pushboolean( L, result );
	return 1;
}

/*
=================
SV_Lua_SendServerCmd
=================
*/
static int SV_Lua_SendServerCmd( lua_State *L ) {
	int clientValid = 0;
	const char *msg = lua_tostring( L, 2 );
	client_t *cl = NULL;
	if ( !msg ) {
		Logging_Printf( LP_CONSOLE, "WARNINGS", "lua sv.send_servercmd: invalid message\n" );
		return 0;
	}
	if ( !lua_isnil( L, 1 ) ) {
		int clientNum = lua_tointegerx( L, 1, &clientValid );
		if ( !clientValid || !CLIENT_CONNECTED( clientNum ) ) {
			Logging_Printf( LP_CONSOLE, "WARNINGS", "lua sv.send_servercmd: invalid client\n" );
			return 0;
		}
		cl = &svs.clients[clientNum];
	}
	SV_SendServerCommand( cl, "%s", msg );
	return 0;
}

/*
=================
SV_Lua_SendGamestate
=================
*/
static int SV_Lua_SendGamestate( lua_State *L ) {
	int clientValid = 0;
	int clientNum = lua_tointegerx( L, 1, &clientValid );
	if ( !clientValid || !CLIENT_CONNECTED( clientNum ) ) {
		Logging_Printf( LP_CONSOLE, "WARNINGS", "lua send_gamestate: invalid client\n" );
		return 0;
	}
	SV_SendClientGameState( &svs.clients[clientNum] );
	return 0;
}

/*
=================
SV_Lua_ExecClientCmd

Run a command as if it was issued by a client. Bypasses flood protection.
Will result in a SV_LUA_EVENT_CLIENT_COMMAND call back to Lua. Lua script is
responsible for avoiding a loop if calling back from this event.
=================
*/
static int SV_Lua_ExecClientCmd( lua_State *L ) {
	const char *cmd = lua_tostring( L, 2 );
	if ( !cmd ) {
		Logging_Printf( LP_CONSOLE, "WARNINGS", "lua sv.exec_client_cmd: invalid cmd\n" );
	} else {
		int clientValid = 0;
		int clientNum = lua_tointegerx( L, 1, &clientValid );
		if ( !clientValid || !CLIENT_CONNECTED( clientNum ) ) {
			Logging_Printf( LP_CONSOLE, "WARNINGS", "lua sv.exec_client_cmd: invalid client\n" );
		} else if ( sv_lua_running_client_command ) {
			// shouldn't happen
			Logging_Printf( LP_CONSOLE, "WARNINGS", "lua sv.exec_client_cmd: already running\n" );
		} else {
			sv_lua_running_client_command = qtrue;
			SV_ExecuteClientCommand( &svs.clients[clientNum], cmd );
			sv_lua_running_client_command = qfalse;
		}
	}
	return 0;
}

/*
=================
SV_Lua_SetupInterace
=================
*/
static void SV_Lua_SetupInterface( lua_State *L ) {
	lua_newtable( L );

	#define ADD_FUNCTION( name, function ) \
		lua_pushcfunction( L, function ); \
		lua_setfield( L, -2, name );
	ADD_FUNCTION( "get_sv_time", SV_Lua_GetSVTime );
	ADD_FUNCTION( "get_svs_time", SV_Lua_GetSVSTime );
	ADD_FUNCTION( "netadr_to_string", SV_Lua_NetadrToString );
	ADD_FUNCTION( "get_client_state", SV_Lua_GetClientState );
	ADD_FUNCTION( "get_client_netadr", SV_Lua_GetClientNetadr );
	ADD_FUNCTION( "get_client_userinfo", SV_Lua_GetClientUserinfo );
	ADD_FUNCTION( "get_playerstate_data", SV_Lua_GetPlayerstateData );
	ADD_FUNCTION( "set_playerstate_data", SV_Lua_SetPlayerstateData );
	ADD_FUNCTION( "is_client_cs_ready", SV_Lua_IsClientCSReady );
	ADD_FUNCTION( "send_servercmd", SV_Lua_SendServerCmd );
	ADD_FUNCTION( "send_gamestate", SV_Lua_SendGamestate );
	ADD_FUNCTION( "exec_client_cmd", SV_Lua_ExecClientCmd );

	#define ADD_STRING_CONSTANT( name, value ) \
		lua_pushstring( L, value ); \
		lua_setfield( L, -2, name );
	lua_newtable( L );
	ADD_STRING_CONSTANT( "clear_server", SV_LUA_EVENT_CLEAR_SERVER );
	ADD_STRING_CONSTANT( "pre_map_start", SV_LUA_EVENT_PRE_MAP_START );
	ADD_STRING_CONSTANT( "pre_map_start_infocs", SV_LUA_EVENT_PRE_MAP_START_INFOCS );
	ADD_STRING_CONSTANT( "post_map_start", SV_LUA_EVENT_POST_MAP_START );
	ADD_STRING_CONSTANT( "pre_map_restart", SV_LUA_EVENT_PRE_MAP_RESTART );
	ADD_STRING_CONSTANT( "post_map_restart", SV_LUA_EVENT_POST_MAP_RESTART );
	ADD_STRING_CONSTANT( "load_entities", SV_LUA_EVENT_LOAD_ENTITIES );
	ADD_STRING_CONSTANT( "pre_client_connect", SV_LUA_EVENT_PRE_CLIENT_CONNECT );
	ADD_STRING_CONSTANT( "post_client_connect", SV_LUA_EVENT_POST_CLIENT_CONNECT );
	ADD_STRING_CONSTANT( "post_client_disconnect", SV_LUA_EVENT_POST_CLIENT_DISCONNECT );
	ADD_STRING_CONSTANT( "post_userinfo_changed", SV_LUA_EVENT_POST_USERINFO_CHANGED );
	ADD_STRING_CONSTANT( "set_configstring", SV_LUA_EVENT_SET_CONFIGSTRING );
	ADD_STRING_CONSTANT( "update_configstring", SV_LUA_EVENT_UPDATE_CONFIGSTRING );
	ADD_STRING_CONSTANT( "gamestate_configstring", SV_LUA_EVENT_GAMESTATE_CONFIGSTRING );
	ADD_STRING_CONSTANT( "open_download", SV_LUA_EVENT_OPEN_DOWNLOAD );
	ADD_STRING_CONSTANT( "get_resource_path", SV_LUA_EVENT_GET_RESOURCE_PATH );
	ADD_STRING_CONSTANT( "game_module_consolecmd", SV_LUA_EVENT_GAME_MODULE_CONSOLECMD );
	ADD_STRING_CONSTANT( "game_module_servercmd", SV_LUA_EVENT_GAME_MODULE_SERVERCMD );
	ADD_STRING_CONSTANT( "client_cmd", SV_LUA_EVENT_CLIENT_COMMAND );
	ADD_STRING_CONSTANT( "packet_cmd", SV_LUA_EVENT_PACKET_COMMAND );
	lua_setfield( L, -2, "events" );

	lua_setglobal( L, "sv" );
}

/*
=======================================================================

CALLS TO LUA SCRIPTS

=======================================================================
*/

/*
=================
SV_Lua_SimpleClientEventCall

Executes a simple event call with client number parameter.
Returns true if "suppress" flag was returned, false otherwise.
=================
*/
qboolean SV_Lua_SimpleClientEventCall( const char *name, int clientNum ) {
	if ( Stef_Lua_InitEventCall( name ) ) {
		Stef_Lua_PushInteger( "client", clientNum );
		return Stef_Lua_RunEventCall() && Stef_Lua_FinishEventCall() ? qtrue : qfalse;
	}
	return qfalse;
}

/*
=================
SV_Lua_GamestateConfigstrings
=================
*/
qboolean SV_Lua_GamestateConfigstrings( int clientNum, msg_t *msg ) {
	qboolean override = qfalse;
	if ( Stef_Lua_InitEventCall( SV_LUA_EVENT_GAMESTATE_CONFIGSTRING ) ) {
		Stef_Lua_PushInteger( "client", clientNum );
		if ( Stef_Lua_RunEventCall() ) {
			if ( lua_getfield( stef_lua_state, -1, "configstrings" ) == LUA_TTABLE ) {
				int i;
				for ( i = 0; i < MAX_CONFIGSTRINGS; ++i ) {
					if ( lua_geti( stef_lua_state, -1, i ) == LUA_TSTRING ) {
						const char *str = lua_tostring( stef_lua_state, -1 );
						if ( str && *str ) {
							MSG_WriteByte( msg, svc_configstring );
							MSG_WriteShort( msg, i );
							MSG_WriteBigString( msg, str );
							// Com_Printf( "cs str: index(%i) value(%s)\n", i, str );
						}
					}
					lua_pop( stef_lua_state, 1 );
				}
				Logging_Printf( LP_INFO, "SV_LUA_GAMESTATE", "gamestate cs override for client %i\n", clientNum );
				override = qtrue;
			}
			lua_pop( stef_lua_state, 1 );
			Stef_Lua_FinishEventCall();
		}
	}
	return override;
}

/*
=================
SV_Lua_OpenDownload

Called when a client requests to begin a UDP download.

Event fields:
- client: client number
- name: path of the file the client is requesting

Result fields:
- cmd: can be "fspath" to specify file location, or "error" to block the download
- path: file location, if cmd is "fspath"
- message: error message, if cmd is "error"

To abort download: Sets cl->downloadName to empty. Responsible for sending error message.
To override download file: Sets cl->download to download file handle (and sets cl->downloadSize).
=================
*/
void SV_Lua_OpenDownload( int clientNum ) {
	client_t *cl = &svs.clients[clientNum];
	if ( Stef_Lua_InitEventCall( SV_LUA_EVENT_OPEN_DOWNLOAD ) ) {
		Stef_Lua_PushInteger( "client", clientNum );
		Stef_Lua_PushString( "request", cl->downloadName );
		if ( Stef_Lua_RunEventCall() ) {
			if ( lua_getfield( stef_lua_state, -1, "cmd" ) == LUA_TSTRING ) {
				const char *cmd = lua_tostring( stef_lua_state, -1 );

				if ( !Q_stricmp( cmd, "fspath" ) ) {
					// Filesystem path reference
					const char *path = "<invalid>";
					if ( lua_getfield( stef_lua_state, -2, "path" ) == LUA_TSTRING ) {
						// Attempt to open file
						path = lua_tostring( stef_lua_state, -1 );
						cl->downloadSize = FS_SV_FOpenFileRead( path, &cl->download );
					}
					if ( cl->download == FS_INVALID_HANDLE ) {
						// Abort download
						Logging_Printf( LP_CONSOLE, "WARNINGS", "WARNING: Lua script specified download path '%s',"
								" but the file could not be opened.\n", path );
						SV_Lua_OpenDownloadError( &svs.clients[clientNum], "Server failed to open file for downloading." );
						*cl->downloadName = '\0';
					}
					lua_pop( stef_lua_state, 1 );

				} else if ( !Q_stricmp( cmd, "error" ) ) {
					const char *message = "Server download error.";
					if ( lua_getfield( stef_lua_state, -2, "message" ) == LUA_TSTRING ) {
						message = lua_tostring( stef_lua_state, -1 );
					} else {
						Logging_Printf( LP_CONSOLE, "WARNINGS", "WARNING: Lua script specified error command,"
								" but no error message.\n" );
					}
					SV_Lua_OpenDownloadError( &svs.clients[clientNum], message );
					*cl->downloadName = '\0';
					lua_pop( stef_lua_state, 1 );

				} else if ( !Q_stricmp( cmd, "abort" ) ) {
					// typically used when lua is going to initiate a gamestate resend
					*cl->downloadName = '\0';

				} else if ( *cmd ) {
					Logging_Printf( LP_CONSOLE, "WARNINGS", "WARNING: Unrecognized Lua download command '%s'.\n", cmd );
				}
			}
			lua_pop( stef_lua_state, 1 );
			Stef_Lua_FinishEventCall();
		}
	}
}

/*
=================
SV_Lua_GetEntityString

Called when initializing entity string during server map startup.

Event fields:
- for_bots: true if generating entities for botlib
- text: entity string from bsp being loaded

Result fields:
- override: use modified text value
=================
*/
char *SV_Lua_GetEntityString( qboolean forBots ) {
	char *result = NULL;
	static char *botBuffer;
	static char *entityBuffer;

	// run event call
	if ( Stef_Lua_InitEventCall( SV_LUA_EVENT_LOAD_ENTITIES ) ) {
		Stef_Lua_PushBoolean( "for_bots", forBots );
		Stef_Lua_PushString( "text", CM_EntityString() );
		if ( Stef_Lua_RunEventCall() ) {
			if ( lua_getfield( stef_lua_state, -1, "override" ) == LUA_TBOOLEAN && lua_toboolean( stef_lua_state, -1 ) ) {
				const char *newString;
				lua_getfield( stef_lua_state, -2, "text" );
				newString = lua_tostring( stef_lua_state, -1 );
				if ( newString ) {
					char **buffer = forBots ? &botBuffer : &entityBuffer;
					size_t length = strlen( newString );
					if ( *buffer ) {
						free( *buffer );
					}
					*buffer = malloc( length + 1 );
					memcpy( *buffer, newString, length );
					(*buffer)[length] = '\0';
					result = *buffer;
				}
				lua_pop( stef_lua_state, 1 ); // text
			}
			lua_pop( stef_lua_state, 1 ); // override boolean
			Stef_Lua_FinishEventCall();
		}
	}

	return result ? result : CM_EntityString();
}

/*
=================
SV_Lua_GetResourcePath

Returns alternative path for aas or bsp file. type should be "bsp" or "aas".
Returns qtrue if result was returned. Can be called with NULL buffer to test only.
=================
*/
qboolean SV_Lua_GetResourcePath( const char *type, char *buffer, unsigned int bufSize ) {
	qboolean haveResult = qfalse;
	if ( buffer ) {
		*buffer = '\0';
	}
	if ( Stef_Lua_InitEventCall( SV_LUA_EVENT_GET_RESOURCE_PATH ) ) {
		Stef_Lua_PushString( "type", type );
		if ( Stef_Lua_RunEventCall() ) {
			if ( lua_getfield( stef_lua_state, -1, "path" ) == LUA_TSTRING ) {
				const char *result = lua_tostring( stef_lua_state, -1 );
				if ( buffer ) {
					Q_strncpyz( buffer, result, bufSize );
				}
				haveResult = qtrue;
			}
			lua_pop( stef_lua_state, 1 );
			Stef_Lua_FinishEventCall();
		}
	}
	return haveResult;
}

/*
=================
SV_Lua_PacketCommand

Returns qtrue to skip standard handling of command.
=================
*/
qboolean SV_Lua_PacketCommand( const netadr_t *from ) {
	if ( SVC_RateLimitAddress( from, 10, 1000 ) ) {
		if ( com_developer->integer ) {
			Com_Printf( "SVC_RemoteCommand: rate limit from %s exceeded, dropping request\n",
					NET_AdrToString( from ) );
		}
		return qtrue;
	}

	else {
		if ( Stef_Lua_InitEventCall( SV_LUA_EVENT_PACKET_COMMAND ) ) {
			lua_pushlstring( stef_lua_state, (char *)from, sizeof( *from ) );
			lua_setfield( stef_lua_state, -2, "netadr" );
			return Stef_Lua_RunEventCall() && Stef_Lua_FinishEventCall() ? qtrue : qfalse;
		}
	}

	return qfalse;
}

/*
=======================================================================

INIT

=======================================================================
*/

/*
=================
SV_Lua_InitState
=================
*/
void SV_Lua_Init( void ) {
	Com_Printf( "initializing lua server interface...\n" );
	SV_Lua_SetupInterface( stef_lua_state );
}
#endif
