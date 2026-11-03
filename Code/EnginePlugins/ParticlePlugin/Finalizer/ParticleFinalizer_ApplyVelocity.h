#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Moves particles along their velocity.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_ApplyVelocity final : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_ApplyVelocity, ezParticleFinalizer);

public:
  ezParticleFinalizer_ApplyVelocity();

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamVelocity;
  };
};
