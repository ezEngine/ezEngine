#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that sets particle velocity within a cone
///
/// Velocities point along the local Z-axis with random deviation within the cone angle.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_VelocityCone final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_VelocityCone, ezParticleInitializer);

public:
  ezParticleInitializer_VelocityCone();


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

public:
  ezAngle m_Angle;
  ezVarianceTypeFloat m_Speed;
  ezHashedString m_sSpeedScaleParameter;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamVelocity;
  };
};
