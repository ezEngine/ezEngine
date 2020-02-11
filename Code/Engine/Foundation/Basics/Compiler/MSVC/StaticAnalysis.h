
#pragma once

#undef EZ_MSVC_ANALYSIS_WARNING_PUSH
#undef EZ_MSVC_ANALYSIS_WARNING_POP
#undef EZ_MSVC_ANALYSIS_WARNING_DISABLE
#undef EZ_MSVC_WARNING_ASSUME

// These use the __pragma version to control the warnings so that they can be used within other macros etc.
#define EZ_MSVC_ANALYSIS_WARNING_PUSH __pragma(warning(push))
#define EZ_MSVC_ANALYSIS_WARNING_POP __pragma(warning(pop))
#define EZ_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber) __pragma(warning(disable : warningNumber))
#define EZ_MSVC_WARNING_ASSUME(expression) __assume(expression)
