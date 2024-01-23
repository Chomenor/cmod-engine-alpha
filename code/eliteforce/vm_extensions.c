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

#ifdef STEF_VM_EXTENSIONS
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifdef STEF_SERVER_BROWSER_EXTENSIONS
int CL_ServerStatusExt( const char *serverAddress, char *serverStatusString, int maxLen, char *extString, int extLen );
#endif

#ifdef STEF_SUPPORT_STATUS_SCORES_OVERRIDE
void SV_HandleGameInfoMessage( const char *info );
#endif

#define VMEXT_TRAP_OFFSET 2400

typedef enum {
#ifdef STEF_LOGGING_SYSTEM
	VMEXT_DEBUG_FRAME_ENTRY,
	VMEXT_DEBUG_FRAME_EXIT,
	VMEXT_DEBUG_CHECK_CONDITIONS,
	VMEXT_DEBUG_PRINT_EXT,
#endif
#ifdef STEF_SERVER_BROWSER_EXTENSIONS
	VMEXT_LAN_SERVERSTATUS_EXT,
#endif
#ifdef STEF_CLIENT_ALT_SWAP_SUPPORT
	VMEXT_ALTSWAP_SET_STATE,
#endif

	VMEXT_FUNCTION_COUNT
} vmext_function_id_t;

/*
==================
VMExt_CheckGetString

Handles GetValue calls returning strings.
Returns qtrue on match and writes result to buffer, qfalse otherwise.
==================
*/
static qboolean VMExt_CheckGetString( const char *command, char *buffer, unsigned int size ) {
#ifdef STEF_LOGGING_SYSTEM
	if ( !Q_stricmp( command, "logging_get_stack" ) ) {
		Logging_GetStack( buffer, size );
		return qtrue;
	}
#endif
#ifdef STEF_VM_CONFIG_VALUES
	{
		int i;
		static const char *keys[][2] = {
			// Display this version value in the UI menu pane.
			{ "ui_version_string", "cMod HM v2.0 Alpha" },

			// Enable UI options for various cvar settings.
			{ "ui_support_r_ext_mapLightingGamma", "1" },
			{ "ui_support_r_ext_overBrightFactor", "1" },
			{ "ui_support_r_intensity", "1" },
			//{ "ui_support_r_intensity_fractional", "1" },
			{ "ui_support_r_swapInterval", "1" },
			{ "ui_support_r_ext_max_anisotropy", "1" },
			{ "ui_support_s_useOpenAL", "1" },

			// Disable UI options for deprecated settings.
			{ "ui_no_cd_key", "1" },
			{ "ui_no_a3d", "1" },
			{ "ui_skip_k_language", "1" },
			{ "ui_skip_r_glDriver", "1" },
			{ "ui_skip_r_allowExtensions", "1" },
			{ "ui_skip_r_colorbits", "1" },
			{ "ui_skip_r_depthbits", "1" },
			{ "ui_skip_r_stencilbits", "1" },
			{ "ui_skip_r_texturebits", "1" },
			{ "ui_skip_r_lowEndVideo", "1" },
			{ "ui_skip_s_khz", "1" },
			{ "ui_skip_strafe", "1" },
			{ "ui_suppress_cg_viewsize", "1" },
			{ "ui_suppress_cl_freelook", "1" },

			// Use extension commands instead of cvars to access/modify certain settings to allow more engine implementation flexibility.
			{ "ui_support_cmd_get_multisample", "1" },
			{ "ui_support_cmd_set_multisample", "1" },
			{ "ui_support_cmd_get_raw_mouse", "1" },
			{ "ui_support_cmd_set_raw_mouse", "1" },

			// Indicate support for console commands.
			{ "ui_support_minimize", "1" },
			{ "ui_support_screenshotJPEG", "1" },

			// Indicate that "globalservers" command with parameter 0 fetches all masters
			{ "ui_support_globalservers_multi_fetch", "1" },

			// Indicates that the r_mode cvar only applies to windowed mode, not fullscreen.
			{ "ui_using_windowed_r_mode", "1" },

			// Indicate that s_volume scales everything including music, and that the UI should label it as something like
			// "overall volume" instead of "effects volume".
			{ "ui_using_global_s_volume", "1" },

			// Indicates that the UI can use more modern settings for the "video options" templates in the video data menu.
			{ "ui_modern_video_templates", "1" },

			// Indicate support for renderers which can be selected via "set cl_renderer opengl1" and "set cl_renderer opengl2".
			{ "ui_support_cl_renderer_opengl1", "1" },
			{ "ui_support_cl_renderer_opengl2", "1" },
		};

		for ( i = 0; i < ARRAY_LEN( keys ); ++i ) {
			if ( !Q_stricmp( command, keys[i][0] ) ) {
				Q_strncpyz( buffer, keys[i][1], size );
				return qtrue;
			}
		}
	}

	// Special command for accessing/modifying certain settings
	if ( !Q_stricmp( command, "cmd_get_multisample" ) ) {
		Com_sprintf( buffer, size, "%i", Cvar_VariableIntegerValue( "r_ext_multisample" ) );
		return qtrue;
	}
	if ( !Q_stricmpn( command, "cmd_set_multisample ", sizeof( "cmd_set_multisample " ) - 1 ) ) {
		int value = atoi( &command[sizeof( "cmd_set_multisample " ) - 1] );
		if ( value >= 4 )
			value = 4;
		else if ( value >= 2 )
			value = 2;
		else
			value = 0;

		// For now just set both the standard and framebuffer multisample values
		// It's not pretty but it seems to work sufficiently well
		Cvar_SetSafe( "r_ext_multisample", va( "%i", value ) );
		Cvar_SetSafe( "r_ext_framebuffer_multisample", va( "%i", value ) );
		return qtrue;
	}
	if ( !Q_stricmp( command, "cmd_get_raw_mouse" ) ) {
		Com_sprintf( buffer, size, "%i", !Cvar_VariableIntegerValue( "in_mouse_warping" ) );
		return qtrue;
	}
	if ( !Q_stricmpn( command, "cmd_set_raw_mouse ", sizeof( "cmd_set_raw_mouse " ) - 1 ) ) {
		int value = atoi( &command[sizeof( "cmd_set_raw_mouse " ) - 1] );
		Cvar_SetSafe( "in_mouse_warping", value ? "0" : "1" );
		Cbuf_AddText( "in_restart\n" );
		return qtrue;
	}
#endif
#ifdef STEF_SUPPORT_STATUS_SCORES_OVERRIDE
	if ( !Q_stricmpn( command, "gameinfo ", sizeof( "gameinfo " ) - 1 ) ) {
		SV_HandleGameInfoMessage( &command[sizeof("gameinfo ") - 1] );
		return qtrue;
	}
#endif
#ifdef STEF_SERVER_ALT_SWAP_SUPPORT
	if ( !Q_stricmp( command, "sv_support_setAltSwap" ) && sv_altSwapSupport->integer ) {
		Q_strncpyz( buffer, "1", sizeof( buffer ) );
		return qtrue;
	}
#endif

	return qfalse;
}

