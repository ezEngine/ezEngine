#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

/// \brief An object mirror that mirrors across IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another
/// one on the engine side as receiver.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezIPCObjectMirrorEngine : public ezDocumentObjectMirror
{
public:
  ezIPCObjectMirrorEngine();
  ~ezIPCObjectMirrorEngine();

  virtual void ApplyOp(ezObjectChange& change) override;
};
