/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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

#include "../../server/server.h"

#ifdef STEF_SUPPORT_STATUS_SCORES_OVERRIDE
static struct {
	qboolean active[MAX_CLIENTS];
	int scores[MAX_CLIENTS];
} statusScoresOverride_state;

/*
==================
SV_HandleGameInfoMessage
==================
*/
void SV_HandleGameInfoMessage( const char *info ) {
	const char *token = Info_ValueForKey( info, "type" );
	if ( !Q_stricmp( token, "clientstate" ) ) {
		token = Info_ValueForKey( info, "client" );
		if ( *token ) {
			int clientNum = atoi( token );
			if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
				token = Info_ValueForKey( info, "score" );
				if ( *token ) {
					statusScoresOverride_state.active[clientNum] = qtrue;
					statusScoresOverride_state.scores[clientNum] = atoi( token );
				} else {
					statusScoresOverride_state.active[clientNum] = qfalse;
				}
			}
		}
	}
}

/*
==================
SV_StatusScoresOverride_Reset
==================
*/
void SV_StatusScoresOverride_Reset( void ) {
	memset( &statusScoresOverride_state, 0, sizeof( statusScoresOverride_state ) );
}

/*
==================
SV_StatusScoresOverride_AdjustScore
==================
*/
int SV_StatusScoresOverride_AdjustScore( int defaultScore, int clientNum ) {
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS && statusScoresOverride_state.active[clientNum] ) {
		return statusScoresOverride_state.scores[clientNum];
	}
	return defaultScore;
}
#endif

#ifdef STEF_GAMESTATE_OVERFLOW_FIX
/*
==================
SV_CalculateMaxBaselines

Sets client->maxEntityBaseline to the highest entity baseline index that can be written
safely without potential gamestate overflow.
==================
*/
void SV_CalculateMaxBaselines( client_t *client, msg_t msg ) {
	int start;
	const svEntity_t *svEnt;
	entityState_t nullstate;
	byte msgBuffer[MAX_MSGLEN];

	client->maxEntityBaseline = -1;
	Com_Memset( &nullstate, 0, sizeof( nullstate ) );
	msg.data = msgBuffer;

	for ( start = 0; start < MAX_GENTITIES; start++ ) {
		if ( !sv.baselineUsed[start] ) {
			continue;
		}
		svEnt = &sv.svEntities[start];
		MSG_WriteByte( &msg, svc_baseline );
		MSG_WriteDeltaEntity( &msg, &nullstate, &svEnt->baseline, qtrue );

		if ( msg.cursize + 32 >= msg.maxsize ) {
			break;
		}
		client->maxEntityBaseline = start;
	}
}
#endif
