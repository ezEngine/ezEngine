#pragma once

#define EZ_INCLUDING_BASICS_H

#include <Foundation/Basics/PreprocessorUtils.h>

#include <Foundation/Basics/AllDefinesOff.h>

#include <Foundation/Basics/Platform/DetectArchitecture.h>
#include <Foundation/Basics/Platform/DetectPlatform.h>

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
#else
#  error "Undefined platform!"
#endif

// Here all the different features that each platform supports are declared.
#include <Foundation/Basics/Platform/PlatformFeatures.h>

// Include this last, it will ensure the previous includes have setup everything correctly
#include <Foundation/Basics/Platform/CheckDefinitions.h>

// Include common definitions and macros (e.g. EZ_CHECK_AT_COMPILETIME)
#include <Foundation/Basics/Platform/Common.h>

// Include magic preprocessor macros
#include <Foundation/Basics/Platform/BlackMagic.h>

// Now declare all fundamental types
#include <Foundation/Types/Types.h>

#ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#  if BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && EZ_DISABLED(EZ_COMPILE_ENGINE_AS_DLL)
#    error "The Buildsystem is configured to build the Engine as a shared library, but EZ_COMPILE_ENGINE_AS_DLL is not defined in UserConfig.h"
#  endif
#  if !BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#    error "The Buildsystem is configured to build the Engine as a static library, but EZ_COMPILE_ENGINE_AS_DLL is defined in UserConfig.h"
#  endif
#endif

// Finally include the rest of basics
#include <Foundation/Basics/Assert.h>

#include <Foundation/Types/TypeTraits.h>

#include <Foundation/Strings/UnicodeUtils.h>

#include <Foundation/Strings/StringUtils.h>

#include <Foundation/Strings/Implementation/StringIterator.h>

#include <Foundation/Strings/StringView.h>

#include <Foundation/Strings/Implementation/FormatStringArgs.h>

#include <Foundation/Strings/FormatString.h>


class ezAllocator;

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
    EZ_ASSERT_RELEASE(s_pAlignedAllocator != nullptr, "ezFoundation must have been initialized before this function can be called. This "
                                                      "error can occur when you have a global variable or a static member variable that "
                                                      "(indirectly) requires an allocator. Check out the documentation for 'ezStatic' for "
                                                      "more information about this issue.");
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
