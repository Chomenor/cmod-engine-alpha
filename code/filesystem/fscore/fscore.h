/*
===========================================================================
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

// If the version in the cache file does not match this string, the cache will be rebuilt.
// This version should always be incremented when anything affecting the cache file format changes.
#define FSC_CACHE_VERSION "quake3e-fs-v14"

#define FSC_MAX_QPATH 256	// Buffer size including null terminator
#define FSC_MAX_MODDIR 32	// Buffer size including null terminator

#define	FSC_MAX_TOKEN_CHARS 1024	// based on q_shared.h
#define FSC_MAX_SHADER_NAME FSC_MAX_TOKEN_CHARS

#define FSC_NULL 0		// normal pointer
#define FSC_SPNULL 0	// fsc_stackptr_t

typedef enum {
	fsc_false,
	fsc_true
} fsc_boolean;

// pointer pseudo-type
typedef struct {
	int _unused;
} fsc_filehandle_t;

// pointer pseudo-type
typedef struct {
	int _unused;
} fsc_ospath_t;

typedef struct {
	char *data;
	unsigned int position;
	unsigned int size;
	fsc_boolean overflowed;
} fsc_stream_t;

typedef struct {
	unsigned int position;
	// data added after structure
} fsc_stack_bucket_t;

typedef struct {
	fsc_stack_bucket_t **buckets;
	int buckets_position;
	int buckets_size;
} fsc_stack_t;

typedef unsigned int fsc_stackptr_t;

#define FSC_STACK_RETRIEVE( stack, pointer, allow_null ) \
		FSC_StackRetrieve( stack, pointer, allow_null, __func__, #pointer )

#define FSC_HASHTABLE_MAX_BUCKETS ( 10 << 20 )

// This needs to be the first field of each entry of the hash table, so each entry can be casted to this type
typedef struct {
	fsc_stackptr_t next;
} fsc_hashtable_entry_t;

typedef struct {
	fsc_stackptr_t *buckets;
	fsc_stack_t *stack;
	int bucket_count;
	int utilization;
} fsc_hashtable_t;

typedef struct {
	fsc_stack_t *stack;
	fsc_stackptr_t *next_ptr;
} fsc_hashtable_iterator_t;

typedef struct {
	// Need space for 2 extra null terminators
	char buffer[FSC_MAX_QPATH + 2];
	char *dir;
	char *name;
	char *ext;
} fsc_qpath_buffer_t;

typedef enum {
	FSC_ERRORLEVEL_INFO,
	FSC_ERRORLEVEL_WARNING,
	FSC_ERRORLEVEL_FATAL,		// error handler should abort the application
} fsc_error_level_t;

typedef enum {
	FSC_ERROR_GENERAL,
	FSC_ERROR_EXTRACT,
	FSC_ERROR_PK3FILE,			// current_element: fsc_file_t (fsc_file_direct_t)
	FSC_ERROR_SHADERFILE,		// current_element: fsc_file_t (fsc_file_direct_t)
	FSC_ERROR_CROSSHAIRFILE,	// current_element: fsc_file_t
} fsc_error_category_t;

typedef void ( *fsc_error_handler_t )( fsc_error_level_t level, fsc_error_category_t category, const char *msg, void *element );

#define FSC_ASSERT( expression ) { if ( !( expression ) ) FSC_FatalErrorTagged( "assertion failed", __func__, #expression ); }

typedef struct {
	fsc_ospath_t *os_path;
	char *qpath_with_mod_dir;
	unsigned int os_timestamp;
	unsigned int filesize;
} iterate_data_t;

typedef enum {
	FSC_SEEK_SET,
	FSC_SEEK_CUR,
	FSC_SEEK_END
} fsc_seek_type_t;

#define FSC_SOURCETYPE_DIRECT 1
#define FSC_SOURCETYPE_PK3 2

#define FSC_FILEFLAG_LINKED_CONTENT 1	// This file has other content like pk3 contents or shaders linked to it
#define FSC_FILEFLAG_DLPK3 2	// This pk3 is located in a download directory
#define FSC_FILEFLAG_REFONLY_PK3 4		// Pk3 available for pure/download lists, but contents not actually indexed
#define FSC_FILEFLAG_NOLIST_PK3 8		// Pk3 indexed normally, but omitted from file listing operations

// pk3 file from any of the special directories ("downloads", "refonly", or "nolist")
#define FSC_FILEFLAGS_SPECIAL_PK3 ( FSC_FILEFLAG_DLPK3 | FSC_FILEFLAG_REFONLY_PK3 | FSC_FILEFLAG_NOLIST_PK3 )

typedef struct fsc_file_s {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	// Qpath filename as generated by FSC_SplitQpath
	// Note: The character encoding for qpaths is currently not standardized for values outside the ASCII range (val > 127)
	// It depends on the encoding used by the OS library / pk3 file, which may be UTF-8, CP-1252, or something else
	// Currently most content just uses ASCII characters
	fsc_stackptr_t qp_dir_ptr;		// includes trailing slash; empty string for no directory
	fsc_stackptr_t qp_name_ptr;		// filename excluding directory and extension
	fsc_stackptr_t qp_ext_ptr;		// includes leading dot; empty string for no extension

	unsigned int filesize;
	fsc_stackptr_t contents_cache;		// pointer to file data if cached, null otherwise
	fsc_stackptr_t next_in_directory;	// Iteration
	unsigned short flags;
	unsigned short sourcetype;
} fsc_file_t;

typedef struct {
	fsc_file_t f;

	// Enable / Disable
	int refresh_count;

	int source_dir_id;
	fsc_stackptr_t os_path_ptr;
	unsigned int os_timestamp;
	fsc_stackptr_t qp_mod_ptr;

	fsc_stackptr_t pk3dir_ptr;		// null if file is not part of a pk3dir
	unsigned int pk3_hash;			// null if file is not a valid pk3

	// For resource tallies
	unsigned int pk3_subfile_count;
	unsigned int shader_file_count;
	unsigned int shader_count;
} fsc_file_direct_t;

typedef struct {
	fsc_file_t f;
	fsc_stackptr_t source_pk3;	// fsc_file_direct_t
	unsigned int header_position;
	unsigned int compressed_size;
	short compression_method;
} fsc_file_frompk3_t;

typedef struct {
	fsc_hashtable_entry_t hte;
	fsc_stackptr_t pk3;
} fsc_pk3_hash_map_entry_t;

typedef struct fsc_filesystem_s fsc_filesystem_t;

typedef struct {
	// Identifies the sourcetype - 1 and 2 are reserved for FSC_SOURCETYPE_DIRECT and FSC_SOURCETYPE_PK3
	int sourcetype_id;

	// Returns true if active, false otherwise.
	fsc_boolean ( *is_file_active )( const fsc_file_t *file, const fsc_filesystem_t *fs );

	// Should always return a valid static string.
	const char *( *get_mod_dir )( const fsc_file_t *file, const fsc_filesystem_t *fs );

	// Buffer should be length file->filesize.
	// Returns number of bytes successfully read, which equals file->filesize on success.
	unsigned int ( *extract_data )( const fsc_file_t *file, char *buffer, const fsc_filesystem_t *fs );
} fsc_sourcetype_t;

typedef struct {
	// Useful for info print messages and to determine
	// a reasonable size for building exported hashtables
	int valid_pk3_count;
	int pk3_subfile_count;
	int shader_file_count;
	int shader_count;
	int total_file_count;
	int cacheable_file_count;
} fsc_stats_t;

#define FSC_CUSTOM_SOURCETYPE_COUNT 2
typedef struct fsc_filesystem_s {
	// Support
	fsc_stack_t general_stack;
	fsc_hashtable_t string_repository;

	// Main Filesystem
	fsc_hashtable_t files;
	int refresh_count;

	// Iteration
	fsc_hashtable_t directories;

	// Shaders
	fsc_hashtable_t shaders;

	// Crosshairs
	fsc_hashtable_t crosshairs;

	// PK3 Hash Lookup - Useful to determine files needed to download
	fsc_hashtable_t pk3_hash_lookup;

	// Custom Sourcetypes - Can be used for special applications
	fsc_sourcetype_t custom_sourcetypes[FSC_CUSTOM_SOURCETYPE_COUNT];

	// Stats
	fsc_stats_t total_stats;
	fsc_stats_t active_stats;
	fsc_stats_t new_stats;
} fsc_filesystem_t;

typedef struct fsc_shader_s {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	fsc_stackptr_t shader_name_ptr;

	fsc_stackptr_t source_file_ptr;
	unsigned int start_position;
	unsigned int end_position;
} fsc_shader_t;

typedef struct {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	unsigned int hash;
	fsc_stackptr_t source_file_ptr;
} fsc_crosshair_t;

typedef struct {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	fsc_stackptr_t qp_dir_ptr;
	fsc_stackptr_t peer_directory;
	fsc_stackptr_t sub_file;
	fsc_stackptr_t sub_directory;
} fsc_directory_t;

typedef struct {
	const fsc_file_t *file;
	fsc_stackptr_t file_ptr;

	// Internal
	fsc_filesystem_t *fs;
	int next_bucket;		// -1 - single-bucket search; >=0 - iterate-all mode
	fsc_hashtable_iterator_t hti;
	const char *dir;
	const char *name;
} fsc_file_iterator_t;

typedef struct {
	const fsc_file_direct_t *pk3;
	fsc_stackptr_t pk3_ptr;

	// Internal
	fsc_filesystem_t *fs;
	int next_bucket;		// -1 - single-bucket search; >=0 - iterate-all mode
	fsc_hashtable_iterator_t hti;
	unsigned int hash;
} fsc_pk3_iterator_t;

typedef struct {
	const fsc_shader_t *shader;
	fsc_stackptr_t shader_ptr;

	// Internal
	fsc_filesystem_t *fs;
	int next_bucket;		// -1 - single-bucket search; >=0 - iterate-all mode
	fsc_hashtable_iterator_t hti;
	const char *name;
} fsc_shader_iterator_t;

#define FSC_SANITY_HASH_BUCKETS 32768
#define FSC_SANITY_MAX_PER_HASH_BUCKET 128

typedef struct {
	// These counters are decremented as data is pulled from a pk3. If they drop below 0,
	// further data from that category will be dropped.
	unsigned int content_cache_memory;
	unsigned int content_index_memory;
	unsigned int data_read;

	// Block pk3s containing too many files/shaders with the same hash.
	unsigned char hash_buckets[FSC_SANITY_HASH_BUCKETS];

	// For error reporting
	fsc_boolean warned;
	fsc_file_direct_t *pk3file;
} fsc_sanity_limit_t;

/* ******************************************************************************** */
// Main Filesystem (fsc_main.c)
/* ******************************************************************************** */

