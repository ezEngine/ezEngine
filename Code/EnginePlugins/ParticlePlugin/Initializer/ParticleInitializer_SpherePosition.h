#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a sphere volume
///
/// Can spawn throughout the volume or only on the surface.
/// Optionally sets initial velocity pointing outward from the center.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_SpherePosition final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_SpherePosition, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_SpherePosition();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

public:
  ezVec3 m_vPositionOffset;         ///< Center of the sphere
  float m_fRadius;                  ///< Sphere radius
  bool m_bSpawnOnSurface;           ///< If true, spawn only on sphere surface
  bool m_bSetVelocity;              ///< If true, set velocity pointing outward from center
  ezVarianceTypeFloat m_Speed;      ///< Speed value when setting velocity
  ezString m_sScaleRadiusParameter; ///< Optional parameter name to scale radius
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_SpherePosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_SpherePosition, ezParticleInitializer);

public:
  ezVec3 m_vPositionOffset;
  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  ezVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;
};
