#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <System/Window/Window.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <Foundation/Types/UniquePtr.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezRemoteEngineProcessViewContext : public ezEngineProcessViewContext
{
public:
  ezRemoteEngineProcessViewContext(ezEngineProcessDocumentContext* pContext);
  ~ezRemoteEngineProcessViewContext();

protected:
  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg) override;
  virtual ezViewHandle CreateView() override;

  static ezUInt32 s_uiActiveViewID;
  static ezRemoteEngineProcessViewContext* s_pActiveRemoteViewContext;
};

