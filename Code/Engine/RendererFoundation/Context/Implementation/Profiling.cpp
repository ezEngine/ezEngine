#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Context/Profiling.h>
#include <RendererFoundation/Context/Context.h>

ezProfilingScopeAndMarker::ezProfilingScopeAndMarker(ezGALContext* pGALContext, const char* szName, const char* szFunctionName)
  : ezProfilingScope(szName, szFunctionName)
  , m_pGALContext(pGALContext)
{
  m_pGALContext->PushMarker(m_szName);
}

ezProfilingScopeAndMarker::~ezProfilingScopeAndMarker()
{
  m_pGALContext->PopMarker();
}
