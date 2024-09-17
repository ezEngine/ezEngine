#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERWEBGPU_LIB
#    define EZ_RENDERERWEBGPU_DLL EZ_DECL_EXPORT
#  else
#    define EZ_RENDERERWEBGPU_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_RENDERERWEBGPU_DLL
#endif

EZ_RENDERERWEBGPU_DLL void EnableWebGPUTrace(bool bEnable);
EZ_RENDERERWEBGPU_DLL bool IsWebGPUTraceEnabled();

#define EZ_WEBGPU_TRACE()                                            \
  if (IsWebGPUTraceEnabled())                                             \
  {                                                                  \
    ezLog::Info("TRACE: {}:{}", EZ_SOURCE_FUNCTION, EZ_SOURCE_LINE); \
  }

#define EZ_WEBGPU_TRACE_TEXT(text)                                              \
  if (IsWebGPUTraceEnabled())                                                        \
  {                                                                             \
    ezLog::Info("TRACE: {}:{} - {}", EZ_SOURCE_FUNCTION, EZ_SOURCE_LINE, text); \
  }
