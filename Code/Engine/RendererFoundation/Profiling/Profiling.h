#pragma once

#include <RendererFoundation/Basics.h>
#include <Foundation/Profiling/Profiling.h>

/// Sets profiling marker and GPU timings for the current scope.
class EZ_RENDERERFOUNDATION_DLL ezProfilingScopeAndMarker : public ezProfilingScope
{
public:
  ezProfilingScopeAndMarker(ezGALContext* pGALContext, const char* szName);

  ~ezProfilingScopeAndMarker();

protected:
  ezGALContext* m_pGALContext;
  struct GPUTimingScope* m_pTimingScope;
};

/// \brief Profiles the current scope using the given name and also inserts a marker with the given GALContext.
#define EZ_PROFILE_AND_MARKER(GALContext, szName) \
  ezProfilingScopeAndMarker EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(GALContext, szName)

