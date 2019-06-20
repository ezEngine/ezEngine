#pragma once

#ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#define EZ_FOUNDATION_INTERNAL_HEADER_ALLOWED 1
#else
#define EZ_FOUNDATION_INTERNAL_HEADER_ALLOWED 0
#endif

#define EZ_FOUNDATION_INTERNAL_HEADER static_assert(EZ_FOUNDATION_INTERNAL_HEADER_ALLOWED, "This is a internal header which may only be used when compiling ezFoundation");