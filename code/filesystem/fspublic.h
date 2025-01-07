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

/* ******************************************************************************** */
// Definitions
/* ******************************************************************************** */

#ifndef FSLOCAL

typedef struct fsc_file_s fsc_file_t;
typedef struct fsc_shader_s fsc_shader_t;

#ifdef ELITEFORCE
#define Q3CONFIG_CFG "hmconfig.cfg"
#else
#ifdef DEDICATED
#	define Q3CONFIG_CFG "q3config_server.cfg"
#else
#	define Q3CONFIG_CFG "q3config.cfg"
#endif
#endif

#ifdef _WIN32
#define SYSTEM_NEWLINE "\r\n"
#else
#define SYSTEM_NEWLINE "\n"
#endif

// Duplicated from qcommon.h to fix build
typedef intptr_t (QDECL *vmMainFunc_t)( int command, int arg0, int arg1, int arg2 );
typedef intptr_t (QDECL *dllSyscall_t)( intptr_t callNum, ... );

typedef enum {
	H_SYSTEM,
	H_QAGAME,
	H_CGAME,
	H_Q3UI
} handleOwner_t;

// Rough adaptation of Q3E flags used by FS_FilenameCompletion command to new filesystem conventions
#define FS_MATCH_EXTERN LISTFLAG_DIRECT_SOURCE_ONLY
#define FS_MATCH_PK3s LISTFLAG_PK3_SOURCE_ONLY
#define FS_MATCH_STICK LISTFLAG_IGNORE_PURE_LIST
#define FS_MATCH_SUBDIRS 0	// Fixme
#define FS_MATCH_ANY 0

typedef qboolean ( *fnamecallback_f )( const char *filename, int length );

#ifdef ELITEFORCE
#define Q3CONFIG_CFG "hmconfig.cfg"
#define CONSOLE_HISTORY_FILE "hmhistory"
#else
#ifdef DEDICATED
#define Q3CONFIG_CFG "q3config_server.cfg"
#define CONSOLE_HISTORY_FILE "q3history_server"
#else
#define Q3CONFIG_CFG "q3config.cfg"
#define CONSOLE_HISTORY_FILE "q3history"
#endif
#endif

typedef time_t fileTime_t;
#if defined( _MSC_VER ) && defined( __clang__ )
typedef _off_t fileOffset_t;
#else
typedef off_t fileOffset_t;
#endif

// Standard buffer size to use when generating system paths
#define FS_MAX_PATH 512

#define FS_INVALID_HANDLE 0

// Lookup flags shared with renderer. Must match values in tr_public.h!
#define LOOKUPFLAG_ENABLE_DDS 1					// Enable dds format for image lookups.
#define LOOKUPFLAG_ENABLE_MTR 2					// Enable mtr shader files.

// Lookup flags
#define LOOKUPFLAG_IGNORE_PURE_LIST 4			// Ignore pure list entirely (allow all files AND ignore ordering)
#define LOOKUPFLAG_PURE_ALLOW_DIRECT_SOURCE 8	// Allow files on disk (non-pk3) when pure
#define LOOKUPFLAG_IGNORE_CURRENT_MAP 16		// Ignore current map precedence criteria
#define LOOKUPFLAG_DIRECT_SOURCE_ONLY 32		// Only allow files on disk
#define LOOKUPFLAG_PK3_SOURCE_ONLY 64			// Only allow files in pk3s
#define LOOKUPFLAG_SETTINGS_FILE 128			// Apply fs_mod_settings for auto-executed config files (e.g. q3config, autoexec, default)
#define LOOKUPFLAG_NO_DOWNLOAD_FOLDER 256		// Don't allow files from download folder
#define LOOKUPFLAG_IGNORE_SERVERCFG 512			// Ignore servercfg precedence (can still read servercfg directory; just don't prioritize)

// File List Flags
#define LISTFLAG_IGNORE_TAPAK0 1				// Ignore missionpak pak0.pk3 (to keep incompatible models out of model list)
#define LISTFLAG_IGNORE_PURE_LIST 2				// Ignore pure list entirely (allow all files AND ignore ordering)
#define LISTFLAG_PURE_ALLOW_DIRECT_SOURCE 4		// Allow files on disk (non-pk3) when pure
#define LISTFLAG_DIRECT_SOURCE_ONLY 8			// Only allow files on disk
#define LISTFLAG_PK3_SOURCE_ONLY 16				// Only allow files in pk3s

