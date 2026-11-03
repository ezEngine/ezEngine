#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezParticleEffectResourceHandle = ezTypedResourceHandle<class ezParticleEffectResource>;

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


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual void OnDeinitialize(const ezParticleModuleContext& ctx) const override;
  virtual bool GetReceiveParticleDeath() const override { return true; }
  virtual void OnParticleDeath(const ezParticleModuleContext& ctx, ezUInt64 uiElementIndex) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;
  virtual void ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  /// Returns the maximum effect radius for culling.
  ///
  /// This is an approximation based on the spawned effect's bounding radius.
  virtual float GetMaxParticleRadius(const ezParticleModuleContext& ctx, float fParticleSize) const override { return ctx.GetInstanceData<InstanceData>().m_fMaxEffectRadius; }

  ezHashedString m_sEffect;
  ezHashedString m_sSharedInstanceName; // to be removed

  ezParticleEffectResourceHandle m_hEffect;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    float m_fMaxEffectRadius = 1.0f;
    ezProcessingStream* m_pStreamPosition = nullptr;
    ezProcessingStream* m_pStreamEffectID = nullptr;
  };

  static void ClearEffects(const ezParticleModuleContext& ctx, bool bInterruptImmediately);
};
