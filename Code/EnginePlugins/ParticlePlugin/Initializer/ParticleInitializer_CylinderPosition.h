#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Components/ParticleAreaComponent.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a cylinder volume
///
/// Can spawn throughout the volume or only on the surface.
/// Optionally sets initial velocity pointing outward from the cylinder axis.
/// With a height of zero, particles spawn in a disc (or on a ring, with 'OnSurface').
///
/// If 'UseSpawnArea' is enabled and the object that the effect is attached to or one of its parents has an ezParticleCylinderAreaComponent, that area is used instead
/// of the cylinder defined here. With a height of zero here, the height of the area is ignored. The spawn counts are scaled by how
/// much larger the volume (or the surface) of the area is. See ezParticleAreaComponent.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_CylinderPosition final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_CylinderPosition, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_CylinderPosition();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

public:
  ezVec3 m_vPositionOffset;         ///< Center of the cylinder
  float m_fRadius;                  ///< Cylinder radius
  float m_fHeight;                  ///< Cylinder height
  bool m_bSpawnOnSurface;           ///< If true, spawn only on cylinder surface
  bool m_bSetVelocity;              ///< If true, set velocity pointing outward from axis
  bool m_bUseSpawnArea = false;     ///< If true, an area component on the owner object or its parents replaces the shape. Should stay off for effects that are spawned by other effects.
  ezVarianceTypeFloat m_Speed;      ///< Speed value when setting velocity
  ezString m_sScaleRadiusParameter; ///< Optional parameter name to scale radius
  ezString m_sScaleHeightParameter; ///< Optional parameter name to scale height
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_CylinderPosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_CylinderPosition, ezParticleInitializer);

public:
  ezVec3 m_vPositionOffset;
  float m_fRadius;             ///< The radius of the initializer, replaced by the radius of the area, once there is one.
  float m_fHeight;             ///< The height of the initializer, replaced by the height of the area, once there is one.
  float m_fRadiusScale = 1.0f; ///< Scales the radius of the area, from the scale parameter.
  float m_fHeightScale = 1.0f; ///< Scales the height of the area, from the scale parameter. Zero if the initializer is flat.
  ezVarianceTypeFloat m_Speed;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  bool m_bUseSpawnArea = false;

  virtual bool IsReadyToSpawn() const override;
  virtual float GetSpawnCountMultiplier() const override;
  virtual void OnFinalize() override;
  virtual void UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject) override;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void OnReset() override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;

  // written in UpdateFromWorld()
  ezParticleAreaReference m_Area;
};
