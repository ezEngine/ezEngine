#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffectFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffectFactory, ezParticleTypeFactory);

public:
  ezParticleTypeEffectFactory();

  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sEffect;
  ezUInt64 m_uiRandomSeed;
  ezString m_sSharedInstanceName;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffect : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffect, ezParticleType);

public:
  ezParticleTypeEffect();
  ~ezParticleTypeEffect();

  ezParticleEffectResourceHandle m_hEffect;
  ezUInt64 m_uiRandomSeed;
  ezString m_sSharedInstanceName;

  virtual void CreateRequiredStreams() override;
  virtual void AfterPropertiesConfigured(bool bFirstTime) override;
  virtual void ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override {}

protected:
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamEffectID;
};


