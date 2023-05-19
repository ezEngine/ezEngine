#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>

ezIPCObjectMirrorEditor::ezIPCObjectMirrorEditor()
  : ezDocumentObjectMirror()
{
  m_pIPC = nullptr;
}

ezIPCObjectMirrorEditor::~ezIPCObjectMirrorEditor() = default;

void ezIPCObjectMirrorEditor::SetIPC(ezEditorEngineConnection* pIPC)
{
  EZ_ASSERT_DEBUG(m_pContext == nullptr, "Need to call SetIPC before SetReceiver");
  m_pIPC = pIPC;
}

ezEditorEngineConnection* ezIPCObjectMirrorEditor::GetIPC()
{
  return m_pIPC;
}

void ezIPCObjectMirrorEditor::ApplyOp(ezObjectChange& ref_change)
{
  if (m_pManager)
  {
    SendOp(ref_change);
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