const fsc_file_direct_t *FSC_GetBaseFile( const fsc_file_t *file, const fsc_filesystem_t *fs );
unsigned int FSC_ExtractFile( const fsc_file_t *file, char *buffer, const fsc_filesystem_t *fs );
char *FSC_ExtractFileAllocated( const fsc_file_t *file, const fsc_filesystem_t *fs );
fsc_boolean FSC_IsFileActive( const fsc_file_t *file, const fsc_filesystem_t *fs );
fsc_boolean FSC_FromDownloadPk3( const fsc_file_t *file, const fsc_filesystem_t *fs );
const char *FSC_GetModDir( const fsc_file_t *file, const fsc_filesystem_t *fs );
void FSC_FileToStream( const fsc_file_t *file, fsc_stream_t *stream, const fsc_filesystem_t *fs,
		fsc_boolean include_mod, fsc_boolean include_pk3_origin );

void FSC_RegisterFile( fsc_stackptr_t file_ptr, fsc_sanity_limit_t *sanity_limit, fsc_filesystem_t *fs );
void FSC_LoadFile( int source_dir_id, const fsc_ospath_t *os_path, const char *mod_dir, const char *pk3dir_name,
		const char *qp_dir, const char *qp_name, const char *qp_ext, unsigned int os_timestamp, unsigned int filesize,
		fsc_filesystem_t *fs );
