#pragma once

// This should be defined by the compiler specific header
#ifdef NULL
  #undef NULL
#endif

#include <Foundation/Basics/Platform/DetectPlatform.h>
#include <Foundation/UserConfig.h>

// include the different headers for the supported platforms
#if EZ_PLATFORM_WINDOWS
  #include <Foundation/Basics/Platform/Win/Platform_win.h>
#elif EZ_PLATFORM_LINUX
  #include <Foundation/Basics/Platform/Linux/Platform_linux.h>
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
#include <Foundation/Basics/Types.h>

#ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
  #if (defined(BUILDSYSTEM_COMPILE_ENGINE_AS_DLL) && !defined(EZ_COMPILE_ENGINE_AS_DLL))
    #error "The Buildsystem is configured to build the Engine as a shared library, but EZ_COMPILE_ENGINE_AS_DLL is not defined in UserConfig.h"
  #endif
  #if (!defined(BUILDSYSTEM_COMPILE_ENGINE_AS_DLL) && defined(EZ_COMPILE_ENGINE_AS_DLL))
    #error "The Buildsystem is configured to build the Engine as a static library, but EZ_COMPILE_ENGINE_AS_DLL is defined in UserConfig.h"
  #endif
#endif

// Configure the DLL Import/Export Define
#if EZ_COMPILE_ENGINE_AS_DLL
  #ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
    #define EZ_FOUNDATION_DLL __declspec(dllexport)
    #define EZ_FOUNDATION_TEMPLATE
  #else
    #define EZ_FOUNDATION_DLL __declspec(dllimport)
    #define EZ_FOUNDATION_TEMPLATE extern
  #endif
#else
  #define EZ_FOUNDATION_DLL
  #define EZ_FOUNDATION_TEMPLATE
#endif

// Finally include the rest of basics
#include <Foundation/Basics/Assert.h>
#include <Foundation/Basics/TypeTraits.h>

#include <Foundation/Memory/IAllocator.h>

#include <Foundation/Configuration/StartupDeclarations.h>

class EZ_FOUNDATION_DLL ezFoundation
{
public:
  struct EZ_FOUNDATION_DLL Config
  {
    Config();

    ezIAllocator* pBaseAllocator;
    ezIAllocator* pDebugAllocator;
    ezIAllocator* pDefaultAllocator;
    ezIAllocator* pAlignedAllocator;    
  };

  static Config s_Config;

  /// The base allocator should only be used to allocate other allocators
  EZ_FORCE_INLINE static ezIAllocator* GetBaseAllocator()
  { 
    EZ_ASSERT_API(s_pBaseAllocator != NULL, "ezFoundation must have been initialized before this function can be called. This error can occur when you have a global variable or a static member variable that (indirectly) requires an allocator. Check out the documentation for 'ezStatic' for more information about this issue."); 
    return s_pBaseAllocator;
  }

  /// The debug allocator should be used to allocate debug information to seperate these from regular memory
  EZ_FORCE_INLINE static ezIAllocator* GetDebugAllocator()
  { 
    EZ_ASSERT_API(s_pDebugAllocator != NULL, "ezFoundation must have been initialized before this function can be called. This error can occur when you have a global variable or a static member variable that (indirectly) requires an allocator. Check out the documentation for 'ezStatic' for more information about this issue."); 
    return s_pDebugAllocator;
  }

  /// The default allocator can be used for any kind of allocation if no alignment is required
  EZ_FORCE_INLINE static ezIAllocator* GetDefaultAllocator()
  { 
    EZ_ASSERT_API(s_pDefaultAllocator != NULL, "ezFoundation must have been initialized before this function can be called. This error can occur when you have a global variable or a static member variable that (indirectly) requires an allocator. Check out the documentation for 'ezStatic' for more information about this issue."); 
    return s_pDefaultAllocator;
  }

  /// The aligned allocator should be used for any allocations which need an alignment
  EZ_FORCE_INLINE static ezIAllocator* GetAlignedAllocator()
  { 
    EZ_ASSERT_API(s_pAlignedAllocator != NULL, "ezFoundation must have been initialized before this function can be called. This error can occur when you have a global variable or a static member variable that (indirectly) requires an allocator. Check out the documentation for 'ezStatic' for more information about this issue."); 
    return s_pAlignedAllocator;
  }

private:
  friend class ezStartup;

  static void Initialize();
  static void Shutdown();


  // ezStatic must be able to call 'PushStaticAllocator' and 'PopStaticAllocator'
  template <typename T>
  friend class ezStatic;

  friend struct ezStaticAllocatorWrapper;

  /// Returns the allocator that is used to by global data and static members.
  static ezIAllocator* GetStaticAllocator();

  /// Temporarily stores the current allocators and sets the static allocator for all of them.
  static void PushStaticAllocator()
  {
    s_pBaseAllocatorTemp    = s_pBaseAllocator;
    s_pDebugAllocatorTemp   = s_pDebugAllocator;
    s_pDefaultAllocatorTemp = s_pDefaultAllocator;
    s_pAlignedAllocatorTemp = s_pAlignedAllocator;

    ezIAllocator* pAllocator = GetStaticAllocator();

    s_pBaseAllocator    = pAllocator;
    s_pDebugAllocator   = pAllocator;
    s_pDefaultAllocator = pAllocator;
    s_pAlignedAllocator = pAllocator;
  }

  /// Resets all allocators back to their state before PushStaticAllocator was executed.
  static void PopStaticAllocator()
  {
    s_pBaseAllocator    = s_pBaseAllocatorTemp;
    s_pDebugAllocator   = s_pDebugAllocatorTemp;
    s_pDefaultAllocator = s_pDefaultAllocatorTemp;
    s_pAlignedAllocator = s_pAlignedAllocatorTemp;
  }

  static bool s_bIsInitialized;

  static bool s_bOwnsBaseAllocator;

  static ezIAllocator* s_pStaticAllocator;

  static ezIAllocator* s_pBaseAllocator;
  static ezIAllocator* s_pDebugAllocator;
  static ezIAllocator* s_pDefaultAllocator;
  static ezIAllocator* s_pAlignedAllocator;

  static ezIAllocator* s_pBaseAllocatorTemp;
  static ezIAllocator* s_pDebugAllocatorTemp;
  static ezIAllocator* s_pDefaultAllocatorTemp;
  static ezIAllocator* s_pAlignedAllocatorTemp;
};

struct ezNullAllocatorWrapper
{
  EZ_FORCE_INLINE static ezIAllocator* GetAllocator()
  {
    EZ_REPORT_FAILURE("This method should never be called");
    return NULL;
  }
};

struct ezDefaultAllocatorWrapper
{
  EZ_FORCE_INLINE static ezIAllocator* GetAllocator()
  {
    return ezFoundation::GetDefaultAllocator();
  }
};

struct ezStaticAllocatorWrapper
{
  EZ_FORCE_INLINE static ezIAllocator* GetAllocator()
  {
    return ezFoundation::GetStaticAllocator();
  }
};

#define EZ_INCLUDE_STATIC_H
#include <Foundation/Basics/Static.h>
#undef EZ_INCLUDE_STATIC_H