// Path Generation Flags
#define FS_NO_SANITIZE 1
#define FS_CREATE_DIRECTORIES 2
#define FS_CREATE_DIRECTORIES_FOR_FILE 4
#define FS_ALLOW_DIRECTORIES 8
#define FS_ALLOW_PK3 16
#define FS_ALLOW_DLL 32
#define FS_ALLOW_SPECIAL_CFG 64

// Config file types
typedef enum {
	FS_CONFIGTYPE_NONE,
	FS_CONFIGTYPE_DEFAULT,
	FS_CONFIGTYPE_SETTINGS,
} fs_config_type_t;

#define DEF_PUBLIC( f ) f;
#define DEF_LOCAL( f )

#endif	// !FSLOCAL

/* ******************************************************************************** */
// Main (fs_main.c)
/* ******************************************************************************** */

// State Accessors
DEF_PUBLIC( const char *FS_GetCurrentGameDir( void ) )
DEF_PUBLIC( const char *FS_GetBaseGameDir( void ) )
DEF_PUBLIC( const char *FS_PidFileDirectory( void ) )
DEF_PUBLIC( qboolean FS_Initialized( void ) )
DEF_LOCAL( int FS_ConnectedServerPureState( void ) )

// State Modifiers
DEF_PUBLIC( void FS_RegisterCurrentMap( const char *name ) )
DEF_PUBLIC( void FS_SetConnectedServerPureValue( int sv_pure ) )
DEF_PUBLIC( void FS_PureServerSetLoadedPaks( const char *hash_list, const char *name_list ) )
DEF_PUBLIC( void FS_DisconnectCleanup( void ) )
DEF_PUBLIC( void FS_UpdateModDir( void ) )
DEF_PUBLIC( qboolean FS_ConditionalRestart( int checksumFeed, qboolean clientRestart ) )
DEF_PUBLIC( void FS_BypassPure( void ) )
DEF_PUBLIC( void FS_RestorePure( void ) )
DEF_PUBLIC( void FS_SetFilenameCallback( fnamecallback_f func ) )

// Filesystem Refresh
DEF_LOCAL( void FS_Refresh( qboolean quiet ) )
DEF_LOCAL( qboolean FS_RecentlyRefreshed( void ) )
DEF_PUBLIC( void FS_AutoRefresh( void ) )

// Filesystem Initialization
DEF_LOCAL( void FS_WriteIndexCache( void ) )
DEF_PUBLIC( void FS_Startup( void ) )

/* ******************************************************************************** */
// Lookup (fs_lookup.c)
/* ******************************************************************************** */

DEF_LOCAL( void FS_DebugCompareResources( int resource1_position, int resource2_position ) )
DEF_PUBLIC( const fsc_file_t *FS_GeneralLookup( const char *name, int lookup_flags, qboolean debug ) )
DEF_PUBLIC( const fsc_shader_t *FS_ShaderLookup( const char *name, int lookup_flags, qboolean debug ) )
DEF_PUBLIC( const fsc_file_t *FS_ImageLookup( const char *name, int lookup_flags, qboolean debug ) )
DEF_PUBLIC( const fsc_file_t *FS_SoundLookup( const char *name, int lookup_flags, qboolean debug ) )
DEF_PUBLIC( const fsc_file_t *FS_VMLookup( const char *name, qboolean qvm_only, qboolean debug, qboolean *is_dll_out ) )

/* ******************************************************************************** */
// File Listing (fs_filelist.c)
/* ******************************************************************************** */

DEF_PUBLIC( void FS_FreeFileList( char **list ) )
DEF_LOCAL( char **FS_ListFilteredFiles_Flags( const char *path, const char *extension,
		const char *filter, int *numfiles_out, int flags ) )
DEF_PUBLIC( char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) )
DEF_PUBLIC( int FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) )

/* ******************************************************************************** */
// File IO (fs_fileio.c)
/* ******************************************************************************** */

// Path Generation Functions
DEF_LOCAL( unsigned int FS_GeneratePathSourcedir( int source_dir_id, const char *path1, const char *path2,
		int path1_flags, int path2_flags, char *target, unsigned int target_size ) )
DEF_PUBLIC( unsigned int FS_GeneratePath( const char *path1, const char *path2, const char *path3,
		int path1_flags, int path2_flags, int path3_flags, char *target, unsigned int target_size ) )
