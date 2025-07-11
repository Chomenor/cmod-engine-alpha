/* ******************************************************************************** */
// Major Features
/* ******************************************************************************** */

// [COMMON] Core EF Defines
#define ELITEFORCE

// [COMMON] Mp3 Support
#define USE_CODEC_MP3

// [FEATURE] New filesystem with various improvements
#define NEW_FILESYSTEM

/* ******************************************************************************** */
// Common
/* ******************************************************************************** */

// [FEATURE] Support for various debug logging functionality.
#define STEF_LOGGING_SYSTEM

// [FEATURE] Additional logging statements.
#if defined( STEF_LOGGING_SYSTEM )
#define STEF_LOGGING_DEFS
#endif

// [FEATURE] Core Lua scripting support.
#define STEF_LUA_SUPPORT

// [FEATURE] Additional Lua event calls.
#if defined( STEF_LUA_SUPPORT )
#define STEF_LUA_EVENTS
#endif

// [TWEAK] Default settings tweaks
// Used as a placeholder until the full settings system is ported.
#define STEF_DEFAULT_SETTINGS_TWEAKS

// [BUGFIX] Use traditional EF float casting behavior in VM for correct physics behavior
// in unpatched mods.
#define STEF_VM_FLOAT_CAST_FIX

// [BUGFIX] Workaround for bugs in EF VM code resulting in engine calls with out-of-bounds
// parameters triggering Quake3e validation error.
#define STEF_VM_BOUNDS_FIXES

/* ******************************************************************************** */
// Client
/* ******************************************************************************** */

// [FEATURE] Support VM calls to extend server querying features for the UI server browser.
#if !defined( DEDICATED )
#define STEF_SERVER_BROWSER_EXTENSIONS
#endif

// [FEATURE] Make certain engine information and setting preferences available to VMs.
// Used to enable features in the cMod VMs that are too specialized to enable by default
// for all engines.
#define STEF_VM_CONFIG_VALUES

// [FEATURE] Support VM calls to enable alt fire button swapping feature without server support.
#if !defined(DEDICATED)
#define STEF_CLIENT_ALT_SWAP_SUPPORT
#endif

// [BUGFIX] Ignore pk3 download 'pak signature' check, because it blocks some valid EF pk3s.
#define STEF_IGNORE_PAK_SIGNATURE

/* ******************************************************************************** */
// Server
/* ******************************************************************************** */

// [FEATURE] Support lua server scripting features.
#ifdef STEF_LUA_SUPPORT
#define STEF_LUA_SERVER
#endif

// [FEATURE] Support server-side recording and admin spectator features.
#define STEF_SERVER_RECORD

// [FEATURE] Allow mods to set custom player score values that are sent in response to
// status queries, instead of using the playerstate score field. Especially useful for
// Elimination mode in cases where the score field is needed for round indicator features.
#define STEF_SUPPORT_STATUS_SCORES_OVERRIDE

// [FEATURE] Support server-side handler for client alt fire button swapping features.
// Although it is possible for cgame to fall back on other methods (game module or client
// engine) for alt swapping, this handler is the preferred and most reliable method.
#define STEF_SERVER_ALT_SWAP_SUPPORT

// [TWEAK] Disable auto-running or saving config files in dedicated server build.
// Only settings manually specified using e.g. exec on command line are loaded.
#if defined( DEDICATED )
#define STEF_NO_DEDICATED_SERVER_AUTO_SETTINGS
#endif

// [TWEAK] Support minimium snaps value. This prevents older clients with low snaps
// defaults from having impaired connections on servers with higher sv_fps settings.
#define STEF_MIN_SNAPS

// [TWEAK] Ignore VM entity startsolid errors.
// Ignores error generated in g_items.c->FinishSpawningItem which can occur on Q3 maps.
#define STEF_IGNORE_STARTSOLID_ERROR

// [TWEAK] Allow bots to select random goal areas when no items are available.
// Can significantly improve bot behavior especially in sniper modes. This is
// currently hacky and unoptimized, and can cause lag issues on some maps, but
// on average the benefits seem to outweight any occasional performance issues.
#define STEF_BOT_RANDOM_GOALS

// [TWEAK] Prevent unnecessary engine modification to "nextmap" cvar which can cause
// "stuck" rotation in some cases.
#if defined( DEDICATED )
#define STEF_NO_ENGINE_NEXTMAP_SET
#endif

// [TWEAK] Fix client ping calculation to more accurately reflect packet loss,
// enabled by sv_pingFix cvar. Also force minimum ping for humans to 1.
#define STEF_SV_PINGFIX

// [TWEAK] Support loading aas files even if they don't match bsp file checksum.
// Aas files are often valid to use even if this checksum doesn't match.
#define STEF_IGNORE_AAS_CHECKSUM

// [BUGFIX] Workaround to allow certain older ioEF versions (e.g. 1.37) to successfully
// negotiate protocol version and connect to the server.
#define STEF_PROTOCOL_MSG_FIX

// [BUGFIX] Workaround to allow bots to join password-protected server running unpatched mods.
#define STEF_BOT_PASSWORD_FIX

// [BUGFIX] Limit player model length as workaround for EF-specific game code bug allowing
// a player with certain long model names to crash other clients under certain conditions.
#define STEF_MODEL_NAME_LENGTH_LIMIT

// [BUGFIX] Increase max UDP download size from 32MB to 128MB by increasing block size.
#define STEF_INCREASE_DOWNLOAD_BLOCK_SIZE

// [BUGFIX] Workaround for game code bug when creating EV_SHIELD_HIT event.
// This fixes the green shield effect so it is displayed reliably, unlike the original
// game where it depended on the position/visibility of the player relative to the map's
// origin (0 0 0) coordinate. A cvar control could be added later but needs to be
// coordinated with game module fixes in order to work consistently.
#define STEF_SHIELD_EFFECT_FIX

// [BUGFIX] Fix for "Delta parseEntitiesNum too old" errors in certain cases.
// (40+ sv_fps value + 1.20 client + high ping/bad connection)
#define STEF_SNAPSHOT_DELTA_BUFFER_FIX

// [BUGFIX] Prevent gamestate overflows by dropping entity baselines.
// Fixes errors on certain maps under certain conditions.
#define STEF_GAMESTATE_OVERFLOW_FIX

// [BUGFIX] Workarounds for potential issues with certain old server browser tools.
#define STEF_GETSTATUS_FIXES

// [BUGFIX] Workaround for issue with "Super Mario Mod" which blocks players who don't
// already have the mod loaded from being able to download it or connect to server.
#if defined( NEW_FILESYSTEM )
#define STEF_MARIO_MOD_FIX
#endif

// [BUGFIX] Fix for potential crashes or instability in Windows dedicated server. Has
// the side effect that dragging or interacting with the console may temporarily freeze
// the server.
#define STEF_WINDOWS_CONSOLE_CRASH_FIX

/* ******************************************************************************** */
// Common Dependencies
/* ******************************************************************************** */

// [COMMON] Add includes for standard cMod header files.
#define STEF_COMMON_HEADERS

// [COMMON] Use stef_cvar_defs.h header for declaring configuration cvars.
#define STEF_CVAR_DEFS

// [COMMON] VM extensions system for sharing additional functions with VM modules.
#define STEF_VM_EXTENSIONS

// [COMMON] Support basic log print functions even if STEF_LOGGING_SYSTEM is disabled.
#define STEF_LOGGING_CORE

/* ******************************************************************************** */
// Misc
/* ******************************************************************************** */

// For mp3 decoder
#define FPM_64BIT
