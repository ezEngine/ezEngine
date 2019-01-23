#pragma once

#include <EnginePluginAssets/Plugin.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

class EZ_ENGINEPLUGINASSETS_DLL ezSkeletonContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonContext, ezEngineProcessDocumentContext);

public:
  ezSkeletonContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);

  ezGameObject* m_pGameObject = nullptr;
  ezSkeletonResourceHandle m_hMesh;
};


