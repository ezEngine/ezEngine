#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffectFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffectFactory, ezParticleTypeFactory);

public:
  ezParticleTypeEffectFactory();
  ~ezParticleTypeEffectFactory();

  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sEffect;
  ezString m_sSharedInstanceName; // to be removed
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffect final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffect, ezParticleType);

public:
  ezParticleTypeEffect();
  ~ezParticleTypeEffect();

  ezParticleEffectResourceHandle m_hEffect;
  //ezString m_sSharedInstanceName;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override {}

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return m_fMaxEffectRadius; }

protected:
  friend class ezParticleTypeEffectFactory;

  virtual void OnReset() override;
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);
  void ClearEffects(bool bInterruptImmediately);

  float m_fMaxEffectRadius = 1.0f;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamEffectID = nullptr;
};


