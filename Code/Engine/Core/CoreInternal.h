#pragma once

#ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#  define EZ_CORE_INTERNAL_HEADER_ALLOWED 1
#else
#  define EZ_CORE_INTERNAL_HEADER_ALLOWED 0
#endif

#define EZ_CORE_INTERNAL_HEADER \
  static_assert(EZ_CORE_INTERNAL_HEADER_ALLOWED, "This is an internal ez header. Please do not #include it directly.");
