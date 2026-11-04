#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that fades particle alpha over their lifetime
///
/// Uses a power curve to control the fade speed.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_FadeOut final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_FadeOut, ezParticleBehavior);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  // ************************************* PROPERTIES ***********************************

  float m_fStartAlpha = 1.0f;
  float m_fExponent = 1.0f;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamLifeTime = nullptr;
    ezProcessingStream* m_pStreamColor = nullptr;
    ezUInt8 m_uiFirstToUpdate = 0;
    ezUInt8 m_uiCurrentUpdateInterval = 2;
  };
};