DEF_PUBLIC( unsigned int FS_GeneratePathWritedir( const char *path1, const char *path2,
		int path1_flags, int path2_flags, char *target, unsigned int target_size ) )

// Misc functions
DEF_PUBLIC( void FS_HomeRemove( const char *homePath ) )
DEF_PUBLIC( void FS_Remove( const char *osPath ) )
DEF_PUBLIC( void FS_Rename( const char *from, const char *to ) )
DEF_LOCAL( qboolean FS_FileInPathExists( const char *testpath ) )
DEF_PUBLIC( qboolean FS_FileExists( const char *file ) )

// File read cache
DEF_PUBLIC( void FS_ReadCache_Initialize( void ) )
DEF_PUBLIC( void FS_ReadCache_AdvanceStage( void ) )
DEF_LOCAL( void FS_ReadCache_Debug( void ) )

// Data reading
DEF_PUBLIC( char *FS_ReadData( const fsc_file_t *file, const char *path, unsigned int *size_out, const char *calling_function ) )
DEF_PUBLIC( void FS_FreeData( char *data ) )
DEF_PUBLIC( char *FS_ReadShader( const fsc_shader_t *shader ) )

// Direct read handle operations
DEF_PUBLIC( fileHandle_t FS_DirectReadHandle_Open( const fsc_file_t *file, const char *path, unsigned int *size_out ) )

// Pipe files
DEF_PUBLIC( fileHandle_t FS_PipeOpenWrite( const char *cmd, const char *filename ) )

// Common handle operations
DEF_PUBLIC( void FS_Handle_Close( fileHandle_t handle ) )
DEF_PUBLIC( void FS_Handle_CloseAll( void ) )
DEF_PUBLIC( fileHandle_t FS_CacheReadHandle_Open( const fsc_file_t *file, const char *path, unsigned int *size_out ) )
DEF_LOCAL( void FS_Handle_PrintList( void ) )
DEF_PUBLIC( int FS_VM_OpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode, handleOwner_t owner ) )
DEF_PUBLIC( int FS_VM_ReadFile( void *buffer, int len, fileHandle_t f, handleOwner_t owner ) )
DEF_PUBLIC( void FS_VM_WriteFile( void *buffer, int len, fileHandle_t f, handleOwner_t owner ) )
DEF_PUBLIC( int FS_VM_SeekFile( fileHandle_t f, long offset, fsOrigin_t origin, handleOwner_t owner ) )
DEF_PUBLIC( void FS_VM_CloseFile( fileHandle_t f, handleOwner_t owner ) )
DEF_PUBLIC( void FS_VM_CloseFiles( handleOwner_t owner ) )

// Journal files
DEF_LOCAL( void FS_Journal_WriteData( const char *data, unsigned int length ) )
DEF_LOCAL( char *FS_Journal_ReadData( void ) )

// Config files
DEF_PUBLIC( fileHandle_t FS_OpenSettingsFileWrite( const char *filename ) )

// Data reading operations
DEF_PUBLIC( int FS_ReadFile( const char *qpath, void **buffer ) )
DEF_PUBLIC( void FS_FreeFile( void *buffer ) )

// "Read-back" tracking
DEF_LOCAL( void FS_ReadbackTracker_Reset( void ) )

// FS_FOpenFile functions
DEF_PUBLIC( long FS_FOpenFileRead( const char *filename, fileHandle_t *file, qboolean uniqueFILE ) )
DEF_PUBLIC( fileHandle_t FS_FOpenFileWrite( const char *filename ) )
DEF_PUBLIC( fileHandle_t FS_FOpenFileAppend( const char *filename ) )
DEF_PUBLIC( int FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode ) )

// Misc handle operations
DEF_PUBLIC( long FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp ) )
int FS_Home_FOpenFileRead( const char *filename, fileHandle_t *fp );
DEF_PUBLIC( fileHandle_t FS_SV_FOpenFileWrite( const char *filename ) )
#ifdef STEF_LUA_SUPPORT
DEF_PUBLIC( fileHandle_t FS_SV_FOpenFileAppend( const char *filename ) )
#endif
DEF_PUBLIC( void FS_FCloseFile( fileHandle_t f ) )
DEF_PUBLIC( int FS_Read( void *buffer, int len, fileHandle_t f ) )
DEF_PUBLIC( int FS_Read2( void *buffer, int len, fileHandle_t f ) )
DEF_PUBLIC( int FS_Write( const void *buffer, int len, fileHandle_t h ) )
DEF_PUBLIC( int FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin ) )
DEF_PUBLIC( int FS_FTell( fileHandle_t f ) )
DEF_PUBLIC( void FS_Flush( fileHandle_t f ) )
DEF_PUBLIC( void FS_ForceFlush( fileHandle_t f ) )
DEF_PUBLIC( void FS_SV_Rename( const char *from, const char *to ) )
DEF_PUBLIC( qboolean FS_SV_FileExists( const char *file ) )
DEF_PUBLIC( void FS_WriteFile( const char *qpath, const void *buffer, int size ) )

