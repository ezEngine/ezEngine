#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Components/ParticleAreaComponent.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a sphere volume
///
/// Can spawn throughout the volume or only on the surface.
/// Optionally sets initial velocity pointing outward from the center.
///
/// If 'UseSpawnArea' is enabled and the object that the effect is attached to or one of its parents has an ezParticleSphereAreaComponent, that area is used instead
/// of the sphere defined here. The spawn counts are then scaled by how much larger the volume (or the surface) of the area is.
/// See ezParticleAreaComponent.
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
  bool m_bUseSpawnArea = false;     ///< If true, an area component on the owner object or its parents replaces the shape. Should stay off for effects that are spawned by other effects.
  ezVarianceTypeFloat m_Speed;      ///< Speed value when setting velocity
  ezString m_sScaleRadiusParameter; ///< Optional parameter name to scale radius
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_SpherePosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_SpherePosition, ezParticleInitializer);

public:
  ezVec3 m_vPositionOffset;
  float m_fRadius = 1.0f;      ///< The radius of the initializer, replaced by the radius of the area, once there is one.
  float m_fRadiusScale = 1.0f; ///< Scales the radius of the area, from the scale parameter.
  ezVarianceTypeFloat m_Speed;
  bool m_bSpawnOnSurface = false;
  bool m_bSetVelocity = false;
  bool m_bUseSpawnArea = false;

  virtual void CreateRequiredStreams() override;
  virtual bool PrepareToSpawn() override;
  virtual float GetSpawnCountMultiplier() const override;
  virtual void OnFinalize() override;
  virtual void UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject) override;

protected:
  virtual void OnReset() override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  // written in UpdateFromWorld()
  ezParticleAreaReference m_Area;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
};
