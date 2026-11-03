#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Computes the bounding volume for the entire particle system.
///
/// Calculates a bounding box-sphere from all particle positions and optionally considers
/// particle sizes to determine the maximum extent. The computed volume is used for culling
/// and rendering optimizations. If no particles exist, the process is skipped.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_Volume final : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_Volume, ezParticleFinalizer);

public:
  ezParticleFinalizer_Volume();
  ~ezParticleFinalizer_Volume();

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void QueryOptionalStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    const ezProcessingStream* m_pStreamSize;
  };
};
