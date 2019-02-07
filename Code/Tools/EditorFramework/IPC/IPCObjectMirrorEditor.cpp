#include <EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>

ezIPCObjectMirrorEditor::ezIPCObjectMirrorEditor()
    : ezDocumentObjectMirror()
{
  m_pIPC = nullptr;
}

ezIPCObjectMirrorEditor::~ezIPCObjectMirrorEditor() {}

void ezIPCObjectMirrorEditor::SetIPC(ezEditorEngineConnection* pIPC)
{
  EZ_ASSERT_DEBUG(m_pContext == nullptr, "Need to call SetIPC before SetReceiver");
  m_pIPC = pIPC;
}

void ezIPCObjectMirrorEditor::ApplyOp(ezObjectChange& change)
{
  if (m_pManager)
  {
    SendOp(change);
  }
  else
  {
    EZ_REPORT_FAILURE("ezIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}

void ezIPCObjectMirrorEditor::SendOp(ezObjectChange& change)
{
  EZ_ASSERT_DEBUG(m_pIPC != nullptr, "Need to call SetIPC before SetReceiver");

  ezEntityMsgToEngine msg;
  msg.m_change = std::move(change);

  m_pIPC->SendMessage(&msg);
}
