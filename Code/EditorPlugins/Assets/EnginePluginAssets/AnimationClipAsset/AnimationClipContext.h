#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>

class EZ_ENGINEPLUGINASSETS_DLL ezAnimationClipContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipContext, ezEngineProcessDocumentContext);

public:
  ezAnimationClipContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);
  void SetPlaybackPosition(double pos);
  void ExtractRootMotionFromFeet();

  ezGameObject* m_pGameObject = nullptr;
  ezString m_sAnimatedMeshToUse;
  ezComponentHandle m_hAnimMeshComponent;
  ezComponentHandle m_hAnimControllerComponent;
};
