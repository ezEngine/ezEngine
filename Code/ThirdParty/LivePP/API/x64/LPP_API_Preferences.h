// Copyright 2011-2024 Molecular Matters GmbH, all rights reserved.

#pragma once


// ------------------------------------------------------------------------------------------------
// LOCAL PREFERENCES
// ------------------------------------------------------------------------------------------------

LPP_NAMESPACE_BEGIN

// Enum identifying different output type masks.
typedef enum LppLocalPreferencesLoggingTypeMask
{
	LPP_LOCAL_PREF_LOGGING_TYPE_MASK_NONE		= 0u,				// do not output any messages
	LPP_LOCAL_PREF_LOGGING_TYPE_MASK_SUCCESS	= 1u << 0u,			// output "Success" messages
	LPP_LOCAL_PREF_LOGGING_TYPE_MASK_INFO		= 1u << 1u,			// output "Info" messages
	LPP_LOCAL_PREF_LOGGING_TYPE_MASK_WARNING	= 1u << 2u,			// output "Warning" messages
	LPP_LOCAL_PREF_LOGGING_TYPE_MASK_ERROR		= 1u << 3u,			// output "Error" messages
	LPP_LOCAL_PREF_LOGGING_TYPE_MASK_PANIC		= 1u << 4u,			// output "Panic" messages
	LPP_LOCAL_PREF_LOGGING_TYPE_MASK_ALL		= 0xFFu				// output all messages
} LppLocalPreferencesLoggingTypeMask;


// Struct defining local preferences to use when creating an agent.
// These preferences must be defined at the time of agent creation and cannot be stored in the global or project preferences.
// They can only be set using the corresponding API/function argument and must be created by calling LppCreateDefaultLocalPreferences().
typedef struct LppLocalPreferences
{
	struct Logging
	{
		// A type mask that specifies which types of messages should be logged natively, e.g. to the Visual Studio output window using OutputDebugString.
		// Valid values are any combination of LppLocalPreferencesLoggingTypeMask enumerator values.
		unsigned int nativeTypeMask;
	} logging;
} LppLocalPreferences;


// Creates default-initialized local preferences.
LPP_API LppLocalPreferences LppCreateDefaultLocalPreferences(void)
{
	LppLocalPreferences prefs;

	// log everything by default
	prefs.logging.nativeTypeMask = LPP_LOCAL_PREF_LOGGING_TYPE_MASK_ALL;

	return prefs;
}

LPP_NAMESPACE_END


// ------------------------------------------------------------------------------------------------
// PROJECT PREFERENCES
// ------------------------------------------------------------------------------------------------

LPP_NAMESPACE_BEGIN

// Struct defining project preferences.
// They can only be set using the corresponding API/function argument and must be created by calling LppCreateDefaultProjectPreferences().
typedef struct LppProjectPreferences
{
	struct General
	{
		bool spawnBrokerForLocalConnection;
		bool showErrorOnFailedBrokerConnection;
		const wchar_t* directoryToBroker;			// directory to the Broker, either relative to the Agent/Bridge, or absolute
	} general;

	struct HotReload
	{
		const char* objectFileExtensions;
		const char* libraryFileExtensions;
		int captureToolchainEnvironmentTimeout;

		struct PreBuild
		{
			bool isEnabled;
			const wchar_t* executable;
			const wchar_t* workingDirectory;
			const char* commandLineOptions;
		} preBuild;

		bool callCompileHooksForHaltedProcesses;
		bool callLinkHooksForHaltedProcesses;
		bool callHotReloadHooksForHaltedProcesses;
	} hotReload;

	struct Compiler
	{
		const wchar_t* overrideLocation;			// isOverridden must be set to true
		const char* commandLineOptions;
		bool captureEnvironment;
		bool isOverridden;
		bool useOverrideAsFallback;					// isOverridden must be set to true
		bool forcePrecompiledHeaderPDBs;
		bool removeShowIncludes;
		bool removeSourceDependencies;
	} compiler;

	struct Linker
	{
		const wchar_t* overrideLocation;			// isOverridden must be set to true
		const char* commandLineOptions;
		bool captureEnvironment;
		bool isOverridden;
		bool useOverrideAsFallback;					// isOverridden must be set to true
		bool suppressCreationOfImportLibrary;
	} linker;

	struct ExceptionHandler
	{
		bool isEnabled;
		int order;									// 0 = last, 1 = first
	} exceptionHandler;

	struct ContinuousCompilation
	{
		const wchar_t* directory;
		int timeout;
		bool isEnabled;
	} continuousCompilation;

	struct VirtualDrive
	{
		const char* letterPlusColon;
		const wchar_t* directory;
		bool isEnabled;
	} virtualDrive;

	struct UnitySplitting
	{
		const char* fileExtensions;
		int threshold;
		bool isEnabled;
	} unitySplitting;
} LppProjectPreferences;


