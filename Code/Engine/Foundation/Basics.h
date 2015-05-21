#pragma once

#include <Foundation/Basics/PreprocessorUtils.h>
#include <Foundation/Basics/AllDefinesOff.h>
#include <Foundation/Basics/Platform/DetectPlatform.h>
#include <Foundation/UserConfig.h>

// include the different headers for the supported platforms
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Basics/Platform/Win/Platform_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  #include <Foundation/Basics/Platform/OSX/Platform_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Basics/Platform/Linux/Platform_Linux.h>
#else
  #error "Undefined platform!"
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
  #if BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && EZ_DISABLED(EZ_COMPILE_ENGINE_AS_DLL)
    #error "The Buildsystem is configured to build the Engine as a shared library, but EZ_COMPILE_ENGINE_AS_DLL is not defined in UserConfig.h"
  #endif
  #if !BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
    #error "The Buildsystem is configured to build the Engine as a static library, but EZ_COMPILE_ENGINE_AS_DLL is defined in UserConfig.h"
  #endif
#endif

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
    #define EZ_FOUNDATION_DLL __declspec(dllexport)
  #else
    #define EZ_FOUNDATION_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_FOUNDATION_DLL
#endif

// Finally include the rest of basics
#include <Foundation/Basics/Assert.h>
#include <Foundation/Types/TypeTraits.h>

#include <Foundation/Memory/AllocatorBase.h>

#include <Foundation/Configuration/StaticSubSystem.h>

class EZ_FOUNDATION_DLL ezFoundation
{
public:
  static ezAllocatorBase* s_pDefaultAllocator;
  static ezAllocatorBase* s_pAlignedAllocator;

  /// \brief The default allocator can be used for any kind of allocation if no alignment is required
  EZ_FORCE_INLINE static ezAllocatorBase* GetDefaultAllocator()
  { 
    if (s_bIsInitialized)
      return s_pDefaultAllocator;
    else // the default allocator is not yet set so we return the static allocator instead.
      return GetStaticAllocator();
  }

  /// \brief The aligned allocator should be used for any allocations which need an alignment
  EZ_FORCE_INLINE static ezAllocatorBase* GetAlignedAllocator()
  { 
    EZ_ASSERT_RELEASE(s_pAlignedAllocator != nullptr, "ezFoundation must have been initialized before this function can be called. This error can occur when you have a global variable or a static member variable that (indirectly) requires an allocator. Check out the documentation for 'ezStatic' for more information about this issue."); 
    return s_pAlignedAllocator;
  }

private:
  friend class ezStartup;
  friend struct ezStaticAllocatorWrapper;

  static void Initialize();

  /// \brief Returns the allocator that is used to by global data and static members before the default allocator is created.
  static ezAllocatorBase* GetStaticAllocator();

  static bool s_bIsInitialized;
};

