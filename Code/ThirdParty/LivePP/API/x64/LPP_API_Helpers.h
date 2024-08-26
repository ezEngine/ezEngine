// Copyright 2011-2024 Molecular Matters GmbH, all rights reserved.

#pragma once


// ------------------------------------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus
#	define LPP_NAMESPACE					lpp::
#	define LPP_NAMESPACE_BEGIN				namespace lpp {
#	define LPP_NAMESPACE_END				}
#	define LPP_API							inline
#	define LPP_STATIC_CAST(_type)			static_cast<_type>
#	define LPP_REINTERPRET_CAST(_type)		reinterpret_cast<_type>
#	define LPP_NULL							nullptr
#	define LPP_DEFAULT_INIT(_value)			{}
#	define LPP_EXTERN_C						extern "C"
#else
#	include <stdbool.h>						// required for bool in C99
#	define LPP_NAMESPACE
#	define LPP_NAMESPACE_BEGIN
#	define LPP_NAMESPACE_END
#	define LPP_API							static inline
#	define LPP_STATIC_CAST(_type)			(_type)
#	define LPP_REINTERPRET_CAST(_type)		(_type)
#	define LPP_NULL							NULL
#	define LPP_DEFAULT_INIT(_value)			{ _value }
#	define LPP_EXTERN_C						extern
#endif

// Concatenates two preprocessor tokens, even when the tokens themselves are macros.
#define LPP_CONCATENATE_HELPER_HELPER(_a, _b)			_a##_b
#define LPP_CONCATENATE_HELPER(_a, _b)					LPP_CONCATENATE_HELPER_HELPER(_a, _b)
#define LPP_CONCATENATE(_a, _b)							LPP_CONCATENATE_HELPER(_a, _b)

// Generates a unique identifier inside a translation unit.
#define LPP_IDENTIFIER(_identifier)						LPP_CONCATENATE(_identifier, __LINE__)


LPP_NAMESPACE_BEGIN

LPP_API void LppHelperMakeLibraryNameANSI(const char* prefix, const char* pathWithoutTrailingSlash, const char* libraryName, char* const output, size_t outputSize)
{
	// we deliberately do not use memcpy or strcpy here, because that could force more #includes in the user's code
	unsigned int index = 0u;
	while (*prefix != '\0')
	{
		if (index >= (outputSize - 1u))
		{
			// no space left in buffer for this character and a null terminator
			break;
		}

		output[index++] = *prefix++;
	}

	while (*pathWithoutTrailingSlash != '\0')
	{
		if (index >= (outputSize - 1u))
		{
			// no space left in buffer for this character and a null terminator
			break;
		}

		output[index++] = *pathWithoutTrailingSlash++;
	}

	while (*libraryName != '\0')
	{
		if (index >= (outputSize - 1u))
		{
			// no space left in buffer for this character and a null terminator
			break;
		}

		output[index++] = *libraryName++;
	}

	output[index] = '\0';
}


LPP_API void LppHelperMakeLibraryName(const wchar_t* prefix, const wchar_t* pathWithoutTrailingSlash, const wchar_t* libraryName, wchar_t* const output, size_t outputSize)
{
	// we deliberately do not use memcpy or strcpy here, because that could force more #includes in the user's code
	unsigned int index = 0u;
	while (*prefix != L'\0')
	{
		if (index >= (outputSize - 1u))
		{
			// no space left in buffer for this character and a null terminator
			break;
		}

		output[index++] = *prefix++;
	}

	while (*pathWithoutTrailingSlash != L'\0')
	{
		if (index >= (outputSize - 1u))
		{
			// no space left in buffer for this character and a null terminator
			break;
		}

		output[index++] = *pathWithoutTrailingSlash++;
	}

	while (*libraryName != L'\0')
	{
		if (index >= (outputSize - 1u))
		{
			// no space left in buffer for this character and a null terminator
			break;
		}

		output[index++] = *libraryName++;
	}

	output[index] = L'\0';
}

LPP_NAMESPACE_END
