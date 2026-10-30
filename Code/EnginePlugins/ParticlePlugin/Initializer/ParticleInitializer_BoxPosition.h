#pragma once

#include <ParticlePlugin/Components/ParticleAreaComponent.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a box volume
///
/// Axes with a size of zero are flat, e.g. a size of zero along Z spawns particles in a rectangle.
///
/// If 'UseSpawnArea' is enabled and the object that the effect is attached to or one of its parents has an ezParticleBoxAreaComponent, that area is used instead
/// of the box defined here. The axes that are flat here stay flat, and the spawn counts are scaled by how much larger the area is
/// along the other axes. See ezParticleAreaComponent.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_BoxPosition final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_BoxPosition, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_BoxPosition();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  ezVec3 m_vPositionOffset;    ///< Center of the spawn box
  ezVec3 m_vSize;              ///< Size of the spawn box (full size, not half extents)
  ezString m_sScaleXParameter; ///< Optional parameter name to scale box width
  ezString m_sScaleYParameter; ///< Optional parameter name to scale box depth
  ezString m_sScaleZParameter; ///< Optional parameter name to scale box height
  bool m_bUseSpawnArea = false; ///< If true, an area component on the owner object or its parents replaces the shape. Should stay off for effects that are spawned by other effects.
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_BoxPosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_BoxPosition, ezParticleInitializer);

public:
  ezVec3 m_vPositionOffset;
  ezVec3 m_vSize;
  bool m_bUseSpawnArea = false;
  ezVec3 m_vSizeScale; ///< Scales the size of the area, from the scale parameters. Zero for axes on which the initializer is flat.

  virtual void CreateRequiredStreams() override;
  virtual bool IsReadyToSpawn() const override;
  virtual float GetSpawnCountMultiplier() const override;
  virtual void OnFinalize() override;
  virtual void UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject) override;

protected:
  virtual void OnReset() override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition;

  // written in UpdateFromWorld()
  ezParticleAreaReference m_Area;
  bool m_bUseArea = false;
  ezVec3 m_vAreaSize = ezVec3::MakeZero(); ///< Size of the area, including m_vSizeScale.
  float m_fAreaSpawnScale = 1.0f;
};