void FSC_LoadFileFromPath( int source_dir_id, const fsc_ospath_t *os_path, const char *full_qpath, unsigned int os_timestamp,
		unsigned int filesize, fsc_filesystem_t *fs );

void FSC_FilesystemInitialize( fsc_filesystem_t *fs );
void FSC_FilesystemFree( fsc_filesystem_t *fs );
void FSC_FilesystemReset( fsc_filesystem_t *fs );
void FSC_LoadDirectoryRawPath( fsc_filesystem_t *fs, fsc_ospath_t *os_path, int source_dir_id );
void FSC_LoadDirectory( fsc_filesystem_t *fs, const char *path, int source_dir_id );

/* ******************************************************************************** */
// Misc (fsc_misc.c)
/* ******************************************************************************** */

// ***** Standard Data Stream *****

fsc_boolean FSC_StreamReadData( fsc_stream_t *stream, void *output, unsigned int length );
fsc_boolean FSC_StreamWriteData( fsc_stream_t *stream, const void *data, unsigned int length );
void FSC_StreamAppendStringSubstituted( fsc_stream_t *stream, const char *string, const char *substitution_table );
void FSC_StreamAppendString( fsc_stream_t *stream, const char *string );
fsc_stream_t FSC_InitStream( char *buffer, unsigned int bufSize );

