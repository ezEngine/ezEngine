#pragma once

#include <RendererFoundation/Basics.h>
#include <Foundation/Profiling/Profiling.h>

/// Sets profiling marker for the current scope.
/// Via a CVar GPU timing can be activated for the scopes as well.
class EZ_RENDERERFOUNDATION_DLL ezProfilingScopeAndMarker : public ezProfilingScope
{
public:
  ezProfilingScopeAndMarker(ezGALContext* pGALContext, const char* szName, const char* szFunctionName);

  ~ezProfilingScopeAndMarker();

protected:
  ezGALContext* m_pGALContext;
};

/// \brief Profiles the current scope using the given name and also inserts a marker with the given GALContext.
#define EZ_PROFILE_AND_MARKER(GALContext, szName) \
  ezProfilingScopeAndMarker EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(GALContext, szName, EZ_SOURCE_FUNCTION)