/*
==================
VMExt_CheckGetFunction

Handles GetValue calls returning extended functions.
Returns trap id on success, -1 otherwise.
==================
*/
static int VMExt_CheckGetFunction( const char *command, vmIndex_t vm_type ) {
#ifdef STEF_LOGGING_SYSTEM
	if ( !Q_stricmp( command, "logging_frame_entry" ) )
		return VMEXT_DEBUG_FRAME_ENTRY;
	if ( !Q_stricmp( command, "logging_frame_exit" ) )
		return VMEXT_DEBUG_FRAME_EXIT;
	if ( !Q_stricmp( command, "logging_check_conditions" ) )
		return VMEXT_DEBUG_CHECK_CONDITIONS;
	if ( !Q_stricmp( command, "logging_print_ext" ) )
		return VMEXT_DEBUG_PRINT_EXT;
#endif
#ifdef STEF_SERVER_BROWSER_EXTENSIONS
	if ( !Q_stricmp( command, "trap_lan_serverstatus_ext" ) )
		return VMEXT_LAN_SERVERSTATUS_EXT;
#endif
#ifdef STEF_CLIENT_ALT_SWAP_SUPPORT
	if ( !Q_stricmp( command, "trap_altswap_set_state" ) )
		return VMEXT_ALTSWAP_SET_STATE;
#endif

	return -1;
}

/*
==================
VMExt_HandleVMSyscall

Handles VM system calls for GetValue or other extended functions.
Returns qtrue to abort standard syscall handling, qfalse otherwise.
==================
*/
qboolean VMExt_HandleVMSyscall( intptr_t *args, vmIndex_t vm_type, vm_t *vm,
		void *( *VM_ArgPtr )( intptr_t intValue ), intptr_t *retval ) {
	int function_id;
	*retval = 0;

	// Handle GetValue call
	if ( args[0] == COM_TRAP_GETVALUE ) {
		VM_CHECKBOUNDS( vm, args[1], args[2] );

		char *buffer = VMA( 1 );
		unsigned int size = args[2];
		const char *command = VMA( 3 );

		if ( size ) {
			*buffer = '\0';
		}

		if ( VMExt_CheckGetString( command, buffer, size ) ) {
			*retval = 1;
		} else {
			function_id = VMExt_CheckGetFunction( command, vm_type );
			if ( function_id >= 0 ) {
				Com_sprintf( buffer, size, "%i", VMEXT_TRAP_OFFSET + function_id );
				*retval = 1;
			}
		}

		return qtrue;
	}

	// Handle extension function calls
	function_id = args[0] - VMEXT_TRAP_OFFSET;
	if ( function_id >= 0 && function_id < VMEXT_FUNCTION_COUNT ) {

#ifdef STEF_LOGGING_SYSTEM
		if ( function_id == VMEXT_DEBUG_FRAME_ENTRY ) {
			Logging_FrameEntry( VMA( 1 ), VMA( 2 ), args[3], args[4], qtrue );
			return qtrue;
		}
		if ( function_id == VMEXT_DEBUG_FRAME_EXIT ) {
			Logging_FrameExit( VMA( 1 ) );
			return qtrue;
		}
		if ( function_id == VMEXT_DEBUG_CHECK_CONDITIONS ) {
			*retval = 1;
			return qtrue;
		}
		if ( function_id == VMEXT_DEBUG_PRINT_EXT ) {
			Logging_PrintExt( 0, VMA( 1 ), args[2], VMA( 3 ) );
			return qtrue;
		}
#endif
#ifdef STEF_SERVER_BROWSER_EXTENSIONS
		if ( function_id == VMEXT_LAN_SERVERSTATUS_EXT ) {
			VM_CHECKBOUNDS( vm, args[2], args[3] );
			VM_CHECKBOUNDS( vm, args[4], args[5] );
			*retval = CL_ServerStatusExt( VMA(1), VMA(2), args[3], VMA(4), args[5] );
			return qtrue;
		}
#endif
#ifdef STEF_CLIENT_ALT_SWAP_SUPPORT
		if ( function_id == VMEXT_ALTSWAP_SET_STATE ) {
			ClientAltSwap_SetState( args[1] );
			*retval = qtrue;
			return qtrue;
		}
#endif

		Com_Error( ERR_DROP, "Unsupported VM extension function call: %i", function_id );
	}

	return qfalse;
}

#endif
