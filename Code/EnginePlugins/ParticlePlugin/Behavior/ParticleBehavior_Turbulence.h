#pragma once

#include <Foundation/SimdMath/SimdNoise.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Applies a turbulent force to particles using 3D curl noise.
///
/// Curl noise produces divergence-free flow fields that look natural and avoid
/// compression artifacts. Useful for smoke, fire, and magical effects.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Turbulence final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Turbulence, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Turbulence();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;
  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  float m_fStrength = 2.0f;
  float m_fFrequency = 1.0f;
  ezVec3 m_vScrollSpeed = ezVec3::MakeZero();
  ezUInt8 m_uiOctaves = 1;
  bool m_bAffectVelocity = true;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Turbulence final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Turbulence, ezParticleBehavior);

public:
  float m_fStrength = 2.0f;
  float m_fFrequency = 1.0f;
  ezVec3 m_vScrollSpeed = ezVec3::MakeZero();
  ezUInt8 m_uiOctaves = 1;
  bool m_bAffectVelocity = true;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;

  ezSimdPerlinNoise m_Noise{12345};
  float m_fTotalTime = 0.0f;
};
