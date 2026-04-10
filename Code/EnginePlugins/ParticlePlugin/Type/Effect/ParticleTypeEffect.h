#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezParticleEffectResourceHandle = ezTypedResourceHandle<class ezParticleEffectResource>;

/// Factory for creating effect particle types.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffectFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffectFactory, ezParticleTypeFactory);

public:
  ezParticleTypeEffectFactory();
  ~ezParticleTypeEffectFactory();

  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  ezString m_sEffect;
  ezString m_sSharedInstanceName; // to be removed
};

/// Spawns nested particle effects at each particle position.
///
/// Each particle spawns an independent particle effect instance at its position.
/// The spawned effects are automatically cleaned up when the parent particle dies.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeEffect final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeEffect, ezParticleType);

public:
  ezParticleTypeEffect();
  ~ezParticleTypeEffect();

  ezParticleEffectResourceHandle m_hEffect;
  // ezString m_sSharedInstanceName;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  /// Returns the maximum effect radius for culling.
  ///
  /// This is an approximation based on the spawned effect's bounding radius.
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
