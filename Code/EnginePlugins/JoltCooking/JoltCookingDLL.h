#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    ifdef BUILDSYSTEM_BUILDING_JOLTCOOKING_LIB
#      define EZ_JOLTCOOKING_DLL __declspec(dllexport)
#    else
#      define EZ_JOLTCOOKING_DLL __declspec(dllimport)
#    endif
#  else
#    define EZ_JOLTCOOKING_DLL __attribute__((visibility("default")))
#  endif
#else
#  define EZ_JOLTCOOKING_DLL
#endif