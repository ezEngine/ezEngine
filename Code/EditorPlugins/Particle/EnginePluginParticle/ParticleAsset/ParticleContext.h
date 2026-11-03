#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginParticle/EnginePluginParticleDLL.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class ezParticleComponent;

using ezParticleEffectResourceHandle = ezTypedResourceHandle<class ezParticleEffectResource>;

class EZ_ENGINEPLUGINPARTICLE_DLL ezParticleContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleContext, ezEngineProcessDocumentContext);

public:
  ezParticleContext();
  ~ezParticleContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual void OnThumbnailViewContextRequested() override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;
  virtual void UpdateDocumentContext() override;

  void RestartEffect();
  void SetAutoRestartEffect(bool loop);

  /// Shows the first mesh that a mesh position initializer of the effect references, so that one can see where the particles spawn.
  /// Owner meshes ('UseOwnerMesh') can't be shown, the preview has no owner mesh.
  void UpdatePreviewMesh();

private:
  ezBoundingBoxSphere m_ThumbnailBoundingVolume;
  ezParticleEffectResourceHandle m_hParticle;
  ezMeshResourceHandle m_hPreviewMeshResource;
  ezParticleComponent* m_pComponent = nullptr;
  ezComponentHandle m_hPreviewMeshComponent;
  ezUInt32 m_uiPreviewMeshEffectChangeCounter = 0xFFFFFFFF; ///< Resource change counter of the effect when the preview mesh was set.
};
