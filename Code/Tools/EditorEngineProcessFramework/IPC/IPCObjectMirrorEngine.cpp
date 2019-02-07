#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

ezIPCObjectMirrorEngine::ezIPCObjectMirrorEngine()
    : ezDocumentObjectMirror()
{
}

ezIPCObjectMirrorEngine::~ezIPCObjectMirrorEngine() {}

void ezIPCObjectMirrorEngine::ApplyOp(ezObjectChange& change)
{
  if (m_pContext)
  {
    ezDocumentObjectMirror::ApplyOp(change);
  }
  else
  {
    EZ_REPORT_FAILURE("ezIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}
