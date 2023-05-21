#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

ezIPCObjectMirrorEngine::ezIPCObjectMirrorEngine()
  : ezDocumentObjectMirror()
{
}

ezIPCObjectMirrorEngine::~ezIPCObjectMirrorEngine() = default;

void ezIPCObjectMirrorEngine::ApplyOp(ezObjectChange& inout_change)
{
  if (m_pContext)
  {
    ezDocumentObjectMirror::ApplyOp(inout_change);
  }
  else
  {
    EZ_REPORT_FAILURE("ezIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}
