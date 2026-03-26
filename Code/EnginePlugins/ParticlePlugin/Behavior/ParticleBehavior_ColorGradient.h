#pragma once

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that applies a color gradient to particles
///
/// The gradient can be sampled based on particle lifetime or speed.
/// The final color is multiplied by the tint color.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_ColorGradient final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_ColorGradient, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  // ************************************* PROPERTIES ***********************************

  ezEnum<ezParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  ezColor m_TintColor = ezColor::White;
  bool m_bApplyAlpha = true;
  ezEnum<ezGradientSource> m_GradientSource;
  ezColorGradient m_Gradient;
  ezColorGradientResourceHandle m_hSharedGradient;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_ColorGradient final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_ColorGradient, ezParticleBehavior);

public:
  const ezColorGradient* m_pGradient = nullptr;
  ezEnum<ezParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  ezColor m_TintColor;
  bool m_bApplyAlpha = true;

  virtual void CreateRequiredStreams() override;

protected:
  friend class ezParticleBehaviorFactory_ColorGradient;

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
  ezColor m_InitColor;

  /// Staggered update: which particle index to update this frame (cycles from 0 to m_uiCurrentUpdateInterval-1)
  ezUInt8 m_uiFirstToUpdate = 0;
  /// Staggered update: only update every N-th particle per frame to reduce cost
  ezUInt8 m_uiCurrentUpdateInterval = 8;
};
