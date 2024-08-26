// Copyright 2011-2024 Molecular Matters GmbH, all rights reserved.

#pragma once

#include "LPP_API_Helpers.h"


// ------------------------------------------------------------------------------------------------
// HOOKS
// ------------------------------------------------------------------------------------------------

// Registers a hook with any signature in any section.
#if defined(__clang__)
#	define LPP_HOOK(_section, _function, ...)																												\
		extern void (* const LPP_IDENTIFIER(LPP_CONCATENATE(lpp_hook_function, _function)))(__VA_ARGS__) __attribute__((section(_section)));				\
		extern void (* const LPP_IDENTIFIER(LPP_CONCATENATE(lpp_hook_function, _function)))(__VA_ARGS__) __attribute__((section(_section))) = &_function
#elif defined(_MSC_VER)
#	define LPP_HOOK(_section, _function, ...)																																				\
		__pragma(section(_section, read)) __declspec(allocate(_section)) extern void (* const LPP_IDENTIFIER(LPP_CONCATENATE(lpp_hook_function, _function)))(__VA_ARGS__) = &_function
#else
#	error("Live++: Unknown compiler.");
#endif


// Define names for the individual hook sections.
#define LPP_PRECOMPILE_HOOK_SECTION						".lpp_precompile_hooks"
#define LPP_POSTCOMPILE_HOOK_SECTION					".lpp_postcompile_hooks"

#define LPP_COMPILE_START_HOOK_SECTION					".lpp_compile_start_hooks"
#define LPP_COMPILE_SUCCESS_HOOK_SECTION				".lpp_compile_success_hooks"
#define LPP_COMPILE_ERROR_HOOK_SECTION					".lpp_compile_error_hooks"

#define LPP_LINK_START_HOOK_SECTION						".lpp_link_start_hooks"
#define LPP_LINK_SUCCESS_HOOK_SECTION					".lpp_link_success_hooks"
#define LPP_LINK_ERROR_HOOK_SECTION						".lpp_link_error_hooks"

#define LPP_HOTRELOAD_PREPATCH_HOOK_SECTION				".lpp_hotreload_prepatch_hooks"
#define LPP_HOTRELOAD_POSTPATCH_HOOK_SECTION			".lpp_hotreload_postpatch_hooks"

#define LPP_GLOBAL_HOTRELOAD_START_HOOK_SECTION			".lpp_global_hotreload_start_hooks"
#define LPP_GLOBAL_HOTRELOAD_END_HOOK_SECTION			".lpp_global_hotreload_end_hooks"


// Define ID types to uniquely identify hook function signatures.
LPP_NAMESPACE_BEGIN

typedef struct LppPrecompileHookId { char unused; } LppPrecompileHookId;
typedef struct LppPostcompileHookId { char unused; } LppPostcompileHookId;

typedef struct LppCompileStartHookId { char unused; } LppCompileStartHookId;
typedef struct LppCompileSuccessHookId { char unused; } LppCompileSuccessHookId;
typedef struct LppCompileErrorHookId { char unused; } LppCompileErrorHookId;

typedef struct LppLinkStartHookId { char unused; } LppLinkStartHookId;
typedef struct LppLinkSuccessHookId { char unused; } LppLinkSuccessHookId;
typedef struct LppLinkErrorHookId { char unused; } LppLinkErrorHookId;

typedef struct LppHotReloadPrepatchHookId { char unused; } LppHotReloadPrepatchHookId;
typedef struct LppHotReloadPostpatchHookId { char unused; } LppHotReloadPostpatchHookId;

typedef struct LppGlobalHotReloadStartHookId { char unused; } LppGlobalHotReloadStartHookId;
typedef struct LppGlobalHotReloadEndHookId { char unused; } LppGlobalHotReloadEndHookId;

LPP_NAMESPACE_END


// Registers a pre-compile hook.
#define LPP_PRECOMPILE_HOOK(_function)					LPP_HOOK(LPP_PRECOMPILE_HOOK_SECTION, _function, LPP_NAMESPACE LppPrecompileHookId, const wchar_t* const recompiledModulePath, unsigned int filesToCompileCount)

