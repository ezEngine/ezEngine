#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Stores the position of each particle in the 'LastPosition' stream at the start of the update, before anything moves the particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_LastPosition final : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_LastPosition, ezParticleFinalizer);

public:
  ezParticleFinalizer_LastPosition();

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamLastPosition;
  };
};
