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
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_BoxPosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_BoxPosition, ezParticleInitializer);

public:
  ezParticleInitializer_BoxPosition();

  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual bool GetUpdateFromWorld() const override { return m_bUseSpawnArea; }
  virtual void UpdateFromWorld(const ezParticleModuleContext& ctx, const ezWorld& world, ezGameObjectHandle hOwnerObject) const override;
  virtual bool PrepareToSpawn(const ezParticleModuleContext& ctx) const override;
  virtual float GetInstanceSpawnCountMultiplier(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;

public:
  ezVec3 m_vPositionOffset;    ///< Center of the spawn box
  ezVec3 m_vSize;              ///< Size of the spawn box (full size, not half extents)
  ezHashedString m_sScaleXParameter; ///< Optional parameter name to scale box width
  ezHashedString m_sScaleYParameter; ///< Optional parameter name to scale box depth
  ezHashedString m_sScaleZParameter; ///< Optional parameter name to scale box height
  bool m_bUseSpawnArea = false; ///< If true, an area component on the owner object or its parents replaces the shape. Should stay off for effects that are spawned by other effects.

private:
  struct InstanceData
  {
    ezProcessingStream* m_pStreamPosition = nullptr;
    ezVec3 m_vSize;      ///< The size of the initializer, replaced by the size of the area, once there is one.
    ezVec3 m_vSizeScale; ///< Scales the size of the area, from the scale parameters. Zero for axes on which the initializer is flat.

    // written in UpdateFromWorld()
    ezParticleAreaReference m_Area;
  };
};
