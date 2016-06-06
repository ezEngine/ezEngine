#include <PCH.h>
#include <EditorFramework/IPC/IPCObjectMirror.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>

ezIPCObjectMirror::ezIPCObjectMirror() : ezDocumentObjectMirror()
{
  m_pIPC = nullptr;
}

ezIPCObjectMirror::~ezIPCObjectMirror()
{
}

void ezIPCObjectMirror::SetIPC(ezEditorEngineConnection* pIPC)
{
  EZ_ASSERT_DEBUG(m_pContext == nullptr, "Need to call SetIPC before SetReceiver");
  m_pIPC = pIPC;

}

void ezIPCObjectMirror::ApplyOp(ezObjectChange& change)
{
  if (m_pManager)
  {
    SendOp(change);
  }
  else if (m_pContext)
  {
    ezDocumentObjectMirror::ApplyOp(change);
  }
  else
  {
    EZ_REPORT_FAILURE("ezIPCObjectMirror not set up for sender nor receiver!");
  }
}

void ezIPCObjectMirror::SendOp(ezObjectChange& change)
{
  EZ_ASSERT_DEBUG(m_pIPC != nullptr, "Need to call SetIPC before SetReceiver");

  ezEntityMsgToEngine msg;
  msg.m_change = std::move(change);

  m_pIPC->SendMessage(&msg);
}
