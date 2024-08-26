#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDX11_LIB
#    define EZ_RENDERERDX11_DLL EZ_DECL_EXPORT
#  else
#    define EZ_RENDERERDX11_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_RENDERERDX11_DLL
#endif


#define EZ_GAL_DX11_RELEASE(d3dobj) \
  do                                \
  {                                 \
    if ((d3dobj) != nullptr)        \
    {                               \
      (d3dobj)->Release();          \
      (d3dobj) = nullptr;           \
    }                               \
  } while (0)
