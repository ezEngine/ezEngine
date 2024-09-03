#pragma once

#ifdef BUILDSYSTEM_ENABLE_GLFW_SUPPORT
#  define EZ_SUPPORTS_GLFW EZ_ON
#else
#  define EZ_SUPPORTS_GLFW EZ_OFF
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/PlatformFeatures_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/PlatformFeatures_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/PlatformFeatures_Linux.h>
#elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/PlatformFeatures_Android.h>
#elif EZ_ENABLED(EZ_PLATFORM_WEB)
#  include <Foundation/Basics/Platform/Web/PlatformFeatures_Web.h>
#else
#  error "Undefined platform!"
#endif

// now check that the defines for each feature are set (either to 1 or 0, but they must be defined)

#ifndef EZ_SUPPORTS_FILE_ITERATORS
#  error "EZ_SUPPORTS_FILE_ITERATORS is not defined."
#endif

#ifndef EZ_USE_POSIX_FILE_API
#  error "EZ_USE_POSIX_FILE_API is not defined."
#endif

#ifndef EZ_SUPPORTS_FILE_STATS
#  error "EZ_SUPPORTS_FILE_STATS is not defined."
#endif

#ifndef EZ_SUPPORTS_MEMORY_MAPPED_FILE
#  error "EZ_SUPPORTS_MEMORY_MAPPED_FILE is not defined."
#endif

#ifndef EZ_SUPPORTS_SHARED_MEMORY
#  error "EZ_SUPPORTS_SHARED_MEMORY is not defined."
#endif

#ifndef EZ_SUPPORTS_DYNAMIC_PLUGINS
#  error "EZ_SUPPORTS_DYNAMIC_PLUGINS is not defined."
#endif

#ifndef EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#  error "EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS is not defined."
#endif

#ifndef EZ_SUPPORTS_CASE_INSENSITIVE_PATHS
#  error "EZ_SUPPORTS_CASE_INSENSITIVE_PATHS is not defined."
#endif

#ifndef EZ_SUPPORTS_LONG_PATHS
#  error "EZ_SUPPORTS_LONG_PATHS is not defined."
#endif
