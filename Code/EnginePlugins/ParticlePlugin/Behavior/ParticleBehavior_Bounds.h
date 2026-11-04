#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

/// Behavior that restricts particles to a box volume
///
/// Particles can be killed, teleported or bounced when leaving the box bounds.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Bounds final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Bounds, ezParticleBehavior);

public:
  ezParticleBehavior_Bounds();


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void QueryOptionalStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  ezVec3 m_vPositionOffset;
  ezVec3 m_vBoxExtents;
  ezEnum<ezParticleOutOfBoundsMode> m_OutOfBoundsMode;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamLastPosition;
  };
};