// ***** Stack *****

void FSC_StackInitialize( fsc_stack_t *stack );
fsc_stackptr_t FSC_StackAllocate( fsc_stack_t *stack, unsigned int size );
void *FSC_StackRetrieve( const fsc_stack_t *stack, const fsc_stackptr_t pointer, fsc_boolean allow_null,
		const char *caller, const char *expression );
void FSC_StackFree( fsc_stack_t *stack );
unsigned int FSC_StackExportSize( fsc_stack_t *stack );
fsc_boolean FSC_StackExport( fsc_stack_t *stack, fsc_stream_t *stream );
fsc_boolean FSC_StackImport( fsc_stack_t *stack, fsc_stream_t *stream );

// ***** Hashtable *****

void FSC_HashtableInitialize( fsc_hashtable_t *ht, fsc_stack_t *stack, int bucket_count );
void FSC_HashtableIterateBegin( fsc_hashtable_t *ht, unsigned int hash, fsc_hashtable_iterator_t *iterator );
fsc_stackptr_t FSC_HashtableIterateNext( fsc_hashtable_iterator_t *iterator );
void FSC_HashtableInsert( fsc_stackptr_t entry_ptr, unsigned int hash, fsc_hashtable_t *ht );
void FSC_HashtableFree( fsc_hashtable_t *ht );
unsigned int FSC_HashtableExportSize( fsc_hashtable_t *ht );
fsc_boolean FSC_HashtableExport( fsc_hashtable_t *ht, fsc_stream_t *stream );
fsc_boolean FSC_HashtableImport( fsc_hashtable_t *ht, fsc_stack_t *stack, fsc_stream_t *stream );

// ***** Qpath Handling *****

void FSC_SplitQpath( const char *input, fsc_qpath_buffer_t *output, fsc_boolean ignore_extension );
unsigned int FSC_SplitLeadingDirectory( const char *input, char *buffer, unsigned int buffer_length, const char **remainder );

// ***** Sanity Limits *****

fsc_boolean FSC_SanityLimitContent( unsigned int size, unsigned int *limit_value, fsc_sanity_limit_t *sanity_limit );
fsc_boolean FSC_SanityLimitHash( unsigned int hash, fsc_sanity_limit_t *sanity_limit );

