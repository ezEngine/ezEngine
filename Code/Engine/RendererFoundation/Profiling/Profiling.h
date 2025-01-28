#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct GPUTimingScope;

/// Sets profiling marker and GPU timings for the current scope.
class EZ_RENDERERFOUNDATION_DLL ezProfilingScopeAndMarker : public ezProfilingScope
{
public:
  static GPUTimingScope* Start(ezGALCommandEncoder* pCommandEncoder, const char* szName);
  static void Stop(ezGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope);

  ezProfilingScopeAndMarker(ezGALCommandEncoder* pCommandEncoder, const char* szName);

  ~ezProfilingScopeAndMarker();

protected:
  ezGALCommandEncoder* m_pCommandEncoder;
  GPUTimingScope* m_pTimingScope;
};

#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)

/// \brief Profiles the current scope using the given name and also inserts a marker with the given command encoder.
#  define EZ_PROFILE_AND_MARKER(GALCommandEncoder, szName) ezProfilingScopeAndMarker EZ_PP_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(GALCommandEncoder, szName)

#else

#  define EZ_PROFILE_AND_MARKER(GALCommandEncoder, szName) /*empty*/

#endif
