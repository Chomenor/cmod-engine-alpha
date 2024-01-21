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
typedef enum {
	LP_INFO,		// Only log if conditions are met. Don't print to console by default.
	LP_DEVELOPER,	// Print to console if developer is set to 1.
	LP_CONSOLE,		// Always print to console.
} loggingPrintType_t;

void Logging_PrintExt( loggingPrintType_t printlevel, const char *conditions, int entity_num, const char *msg );
void QDECL Logging_PrintfExt( loggingPrintType_t printType, const char *conditions, int entityNum, const char *fmt, ... );
void QDECL Logging_Printf( loggingPrintType_t printType, const char *conditions, const char *fmt, ... );
#endif

#ifdef STEF_LOGGING_SYSTEM
void Logging_FrameEntry( const char *log_conditions, const char *name, int entity_num, int info_value, qboolean reallocate_name );
void Logging_FrameExit( const char *name );
void QDECL Logging_RegisterCrash( const char *fmt, ... );
int Logging_FrameCount( void );
void Logging_FrameJump( int count, const char *reason );
void Logging_GetStack( char *buffer, unsigned int size );
void Logging_PrintStack( loggingPrintType_t printType, const char *conditions );

#define LOGFUNCTION_XRET( prefix1, returntype, prefix2, name, typedparams, untypedparams, logcond, entity_num, info_value ) \
	prefix1 returntype prefix2 name ## _unlogged typedparams; \
	prefix1 returntype prefix2 name typedparams { \
		returntype result; \
		Logging_FrameEntry( logcond, #name, entity_num, info_value, qfalse ); \
		result = name ## _unlogged untypedparams; \
		Logging_FrameExit( #name ); \
		return result; \
	} \
	prefix1 returntype prefix2 name ## _unlogged typedparams

// Standard returning function
#define LOGFUNCTION_RET( returntype, name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XRET( , returntype, , name, typedparams, untypedparams, logcond, -1, 0 )

// Static returning function
#define LOGFUNCTION_SRET( returntype, name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XRET( static, returntype, , name, typedparams, untypedparams, logcond, -1, 0 )

// Returning function with entity num tag
#define LOGFUNCTION_ERET( returntype, name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XRET( , returntype, , name, typedparams, untypedparams, logcond, entity_num, 0 )

// Static returning function with entity num tag
#define LOGFUNCTION_SERET( returntype, name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XRET( static, returntype, , name, typedparams, untypedparams, logcond, entity_num, 0 )

#define LOGFUNCTION_XVOID( prefix1, prefix2, name, typedparams, untypedparams, logcond, entity_num, info_value ) \
	prefix1 void prefix2 name ## _unlogged typedparams; \
	prefix1 void prefix2 name typedparams { \
		Logging_FrameEntry( logcond, #name, entity_num, info_value, qfalse ); \
		name ## _unlogged untypedparams; \
		Logging_FrameExit( #name ); \
	} \
	prefix1 void prefix2 name ## _unlogged typedparams

// Standard void function
#define LOGFUNCTION_VOID( name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XVOID( , , name, typedparams, untypedparams, logcond, -1, 0 )

// Static void function
#define LOGFUNCTION_SVOID( name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XVOID( static, , name, typedparams, untypedparams, logcond, -1, 0 )

// Void function with entity num tag
#define LOGFUNCTION_EVOID( name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XVOID( , , name, typedparams, untypedparams, logcond, entity_num, 0 )

// Static void function with entity num tag
#define LOGFUNCTION_SEVOID( name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XVOID( static, , name, typedparams, untypedparams, logcond, entity_num, 0 )
#endif
