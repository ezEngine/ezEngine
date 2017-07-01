#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

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
  static ezWindow s_CustomWindow;
};

