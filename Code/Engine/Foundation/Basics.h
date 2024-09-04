#pragma once

#define EZ_INCLUDING_BASICS_H

// Very basic Preprocessor defines
#include <Foundation/Basics/PreprocessorUtils.h>

// Set all feature #defines to EZ_OFF
#include <Foundation/Basics/AllDefinesOff.h>

// General detection of the OS and hardware
#include <Foundation/Basics/Platform/DetectArchitecture.h>
#include <Foundation/Basics/Platform/DetectPlatform.h>

// Options by the user to override the build
#include <Foundation/UserConfig.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#    define EZ_FOUNDATION_DLL EZ_DECL_EXPORT
#    define EZ_FOUNDATION_DLL_FRIEND EZ_DECL_EXPORT_FRIEND
#  else
#    define EZ_FOUNDATION_DLL EZ_DECL_IMPORT
#    define EZ_FOUNDATION_DLL_FRIEND EZ_DECL_IMPORT_FRIEND
#  endif
#else
#  define EZ_FOUNDATION_DLL
#  define EZ_FOUNDATION_DLL_FRIEND
#endif

#include <Foundation/FoundationInternal.h>

// include the different headers for the supported platforms
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/Platform_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/Platform_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Linux/Platform_Linux.h>
#elif EZ_ENABLED(EZ_PLATFORM_WEB)
#  include <Foundation/Basics/Platform/Web/Platform_Web.h>
#else
#  error "Undefined platform!"
#endif

// include headers for the supported compilers
#include <Foundation/Basics/Compiler/Clang.h>
#include <Foundation/Basics/Compiler/GCC.h>
#include <Foundation/Basics/Compiler/MSVC.h>

// Here all the different features that each platform supports are declared.
#include <Foundation/Basics/Platform/PlatformFeatures.h>

// Include this last, it will ensure the previous includes have setup everything correctly
#include <Foundation/Basics/Platform/CheckDefinitions.h>

// Include common definitions and macros (e.g. static_assert)
#include <Foundation/Basics/Platform/Common.h>

// Include magic preprocessor macros
#include <Foundation/Basics/Platform/BlackMagic.h>

// Now declare all fundamental types
#include <Foundation/Types/Types.h>

// Type trait utilities
#include <Foundation/Types/TypeTraits.h>

// Assert macros should always be available
#include <Foundation/Basics/Assert.h>

// String formatting is needed by the asserts
#include <Foundation/Strings/FormatString.h>


class EZ_FOUNDATION_DLL ezFoundation
{
public:
  static ezAllocator* s_pDefaultAllocator;
  static ezAllocator* s_pAlignedAllocator;

  /// \brief The default allocator can be used for any kind of allocation if no alignment is required
  EZ_ALWAYS_INLINE static ezAllocator* GetDefaultAllocator()
  {
    if (s_bIsInitialized)
      return s_pDefaultAllocator;
    else // the default allocator is not yet set so we return the static allocator instead.
      return GetStaticsAllocator();
  }

  /// \brief The aligned allocator should be used for all allocations which need alignment
  EZ_ALWAYS_INLINE static ezAllocator* GetAlignedAllocator()
  {
    EZ_ASSERT_ALWAYS(s_pAlignedAllocator != nullptr,
      "ezFoundation must have been initialized before this function can be called."
      "This error can occur when you have a global variable or a static member variable that (indirectly) requires an allocator."
      "Check out the documentation for 'ezStaticsAllocatorWrapper' for more information about this issue.");
    return s_pAlignedAllocator;
  }

  /// \brief Returns the allocator that is used by global data and static members before the default allocator is created.
  static ezAllocator* GetStaticsAllocator();

private:
  friend class ezStartup;
  friend struct ezStaticsAllocatorWrapper;

  static void Initialize();
  static bool s_bIsInitialized;
};

#undef EZ_INCLUDING_BASICS_H
