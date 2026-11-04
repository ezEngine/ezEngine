#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Updates particle age and removes particles that have exceeded their lifetime.
///
/// Each frame, the remaining lifetime is decreased by the time delta. When a particle's
/// lifetime reaches zero, it is removed from the system. If an on-death event is configured,
/// the event is triggered at the particle's position with its velocity as the direction.
/// The lifetime is initialized with optional variance and can be scaled by an effect parameter.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_Age final : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_Age, ezParticleFinalizer);

public:
  ezParticleFinalizer_Age();

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual bool GetReceiveParticleDeath() const override { return !m_sOnDeathEvent.IsEmpty(); }
  virtual void OnParticleDeath(const ezParticleModuleContext& ctx, ezUInt64 uiElementIndex) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  ezVarianceTypeTime m_LifeTime;
  ezHashedString m_sOnDeathEvent;
  ezHashedString m_sLifeScaleParameter;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamLifeTime;
    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamVelocity;
  };
};
