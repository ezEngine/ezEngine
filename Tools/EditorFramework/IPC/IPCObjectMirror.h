#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

/// \brief An object mirror that mirrors accross IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another
/// one on the engine side as receiver.
class EZ_EDITORFRAMEWORK_DLL ezIPCObjectMirror : public ezDocumentObjectMirror
{
public:
  ezIPCObjectMirror();
  ~ezIPCObjectMirror();

  void SetIPC(ezEditorEngineConnection* pIPC);
  virtual void ApplyOp(ezObjectChange& change) override;

private:
  void SendOp(ezObjectChange& change);

  ezEditorEngineConnection* m_pIPC;
};