/* ******************************************************************************** */
// Console Commands (fs_commands.c)
/* ******************************************************************************** */

DEF_LOCAL( void FS_RegisterCommands( void ) )

/* ******************************************************************************** */
// Client Downloading (fs_download.c)
/* ******************************************************************************** */

// Download List Handling
DEF_PUBLIC( void FS_AdvanceDownload( void ) )
DEF_PUBLIC( void FS_PrintDownloadList( void ) )
DEF_PUBLIC( void FS_RegisterDownloadList( const char *hash_list, const char *name_list ) )

// Attempted Download Tracking
DEF_PUBLIC( void FS_RegisterCurrentDownloadAttempt( qboolean http ) )
DEF_PUBLIC( void FS_ClearAttemptedDownloads( void ) )

// Download List Advancement
DEF_PUBLIC( void FS_AdvanceToNextNeededDownload( qboolean curl_disconnected ) )
DEF_PUBLIC( qboolean FS_GetCurrentDownloadInfo( char **local_name_out, char **remote_name_out,
		qboolean *curl_already_attempted_out ) )

// Download Completion
DEF_PUBLIC( void FS_FinalizeDownload( void ) )

/* ******************************************************************************** */
// Referenced Paks & Server Pk3 List Handling (fs_reference.c)
/* ******************************************************************************** */

DEF_LOCAL( void FS_RegisterReference( const fsc_file_t *file ) )
DEF_PUBLIC( void FS_ClearPakReferences( int flags ) )
DEF_PUBLIC( const char *FS_ReferencedPakNames( void ) )
DEF_PUBLIC( const char *FS_ReferencedPakPureChecksums( int maxlen ) )
DEF_PUBLIC( void FS_GenerateReferenceLists( void ) )
DEF_PUBLIC( fileHandle_t FS_OpenDownloadPak( const char *path, unsigned int *size_out ) )

/* ******************************************************************************** */
// Misc (fs_misc.c)
/* ******************************************************************************** */

// Indented debug prints
DEF_LOCAL( void FS_DebugIndentStart( void ) )
DEF_LOCAL( void FS_DebugIndentStop( void ) )
DEF_LOCAL( void QDECL FS_DPrintf( const char *fmt, ... ) __attribute__( ( format( printf, 1, 2 ) ) ) )

// Hash Table
DEF_LOCAL( void FS_Hashtable_Initialize( fs_hashtable_t *hashtable, int bucket_count ) )
DEF_LOCAL( void FS_Hashtable_Insert( fs_hashtable_t *hashtable, fs_hashtable_entry_t *entry, unsigned int hash ) )
DEF_LOCAL( fs_hashtable_iterator_t FS_Hashtable_Iterate( fs_hashtable_t *hashtable, unsigned int hash, qboolean iterate_all ) )
DEF_LOCAL( void *FS_Hashtable_Next( fs_hashtable_iterator_t *iterator ) )
DEF_LOCAL( void FS_Hashtable_Free( fs_hashtable_t *hashtable, void ( *free_entry )( fs_hashtable_entry_t *entry ) ) )
DEF_LOCAL( void FS_Hashtable_Reset( fs_hashtable_t *hashtable, void ( *free_entry )( fs_hashtable_entry_t *entry ) ) )

// Pk3 List
DEF_LOCAL( void FS_Pk3List_Initialize( pk3_list_t *pk3_list, unsigned int bucket_count ) )
DEF_LOCAL( int FS_Pk3List_Lookup( const pk3_list_t *pk3_list, unsigned int hash ) )
DEF_LOCAL( void FS_Pk3List_Insert( pk3_list_t *pk3_list, unsigned int hash ) )
DEF_LOCAL( void FS_Pk3List_Free( pk3_list_t *pk3_list ) )