// ***** Error Handling *****

void FSC_ReportError( fsc_error_level_t level, fsc_error_category_t category, const char *msg, void *element );
void FSC_RegisterErrorHandler( fsc_error_handler_t handler );
void FSC_FatalErrorTagged( const char *msg, const char *caller, const char *expression );

// ***** Misc *****

unsigned int FSC_StringHash( const char *input1, const char *input2 );
unsigned int FSC_MemoryUseEstimate( fsc_filesystem_t *fs );
fsc_stackptr_t FSC_StringRepositoryGetString( const char *input, fsc_hashtable_t *string_repository );

/* ******************************************************************************** */
// Game Parsing Support (fsc_gameparse.c)
/* ******************************************************************************** */

void FSC_SkipRestOfLine( char **data );
char *FSC_ParseExt( char *com_token, char **data_p, fsc_boolean allowLineBreaks );
int FSC_SkipBracedSection( char **program, int depth );

/* ******************************************************************************** */
// Hash Calculation (fsc_md4.c / fsc_sha256.c)
/* ******************************************************************************** */

unsigned int FSC_BlockChecksum( const void *buffer, int length );
void FSC_CalculateSHA256( const char *data, unsigned int size, unsigned char *output );

/* ******************************************************************************** */
// OS Library Interface (fsc_os.c)
/* ******************************************************************************** */

void FSC_ErrorAbort( const char *msg );
fsc_ospath_t *FSC_StringToOSPath( const char *path ); // WARNING: Result must be freed by caller using FSC_Free!!!
char *FSC_OSPathToString( const fsc_ospath_t *os_path ); // WARNING: Result must be freed by caller using FSC_Free!!!
int FSC_OSPathSize( const fsc_ospath_t *os_path );
int FSC_OSPathCompare( const fsc_ospath_t *path1, const fsc_ospath_t *path2 );

void FSC_IterateDirectory( fsc_ospath_t *search_os_path, void( operation )( iterate_data_t *file_data,
		void *iterate_context ), void *iterate_context );

fsc_boolean FSC_RenameFileRaw( fsc_ospath_t *source_os_path, fsc_ospath_t *target_os_path );
fsc_boolean FSC_RenameFile( const char *source, const char *target );
fsc_boolean FSC_DeleteFileRaw( fsc_ospath_t *os_path );
fsc_boolean FSC_DeleteFile( const char *path );
fsc_boolean FSC_MkdirRaw( fsc_ospath_t *os_path );
fsc_boolean FSC_Mkdir( const char *directory );
fsc_filehandle_t *FSC_FOpenRaw( const fsc_ospath_t *os_path, const char *mode );
fsc_filehandle_t *FSC_FOpen( const char *path, const char *mode );
void FSC_FClose( fsc_filehandle_t *fp );
unsigned int FSC_FRead( void *dest, int size, fsc_filehandle_t *fp );
unsigned int FSC_FWrite( const void *src, int size, fsc_filehandle_t *fp );
void FSC_FFlush( fsc_filehandle_t *fp );
int FSC_FSeek( fsc_filehandle_t *fp, int offset, fsc_seek_type_t type );
unsigned int FSC_FTell( fsc_filehandle_t *fp );
void FSC_Memcpy( void *dst, const void *src, unsigned int size );
int FSC_Memcmp( const void *str1, const void *str2, unsigned int size );
void FSC_Memset( void *dst, int value, unsigned int size );
void FSC_Strncpy( char *dst, const char *src, unsigned int size );
void FSC_StrncpyLower( char *dst, const char *src, unsigned int size );
int FSC_Strcmp( const char *str1, const char *str2 );
int FSC_Stricmp( const char *str1, const char *str2 );
int FSC_Strlen( const char *str );
void *FSC_Malloc( unsigned int size );
void *FSC_Calloc( unsigned int size );
void FSC_Free( void *allocation );

/* ******************************************************************************** */
// PK3 Handling (fsc_pk3.c)
/* ******************************************************************************** */

