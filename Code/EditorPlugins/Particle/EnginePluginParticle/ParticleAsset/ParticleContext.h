#pragma once

#include <EnginePluginParticle/EnginePluginParticleDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class ezParticleComponent;

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

class EZ_ENGINEPLUGINPARTICLE_DLL ezParticleContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleContext, ezEngineProcessDocumentContext);

public:
  ezParticleContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

  void RestartEffect();
  void SetAutoRestartEffect(bool loop);

private:
  ezParticleEffectResourceHandle m_hParticle;
  ezMeshResourceHandle m_hPreviewMeshResource;
  ezParticleComponent* m_pComponent;
};


