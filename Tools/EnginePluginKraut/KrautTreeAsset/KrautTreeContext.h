#pragma once

#include <EnginePluginKraut/Plugin.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <KrautPlugin/KrautTreeComponent.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;
class ezGameState;

class EZ_ENGINEPLUGINKRAUT_DLL ezKrautTreeContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeContext, ezEngineProcessDocumentContext);

public:
  ezKrautTreeContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  const ezKrautTreeResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);

  ezGameObject* m_pMainObject;
  ezKrautTreeResourceHandle m_hMainResource;
};