// Creates default-initialized project preferences.
LPP_API LppProjectPreferences LppCreateDefaultProjectPreferences(void)
{
	LppProjectPreferences prefs;

	prefs.general.spawnBrokerForLocalConnection = true;
	prefs.general.showErrorOnFailedBrokerConnection = true;
	prefs.general.directoryToBroker = L"../../Broker";

	prefs.hotReload.objectFileExtensions = ".obj;.o";
	prefs.hotReload.libraryFileExtensions = ".lib;.a";
	prefs.hotReload.captureToolchainEnvironmentTimeout = 10000;
	prefs.hotReload.preBuild.isEnabled = false;
	prefs.hotReload.preBuild.executable = L"";
	prefs.hotReload.preBuild.workingDirectory = L"./";
	prefs.hotReload.preBuild.commandLineOptions = "";
	prefs.hotReload.callCompileHooksForHaltedProcesses = false;
	prefs.hotReload.callLinkHooksForHaltedProcesses = false;
	prefs.hotReload.callHotReloadHooksForHaltedProcesses = false;

	prefs.compiler.overrideLocation = L"";
	prefs.compiler.commandLineOptions = "";
	prefs.compiler.captureEnvironment = true;
	prefs.compiler.isOverridden = false;
	prefs.compiler.useOverrideAsFallback = false;
	prefs.compiler.forcePrecompiledHeaderPDBs = false;
	prefs.compiler.removeShowIncludes = false;
	prefs.compiler.removeSourceDependencies = false;

	prefs.linker.overrideLocation = L"";
	prefs.linker.commandLineOptions = "";
	prefs.linker.captureEnvironment = true;
	prefs.linker.isOverridden = false;
	prefs.linker.useOverrideAsFallback = false;
	prefs.linker.suppressCreationOfImportLibrary = true;

	prefs.exceptionHandler.isEnabled = true;
	prefs.exceptionHandler.order = 1;

	prefs.continuousCompilation.directory = L"";
	prefs.continuousCompilation.timeout = 100;
	prefs.continuousCompilation.isEnabled = false;

	prefs.virtualDrive.letterPlusColon = "";
	prefs.virtualDrive.directory = L"";
	prefs.virtualDrive.isEnabled = false;

	prefs.unitySplitting.fileExtensions = ".cpp;.c;.cc;.c++;.cp;.cxx";
	prefs.unitySplitting.threshold = 3;
	prefs.unitySplitting.isEnabled = true;

	return prefs;
}

LPP_NAMESPACE_END


// ------------------------------------------------------------------------------------------------
// PREFERENCES API
// ------------------------------------------------------------------------------------------------

LPP_NAMESPACE_BEGIN

typedef enum LppBoolPreferences
{
	LPP_BOOL_PREF_LOGGING_PRINT_TIMESTAMPS,						// print timestamps in UI log
	LPP_BOOL_PREF_LOGGING_ENABLE_WORDWRAP,						// enable word wrap in UI log
	LPP_BOOL_PREF_NOTIFICATIONS_ENABLED,						// enable notifications
	LPP_BOOL_PREF_NOTIFICATIONS_PLAY_SOUND_ON_SUCCESS,			// play sound on success
	LPP_BOOL_PREF_NOTIFICATIONS_PLAY_SOUND_ON_ERROR,			// play sound on error
	LPP_BOOL_PREF_HOT_RELOAD_LOAD_INCOMPLETE_MODULES,			// load incomplete modules
	LPP_BOOL_PREF_HOT_RELOAD_LOAD_INCOMPLETE_COMPILANDS,		// load incomplete compilands
	LPP_BOOL_PREF_HOT_RELOAD_DELETE_PATCH_FILES,				// delete patch files upon process exit
	LPP_BOOL_PREF_HOT_RELOAD_CLEAR_LOG,							// clear log upon hot-reload
	LPP_BOOL_PREF_VISUAL_STUDIO_SHOW_MODAL_DIALOG				// show modal dialog in Visual Studio
} LppBoolPreferences;

typedef enum LppIntPreferences
{
	LPP_INT_PREF_HOT_RELOAD_TIMEOUT,							// timeout in milliseconds
	LPP_INT_PREF_HOT_RESTART_TIMEOUT,							// timeout in milliseconds

	LPP_INT_PREF_UI_STYLE,										// UI style:
																//	Light= 0,
																//	Dark = 1

	LPP_INT_PREF_LOGGING_VERBOSITY,								// UI verbosity:
																//	Default = 0,
																//	Detailed = 1

	LPP_INT_PREF_NOTIFICATIONS_FOCUS_TYPE,						// focus broker window upon:
																//	Never = 0,
																//	OnHotReloadOrHotRestart = 1,
																//	OnError = 2,
																//	OnSuccess = 3,
																//	Always = 4
} LppIntPreferences;

typedef enum LppStringPreferences
{
	LPP_STRING_PREF_LOGGING_FONT,								// font as formatted in the .json file, e.g. "Courier New,10,-1,2,400,0,0,0,0,0,0,0,0,0,0,1"
	LPP_STRING_PREF_NOTIFICATIONS_SOUND_ON_SUCCESS_PATH,		// absolute or relative path
	LPP_STRING_PREF_NOTIFICATIONS_SOUND_ON_ERROR_PATH			// absolute or relative path
} LppStringPreferences;

typedef enum LppShortcutPreferences
{
	LPP_SHORTCUT_PREF_HOT_RELOAD,								// shortcut for scheduling a hot-reload
	LPP_SHORTCUT_PREF_HOT_RESTART,								// shortcut for scheduling a hot-restart
	LPP_SHORTCUT_PREF_VISUAL_STUDIO_TOGGLE_OPTIMIZATIONS		// shortcut for toggling optimizations for the current file in Visual Studio
} LppShortcutPreferences;

LPP_NAMESPACE_END
