#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

/// \brief An object mirror that mirrors across IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another
/// one on the engine side as receiver.
class EZ_EDITORFRAMEWORK_DLL ezIPCObjectMirrorEditor : public ezDocumentObjectMirror
{
public:
  ezIPCObjectMirrorEditor();
  ~ezIPCObjectMirrorEditor();

  void SetIPC(ezEditorEngineConnection* pIPC);
  ezEditorEngineConnection* GetIPC();
  virtual void ApplyOp(ezObjectChange& change) override;

private:
  void SendOp(ezObjectChange& change);

  ezEditorEngineConnection* m_pIPC;
};
