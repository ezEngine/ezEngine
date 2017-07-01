#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
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

  static void CreateWindowAndView();
  static void DestroyWindowAndView();

  static ezInt32 s_iWindowReferences;
  static ezViewHandle s_hView;
  static ezUInt32 s_uiActiveViewID;
  static ezUniquePtr<ezWindow> s_pCustomWindow;
};

