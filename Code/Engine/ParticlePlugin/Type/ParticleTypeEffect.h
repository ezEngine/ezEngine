#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffectFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffectFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sEffect;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffect : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffect, ezParticleType);

public:
  ~ezParticleTypeEffect();

  virtual void CreateRequiredStreams() override;

  ezParticleEffectResourceHandle m_hEffect;

  virtual void Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass) const override {}


  virtual void AfterPropertiesConfigured(bool bFirstTime) override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);

  ezStream* m_pStreamPosition;
  ezStream* m_pStreamEffectID;

};


