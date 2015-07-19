#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Context/Profiling.h>
#include <RendererFoundation/Context/Context.h>

ezProfilingScopeAndMarker::ezProfilingScopeAndMarker(ezGALContext* pGALContext,
  const ezProfilingId& id, const char* szFileName, const char* szFunctionName, ezUInt32 uiLineNumber)
  : ezProfilingScope(id, szFileName, szFunctionName, uiLineNumber)
  , m_pGALContext(pGALContext)
{
  m_pGALContext->PushMarker(m_szName);
}

ezProfilingScopeAndMarker::~ezProfilingScopeAndMarker()
{
  m_pGALContext->PopMarker();
}
