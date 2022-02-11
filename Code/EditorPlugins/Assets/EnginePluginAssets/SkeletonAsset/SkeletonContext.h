#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Declarations.h>

class EZ_ENGINEPLUGINASSETS_DLL ezSkeletonContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonContext, ezEngineProcessDocumentContext);

public:
  ezSkeletonContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  ezSkeletonResourceHandle GetSkeleton() const { return m_hSkeleton; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);

  ezGameObject* m_pGameObject = nullptr;
  ezSkeletonResourceHandle m_hSkeleton;
  ezComponentHandle m_hSkeletonComponent;
  ezComponentHandle m_hPoseComponent;
};
