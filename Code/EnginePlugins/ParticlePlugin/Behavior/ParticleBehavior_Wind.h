#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Types/UniquePtr.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezWindWorldModuleInterface;

/// Behavior that applies wind forces to particles
///
/// Samples the wind of the world (ezWindWorldModuleInterface) on a grid that covers the bounding volume of the effect,
/// once per frame in UpdateFromWorld(). The grid resolution is set on the effect (see ezParticleEffectInstance::GetNumWindSamples()).
/// Particles move by the trilinearly interpolated wind at their position.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Wind final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Wind, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Wind();
  ~ezParticleBehaviorFactory_Wind();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fWindInfluence = 1.0f;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Wind final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Wind, ezParticleBehavior);

public:
  ezParticleBehavior_Wind();
  ~ezParticleBehavior_Wind();

  virtual void CreateRequiredStreams() override;
  virtual void OnFinalize() override;
  virtual void UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject) override;

  float m_fWindInfluence = 1.0f;

protected:
  friend class ezParticleBehaviorFactory_Wind;

  virtual void Process(ezUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) override;

  /// Returns the wind at the given position. Zero, until the grid was sampled once.
  ezSimdVec4f GetWindAt(const ezSimdVec4f& vPosition) const;

  ezProcessingStream* m_pStreamPosition = nullptr;

  struct WindSampleGrid
  {
    ezSimdVec4f m_vMinPos;
    ezSimdVec4f m_vMaxPos;
    ezSimdVec4f m_vInvCellSize;
    ezSmallArray<ezSimdVec4f, 16, ezAlignedAllocatorWrapper> m_Samples;
  };

  ezVec4I32 m_vNumWindSamples; // not unsigned so it can be loaded directly to a SIMD register, w contains total num samples

  /// Written in UpdateFromWorld(), read during the particle update. Changes over time are smoothed, so it is only updated, never reset.
  ezUniquePtr<WindSampleGrid> m_pWindSampleGrid;
};
