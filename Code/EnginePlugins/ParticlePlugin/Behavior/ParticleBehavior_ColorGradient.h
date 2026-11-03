#pragma once

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that applies a color gradient to particles
///
/// The gradient can be sampled based on particle lifetime or speed.
/// The final color is multiplied by the tint color.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_ColorGradient final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_ColorGradient, ezParticleBehavior);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  // ************************************* PROPERTIES ***********************************

  ezEnum<ezParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  ezColor m_TintColor = ezColor::White;
  bool m_bApplyAlpha = true;
  ezEnum<ezGradientSource> m_GradientSource;
  ezColorGradient m_Gradient;
  ezColorGradientResourceHandle m_hSharedGradient;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamLifeTime = nullptr;
    ezProcessingStream* m_pStreamColor = nullptr;
    ezProcessingStream* m_pStreamVelocity = nullptr;
    ezColor m_InitColor;

    /// Staggered update: which particle index to update this frame (cycles from 0 to m_uiCurrentUpdateInterval-1)
    ezUInt8 m_uiFirstToUpdate = 0;
    /// Staggered update: only update every N-th particle per frame to reduce cost
    ezUInt8 m_uiCurrentUpdateInterval = 8;
  };
};
