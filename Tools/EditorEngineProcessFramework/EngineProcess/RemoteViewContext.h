#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <System/Window/Window.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <Foundation/Types/UniquePtr.h>

class ezRemoteProcessWindow : public ezWindow
{
public:
  virtual void OnWindowMoveMessage(const ezInt32 newPosX, const ezInt32 newPosY) override;

  ezInt32 m_iWindowPosX = 0;
  ezInt32 m_iWindowPosY = 0;
};

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
  static ezUniquePtr<ezRemoteProcessWindow> s_pCustomWindow;
  static ezVec2I32 s_WindowPosition;
};

