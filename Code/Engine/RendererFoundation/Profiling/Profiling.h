#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct GPUTimingScope;

  /// Sets profiling marker and GPU timings for the current scope.
class EZ_RENDERERFOUNDATION_DLL ezProfilingScopeAndMarker : public ezProfilingScope
{
public:
  static GPUTimingScope* Start(ezGALCommandEncoder* pCommandEncoder, const char* szName);
  static void Stop(ezGALCommandEncoder* pCommandEncoder, GPUTimingScope*& pTimingScope);

  ezProfilingScopeAndMarker(ezGALCommandEncoder* pCommandEncoder, const char* szName);

  ~ezProfilingScopeAndMarker();

protected:
  ezGALCommandEncoder* m_pCommandEncoder;
  GPUTimingScope* m_pTimingScope;
};

#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)

/// \brief Profiles the current scope using the given name and also inserts a marker with the given GALContext.
#  define EZ_PROFILE_AND_MARKER(GALContext, szName) ezProfilingScopeAndMarker EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(GALContext, szName)

#else

#  define EZ_PROFILE_AND_MARKER(GALContext, szName) /*empty*/

#endif