// receive_hash_data is used for standalone hash calculation operations,
// and should be nulled during normal filesystem loading
void FSC_LoadPk3( fsc_ospath_t *os_path, fsc_filesystem_t *fs, fsc_stackptr_t sourcefile_ptr,
		void ( *receive_hash_data )( void *context, char *data, int size ), void *receive_hash_data_context );
unsigned int FSC_GetPk3HashRawPath( fsc_ospath_t *os_path );
unsigned int FSC_GetPk3Hash( const char *path );
void FSC_RegisterPk3HashLookup( fsc_stackptr_t pk3_file_ptr, fsc_hashtable_t *pk3_hash_lookup, fsc_stack_t *stack );

typedef struct fsc_pk3handle_s fsc_pk3handle_t;
fsc_pk3handle_t *FSC_Pk3HandleOpen( const fsc_file_frompk3_t *file, int input_buffer_size, const fsc_filesystem_t *fs );
void FSC_Pk3HandleClose( fsc_pk3handle_t *handle );
unsigned int FSC_Pk3HandleRead( fsc_pk3handle_t *handle, char *buffer, unsigned int length );
extern fsc_sourcetype_t pk3_sourcetype;

/* ******************************************************************************** */
// Shader Lookup (fsc_shader.c)
/* ******************************************************************************** */

int FSC_IndexShaderFile( fsc_filesystem_t *fs, fsc_stackptr_t source_file_ptr, fsc_sanity_limit_t *sanity_limit );
fsc_boolean FSC_IsShaderActive( fsc_filesystem_t *fs, const fsc_shader_t *shader );

/* ******************************************************************************** */
// Crosshair Lookup (fsc_crosshair.c)
/* ******************************************************************************** */

fsc_boolean FSC_IndexCrosshair( fsc_filesystem_t *fs, fsc_stackptr_t source_file_ptr, fsc_sanity_limit_t *sanity_limit );
fsc_boolean FSC_IsCrosshairActive( fsc_filesystem_t *fs, const fsc_crosshair_t *crosshair );

/* ******************************************************************************** */
// Iteration (fsc_iteration.c)
/* ******************************************************************************** */

void FSC_IterationRegisterFile( fsc_stackptr_t file_ptr, fsc_hashtable_t *directories, fsc_hashtable_t *string_repository, fsc_stack_t *stack );

fsc_file_iterator_t FSC_FileIteratorOpen( fsc_filesystem_t *fs, const char *dir, const char *name );
fsc_file_iterator_t FSC_FileIteratorOpenAll( fsc_filesystem_t *fs );
fsc_boolean FSC_FileIteratorAdvance( fsc_file_iterator_t *it );

fsc_pk3_iterator_t FSC_Pk3IteratorOpen( fsc_filesystem_t *fs, unsigned int hash );
fsc_pk3_iterator_t FSC_Pk3IteratorOpenAll( fsc_filesystem_t *fs );
fsc_boolean FSC_Pk3IteratorAdvance( fsc_pk3_iterator_t *it );

fsc_shader_iterator_t FSC_ShaderIteratorOpen( fsc_filesystem_t *fs, const char *name );
fsc_shader_iterator_t FSC_ShaderIteratorOpenAll( fsc_filesystem_t *fs );
fsc_boolean FSC_ShaderIteratorAdvance( fsc_shader_iterator_t *it );

/* ******************************************************************************** */
// Index Cache (fsc_cache.c)
/* ******************************************************************************** */

fsc_boolean FSC_CacheExportFileRawPath( fsc_filesystem_t *source_fs, fsc_ospath_t *os_path );
fsc_boolean FSC_CacheExportFile( fsc_filesystem_t *source_fs, const char *path );
fsc_boolean FSC_CacheImportFileRawPath( fsc_ospath_t *os_path, fsc_filesystem_t *target_fs );
fsc_boolean FSC_CacheImportFile( const char *path, fsc_filesystem_t *target_fs );
