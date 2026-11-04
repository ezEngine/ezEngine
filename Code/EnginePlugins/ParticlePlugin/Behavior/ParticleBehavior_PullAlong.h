#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

/// Behavior that pulls particles along when the effect moves
///
/// Useful for effects attached to moving objects where particles should
/// follow the movement to some degree rather than staying in world space.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_PullAlong final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_PullAlong, ezParticleBehavior);

public:
  ezParticleBehavior_PullAlong();


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  float m_fStrength; ///< How much particles follow the effect movement (0-1 range)

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    bool m_bFirstTime;
    ezVec3 m_vLastEmitterPosition;
    ezVec3 m_vApplyPull;
    ezProcessingStream* m_pStreamPosition;
  };
};
