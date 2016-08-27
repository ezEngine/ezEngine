#pragma once

#include <EnginePluginParticle/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

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
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

  void RestartEffect();

private:
  ezParticleEffectResourceHandle m_hParticle;
  ezParticleComponent* m_pComponent;
};