// Registers a post-compile hook.
#define LPP_POSTCOMPILE_HOOK(_function)					LPP_HOOK(LPP_POSTCOMPILE_HOOK_SECTION, _function, LPP_NAMESPACE LppPostcompileHookId, const wchar_t* const recompiledModulePath, unsigned int filesToCompileCount)


// Registers a compile start hook.
#define LPP_COMPILE_START_HOOK(_function)				LPP_HOOK(LPP_COMPILE_START_HOOK_SECTION, _function, LPP_NAMESPACE LppCompileStartHookId, const wchar_t* const recompiledModulePath, const wchar_t* const recompiledSourcePath)

// Registers a compile success hook.
#define LPP_COMPILE_SUCCESS_HOOK(_function)				LPP_HOOK(LPP_COMPILE_SUCCESS_HOOK_SECTION, _function, LPP_NAMESPACE LppCompileSuccessHookId, const wchar_t* const recompiledModulePath, const wchar_t* const recompiledSourcePath)

// Registers a compile error hook.
#define LPP_COMPILE_ERROR_HOOK(_function)				LPP_HOOK(LPP_COMPILE_ERROR_HOOK_SECTION, _function, LPP_NAMESPACE LppCompileErrorHookId, const wchar_t* const recompiledModulePath, const wchar_t* const recompiledSourcePath, const wchar_t* const compilerOutput)


// Registers a link start hook.
#define LPP_LINK_START_HOOK(_function)					LPP_HOOK(LPP_LINK_START_HOOK_SECTION, _function, LPP_NAMESPACE LppLinkStartHookId, const wchar_t* const recompiledModulePath)

// Registers a link success hook.
#define LPP_LINK_SUCCESS_HOOK(_function)				LPP_HOOK(LPP_LINK_SUCCESS_HOOK_SECTION, _function, LPP_NAMESPACE LppLinkSuccessHookId, const wchar_t* const recompiledModulePath)

// Registers a link error hook.
#define LPP_LINK_ERROR_HOOK(_function)					LPP_HOOK(LPP_LINK_ERROR_HOOK_SECTION, _function, LPP_NAMESPACE LppLinkErrorHookId, const wchar_t* const recompiledModulePath, const wchar_t* const linkerOutput)


// Registers a hot-reload pre-patch hook.
#define LPP_HOTRELOAD_PREPATCH_HOOK(_function)			LPP_HOOK(LPP_HOTRELOAD_PREPATCH_HOOK_SECTION, _function, LPP_NAMESPACE LppHotReloadPrepatchHookId, const wchar_t* const recompiledModulePath, const wchar_t* const* const modifiedFiles, unsigned int modifiedFilesCount, const wchar_t* const* const modifiedClassLayouts, unsigned int modifiedClassLayoutsCount)

// Registers a hot-reload post-patch hook.
#define LPP_HOTRELOAD_POSTPATCH_HOOK(_function)			LPP_HOOK(LPP_HOTRELOAD_POSTPATCH_HOOK_SECTION, _function, LPP_NAMESPACE LppHotReloadPostpatchHookId, const wchar_t* const recompiledModulePath, const wchar_t* const* const modifiedFiles, unsigned int modifiedFilesCount, const wchar_t* const* const modifiedClassLayouts, unsigned int modifiedClassLayoutsCount)


// Registers a global hot-reload start hook.
#define LPP_GLOBAL_HOTRELOAD_START_HOOK(_function)		LPP_HOOK(LPP_GLOBAL_HOTRELOAD_START_HOOK_SECTION, _function, LPP_NAMESPACE LppGlobalHotReloadStartHookId)

// Registers a global hot-reload end hook.
#define LPP_GLOBAL_HOTRELOAD_END_HOOK(_function)		LPP_HOOK(LPP_GLOBAL_HOTRELOAD_END_HOOK_SECTION, _function, LPP_NAMESPACE LppGlobalHotReloadEndHookId)
