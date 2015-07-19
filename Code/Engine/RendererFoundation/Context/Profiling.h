#pragma once

#include <RendererFoundation/Basics.h>
#include <Foundation/Profiling/Profiling.h>

class EZ_RENDERERFOUNDATION_DLL ezProfilingScopeAndMarker : public ezProfilingScope
{
public:
  ezProfilingScopeAndMarker(ezGALContext* pGALContext, const ezProfilingId& id, const char* szFileName,
    const char* szFunctionName, ezUInt32 uiLineNumber);

  ~ezProfilingScopeAndMarker();

protected:
  ezGALContext* m_pGALContext;
};

/// \brief Profiles the current scope using the given profiling Id and also inserts a marker with the given GALContext.
#define EZ_PROFILE_AND_MARKER(GALContext, Id) \
  ezProfilingScopeAndMarker EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(GALContext, Id, \
    EZ_SOURCE_FILE, EZ_SOURCE_FUNCTION, EZ_SOURCE_LINE)
