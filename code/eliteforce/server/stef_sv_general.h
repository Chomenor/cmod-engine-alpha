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

// For general Elite Force server stuff; included at the end of server.h

#ifdef STEF_SERVER_RECORD
void Record_Initialize( void );
void Record_ProcessUsercmd( int clientNum, usercmd_t *usercmd );
void Record_ProcessConfigstring( int index, const char *value );
void Record_ProcessServercmd( int clientNum, const char *value );
void Record_ProcessMapLoaded( void );
void Record_ProcessSnapshot( void );
void Record_ProcessGameShutdown( void );
qboolean Record_ProcessClientConnect( const netadr_t *address, const char *userinfo, int challenge, int qport, qboolean compat );
qboolean Record_ProcessPacketEvent( const netadr_t *address, msg_t *msg, int qport );
#endif

#ifdef STEF_LUA_SERVER
#define SV_LUA_EVENT_CLEAR_SERVER "sv_clear_server"
#define SV_LUA_EVENT_PRE_MAP_START "sv_pre_map_start"
#define SV_LUA_EVENT_PRE_MAP_START_INFOCS "sv_pre_map_start_infocs"
#define SV_LUA_EVENT_POST_MAP_START "sv_post_map_start"
#define SV_LUA_EVENT_PRE_MAP_RESTART "sv_pre_map_restart"
#define SV_LUA_EVENT_POST_MAP_RESTART "sv_post_map_restart"
#define SV_LUA_EVENT_LOAD_ENTITIES "sv_load_entities"
#define SV_LUA_EVENT_PRE_CLIENT_CONNECT "sv_pre_client_connect"
#define SV_LUA_EVENT_POST_CLIENT_CONNECT "sv_post_client_connect"
#define SV_LUA_EVENT_POST_CLIENT_DISCONNECT "sv_post_client_disconnect"
#define SV_LUA_EVENT_POST_USERINFO_CHANGED "sv_post_userinfo_changed"
#define SV_LUA_EVENT_SET_CONFIGSTRING "sv_setcs"
#define SV_LUA_EVENT_UPDATE_CONFIGSTRING "sv_updatecs"
#define SV_LUA_EVENT_GAMESTATE_CONFIGSTRING "sv_gamestatecs"
#define SV_LUA_EVENT_OPEN_DOWNLOAD "sv_opendownload"
#define SV_LUA_EVENT_GET_RESOURCE_PATH "sv_get_resource_path"
#define SV_LUA_EVENT_GAME_MODULE_CONSOLECMD "sv_game_module_consolecmd"
#define SV_LUA_EVENT_GAME_MODULE_SERVERCMD "sv_game_module_servercmd"
#define SV_LUA_EVENT_CLIENT_COMMAND "sv_client_cmd"
#define SV_LUA_EVENT_PACKET_COMMAND "sv_packet_cmd"

qboolean SV_Lua_SimpleClientEventCall( const char *name, int clientNum );
qboolean SV_Lua_GamestateConfigstrings( int clientNum, msg_t *msg );
void SV_Lua_OpenDownload( int clientNum );
char *SV_Lua_GetEntityString( qboolean forBots );
qboolean SV_Lua_GetResourcePath( const char *type, char *buffer, unsigned int bufSize );
qboolean SV_Lua_PacketCommand( const netadr_t *from );
#endif

#ifdef STEF_GAMESTATE_OVERFLOW_FIX
void SV_CalculateMaxBaselines( client_t *client, msg_t msg );
#endif
