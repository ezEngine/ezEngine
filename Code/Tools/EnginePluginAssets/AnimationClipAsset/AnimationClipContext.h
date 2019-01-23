#pragma once

#include <EnginePluginAssets/Plugin.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <RendererCore/Declarations.h>

class EZ_ENGINEPLUGINASSETS_DLL ezAnimationClipContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipContext, ezEngineProcessDocumentContext);

public:
  ezAnimationClipContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);

  ezGameObject* m_pGameObject = nullptr;
  //ezAnimationClipResourceHandle m_hMesh;
};