// Pk3 precedence functions
DEF_LOCAL( int FS_CorePk3Position( unsigned int hash ) )
#ifdef FS_SERVERCFG_ENABLED
DEF_LOCAL( unsigned int FS_Servercfg_Priority( const char *mod_dir ) )
#endif
DEF_LOCAL( fs_modtype_t FS_GetModType( const char *mod_dir ) )

// File helper functions
DEF_PUBLIC( const char *FS_GetFileExtension( const fsc_file_t *file ) )
DEF_PUBLIC( qboolean FS_CheckFilesFromSamePk3( const fsc_file_t *file1, const fsc_file_t *file2 ) )
DEF_LOCAL( int FS_GetSourceDirID( const fsc_file_t *file ) )
DEF_LOCAL( const char *FS_GetSourceDirString( const fsc_file_t *file ) )
DEF_LOCAL( void FS_FileToStream( const fsc_file_t *file, fsc_stream_t *stream, qboolean include_source_dir,
			qboolean include_mod, qboolean include_pk3_origin, qboolean include_size ) )
DEF_LOCAL( void FS_FileToBuffer( const fsc_file_t *file, char *buffer, unsigned int buffer_size, qboolean include_source_dir,
			qboolean include_mod, qboolean include_pk3_origin, qboolean include_size ) )
DEF_PUBLIC( void FS_PrintFileLocation( const fsc_file_t *file ) )

// File disabled check function
DEF_LOCAL( int FS_CheckFileDisabled( const fsc_file_t *file, int checks ) )

// File Sorting Functions
DEF_LOCAL( void FS_WriteSortString( const char *string, fsc_stream_t *output, qboolean prioritize_shorter ) )
DEF_LOCAL( void FS_WriteSortFilename( const fsc_file_t *file, fsc_stream_t *output ) )
DEF_LOCAL( void FS_WriteSortValue( unsigned int value, fsc_stream_t *output ) )
DEF_LOCAL( void FS_WriteCoreSortKey( const fsc_file_t *file, fsc_stream_t *output, qboolean use_server_pure_list ) )
DEF_LOCAL( int FS_ComparePk3Source( const fsc_file_t *file1, const fsc_file_t *file2 ) )

// Misc Functions
DEF_PUBLIC( void FS_ExecuteConfigFile( const char *name, fs_config_type_t config_type, cbufExec_t exec_type, qboolean quiet ) )
DEF_PUBLIC( qboolean FS_ResetReadOnlyAttribute( const char *filename ) )
DEF_PUBLIC( void *FS_LoadLibrary( const char *name ) )
DEF_PUBLIC( void *FS_LoadGameDLL( const fsc_file_t *dll_file, vmMainFunc_t *entryPoint, dllSyscall_t systemcalls ) )
DEF_PUBLIC( void FS_GetModDescription( const char *modDir, char *description, int descriptionLen ) )
DEF_PUBLIC( void FS_FilenameCompletion( const char *dir, const char *ext, qboolean stripExt,
		void ( *callback )( const char *s ), int flags ) )
DEF_PUBLIC( qboolean FS_FilenameCompare( const char *s1, const char *s2 ) )
DEF_PUBLIC( qboolean FS_StripExt( char *filename, const char *ext ) )
DEF_PUBLIC( void QDECL FS_Printf( fileHandle_t f, const char *fmt, ... ) __attribute__( ( format( printf, 2, 3 ) ) ) )
DEF_LOCAL( void FS_CommaSeparatedList( const char **strings, int count, fsc_stream_t *output ) )
DEF_LOCAL( qboolean FS_idPak( const char *pak, const char *base, int numPaks ) )
DEF_PUBLIC( qboolean FS_FileIsInPAK( const char *filename, int *pChecksum, char *pakName ) )
DEF_LOCAL( void FS_SanitizeModDir( const char *source, char *target ) )

// QVM Hash Verification
DEF_LOCAL( qboolean FS_CalculateFileSha256( const fsc_file_t *file, unsigned char *output ) )
DEF_LOCAL( qboolean FS_CheckTrustedVMFile( const fsc_file_t *file ) )
DEF_LOCAL( void FS_Sha256ToStream( unsigned char *sha, fsc_stream_t *output ) )

// Core Pak Verification
DEF_LOCAL( void FS_CheckCorePaks( void ) )

/* ******************************************************************************** */
// Trusted VMs (fs_trusted_vms.c)
/* ******************************************************************************** */

DEF_LOCAL( qboolean FS_CheckTrustedVMHash( unsigned char *hash ) )
