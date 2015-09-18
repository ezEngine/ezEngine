#pragma once

#include <EnginePluginScene/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

class ezObjectSelectionMsgToEngine;

class EZ_ENGINEPLUGINSCENE_DLL ezSceneContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneContext);

public:

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() {}

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  void HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg);

  ezDeque<ezGameObjectHandle> m_Selection;
};


