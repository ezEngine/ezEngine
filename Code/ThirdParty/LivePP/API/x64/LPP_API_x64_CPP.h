// Copyright 2011-2024 Molecular Matters GmbH, all rights reserved.

#pragma once

#include "LPP_API_Helpers.h"

// users of the API should not be required to include <Windows.h>
#ifndef _INC_WINDOWS
	// <Windows.h> was not included, so we provide our own typedefs and function prototypes for the required APIs

	// opaque types
	struct HINSTANCE__;
	typedef HINSTANCE__* HINSTANCE;
	typedef HINSTANCE HMODULE;
	struct IMAGE_DOS_HEADER;

	// standard types
	typedef int BOOL;
	typedef unsigned long DWORD;
	typedef long long INT_PTR;

	// char string types
	typedef char CHAR;
	typedef CHAR* LPSTR;
	typedef const CHAR* LPCSTR;

	// wchar_t string types
	typedef wchar_t WCHAR;
	typedef WCHAR* LPWSTR;
	typedef const WCHAR* LPCWSTR;

	typedef INT_PTR (__stdcall* FARPROC)();

	// APIs
	LPP_EXTERN_C __declspec(dllimport) HMODULE __stdcall LoadLibraryA(LPCSTR lpLibFileName);
	LPP_EXTERN_C __declspec(dllimport) HMODULE __stdcall LoadLibraryW(LPCWSTR lpLibFileName);
	LPP_EXTERN_C __declspec(dllimport) BOOL __stdcall FreeLibrary(HMODULE hLibModule);
	LPP_EXTERN_C __declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
	LPP_EXTERN_C __declspec(dllimport) DWORD __stdcall GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
	LPP_EXTERN_C __declspec(dllimport) DWORD __stdcall GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);

	// required .lib for the Win32 APIs
#	pragma comment(lib, "Kernel32.lib")
#endif


// ------------------------------------------------------------------------------------------------
// WINDOWS-SPECIFIC DEFINITIONS
// ------------------------------------------------------------------------------------------------

#define LPP_PLATFORM_LIBRARY_PREFIX_ANSI	""
#define LPP_PLATFORM_LIBRARY_PREFIX			L""

#if defined(_WIN64)
#	define LPP_PLATFORM_LIBRARY_NAME_ANSI		"\\Agent\\x64\\LPP_Agent_x64_CPP.dll"
#	define LPP_PLATFORM_LIBRARY_NAME			L"\\Agent\\x64\\LPP_Agent_x64_CPP.dll"
#else
#	define LPP_PLATFORM_LIBRARY_NAME_ANSI		"\\Agent\\x64\\LPP_Agent_x86_CPP.dll"
#	define LPP_PLATFORM_LIBRARY_NAME			L"\\Agent\\x64\\LPP_Agent_x86_CPP.dll"
#endif

LPP_NAMESPACE_BEGIN

// Type of a Live++ agent module.
typedef HMODULE LppAgentModule;

// Invalid Live++ agent module.
#define LPP_INVALID_MODULE					LPP_NULL

LPP_NAMESPACE_END


// Clang complains about __ImageBase being a reserved identifier, temporarily disable this warning
#if defined(__clang__)
#	if __has_warning("-Wreserved-identifier")
#		pragma clang diagnostic push
#		pragma clang diagnostic ignored "-Wreserved-identifier"
#	endif
#endif

// Linker pseudo-variable representing the DOS header of the module we're being compiled into.
// See Raymond Chen's blog ("Accessing the current module's HINSTANCE from a static library"):
// https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483
LPP_EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Restore Clang warnings
#if defined(__clang__)
#	if __has_warning("-Wreserved-identifier")
#		pragma clang diagnostic pop
#	endif
#endif


// ------------------------------------------------------------------------------------------------
// WINDOWS-SPECIFIC API
// ------------------------------------------------------------------------------------------------

LPP_NAMESPACE_BEGIN

LPP_API HMODULE LppPlatformLoadLibraryANSI(const char* const name)
{
	return LoadLibraryA(name);
}

LPP_API HMODULE LppPlatformLoadLibrary(const wchar_t* const name)
{
	return LoadLibraryW(name);
}

LPP_API void LppPlatformUnloadLibrary(HMODULE module)
{
	FreeLibrary(module);
}

LPP_API void* LppPlatformGetFunctionAddress(HMODULE module, const char* const name)
{
	return LPP_REINTERPRET_CAST(void*)(GetProcAddress(module, name));
}

LPP_API const char* LppPlatformGetCurrentModulePathANSI(void)
{
#define LPP_MAX_PATH 260
	static char path[LPP_MAX_PATH] = LPP_DEFAULT_INIT('\0');
	GetModuleFileNameA(LPP_REINTERPRET_CAST(HMODULE)(&__ImageBase), path, LPP_MAX_PATH);
#undef LPP_MAX_PATH

	return path;
}

LPP_API const wchar_t* LppPlatformGetCurrentModulePath(void)
{
#define LPP_MAX_PATH 260
	static wchar_t path[LPP_MAX_PATH] = LPP_DEFAULT_INIT(L'\0');
	GetModuleFileNameW(LPP_REINTERPRET_CAST(HMODULE)(&__ImageBase), path, LPP_MAX_PATH);
#undef LPP_MAX_PATH

	return path;
}

LPP_NAMESPACE_END


#include "LPP_API_Version_x64_CPP.h"
#include "LPP_API.h"
