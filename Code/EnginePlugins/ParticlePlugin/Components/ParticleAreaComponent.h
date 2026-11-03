#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleEffectInstance;

/// Base class for areas that the box, sphere and cylinder position initializers use as their spawn shape.
///
/// Only initializers with 'UseSpawnArea' enabled use areas, all others ignore them. It is opt-in, so that an effect that merely is a child
/// of an object with an area keeps its authored shape. Effects spawned by other effects share the owner of their parent effect, so
/// they would spawn in the whole area, instead of at the parent particle, and shouldn't enable it.
///
/// The initializers look for an area of their type (e.g. a box area for the box position initializer) on the object that the effect
/// is attached to and then on its parents, the closest one wins. Areas of other types are ignored, so an effect that uses several
/// position initializers can get an area for each of them, also on the same object.
/// So an area can be placed on the same object as the particle component, or once on a parent object of several effects.
/// Without an area of the matching type, the initializer uses its own shape.
///
/// The area replaces the transform and the sizes of the initializer. The position offset of the initializer still applies, in the space
/// of the effect, so that an effect can keep several layers apart. Particles may therefore spawn outside of the area by that offset.
/// The initializer still decides how the
/// shape is used, e.g. spawning only on the surface, or only in a flat part of the area for sizes of zero.
/// The spawn counts of the effect are scaled by how much larger the area is than the shape of the initializer, so the initializer
/// defines the particle density. The scaling is limited, see ezParticleInitializer::GetInstanceSpawnCountMultiplier().
///
/// The transform and the sizes are read every frame. The spawn count scale is only determined once, when the effect starts,
/// so later changes of the sizes or of 'SpawnScale' don't change how many particles spawn.
class EZ_PARTICLEPLUGIN_DLL ezParticleAreaComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezParticleAreaComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleAreaComponent

public:
  ezParticleAreaComponent();
  ~ezParticleAreaComponent();

  /// Additional factor for the spawn counts of the effects that use this area.
  float m_fSpawnScale = 1.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using ezParticleBoxAreaComponentManager = ezComponentManager<class ezParticleBoxAreaComponent, ezBlockStorageType::Compact>;

/// A box shaped spawn area for the box position initializer, see ezParticleAreaComponent.
class EZ_PARTICLEPLUGIN_DLL ezParticleBoxAreaComponent : public ezParticleAreaComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleBoxAreaComponent, ezParticleAreaComponent, ezParticleBoxAreaComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleBoxAreaComponent

public:
  ezParticleBoxAreaComponent();
  ~ezParticleBoxAreaComponent();

  /// Full size of the box, centered at the object.
  ezVec3 m_vSize = ezVec3(2.0f); // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using ezParticleSphereAreaComponentManager = ezComponentManager<class ezParticleSphereAreaComponent, ezBlockStorageType::Compact>;

/// A sphere shaped spawn area for the sphere position initializer, see ezParticleAreaComponent.
class EZ_PARTICLEPLUGIN_DLL ezParticleSphereAreaComponent : public ezParticleAreaComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleSphereAreaComponent, ezParticleAreaComponent, ezParticleSphereAreaComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleSphereAreaComponent

public:
  ezParticleSphereAreaComponent();
  ~ezParticleSphereAreaComponent();

  float m_fRadius = 1.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using ezParticleCylinderAreaComponentManager = ezComponentManager<class ezParticleCylinderAreaComponent, ezBlockStorageType::Compact>;

/// A cylinder shaped spawn area for the cylinder position initializer, see ezParticleAreaComponent.
///
/// The cylinder is aligned with the Z axis and centered at the object.
class EZ_PARTICLEPLUGIN_DLL ezParticleCylinderAreaComponent : public ezParticleAreaComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleCylinderAreaComponent, ezParticleAreaComponent, ezParticleCylinderAreaComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleCylinderAreaComponent

public:
  ezParticleCylinderAreaComponent();
  ~ezParticleCylinderAreaComponent();

  float m_fRadius = 1.0f; // [ property ]
  float m_fHeight = 2.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

/// Used by the position initializers to find the ezParticleAreaComponent of an effect and to track its transform.
///
/// Update() may only be called from ezParticleModule::UpdateFromWorld().
class EZ_PARTICLEPLUGIN_DLL ezParticleAreaReference
{
public:
  void Reset();

  /// Searches the area in the first call, afterwards only updates the transform of the area relative to the effect.
  ///
  /// Returns the closest area of the requested type, if one was found and is still active, otherwise nullptr.
  /// The search only happens once, so an area that is added later is ignored. If the closest area is inactive at that time, areas on
  /// parent objects are not considered.
  template <typename AreaType>
  const AreaType* Update(const ezWorld& world, const ezParticleEffectInstance& effect)
  {
    return static_cast<const AreaType*>(Update(world, effect, ezGetStaticRTTI<AreaType>()));
  }

  /// Whether it is known if there is an area, which is the case after the first Update() and for effects without an owner object.
  bool IsReady(const ezParticleEffectInstance& effect) const;

  /// Whether the initializer switched to the area since the last Reset(), see UseArea(). Stays true if the area is gone afterwards.
  bool IsAreaUsed() const { return m_bAreaUsed; }

  /// Called by the initializer when it replaces its shape with the area.
  void UseArea(float fSpawnCountMultiplier)
  {
    m_bAreaUsed = true;
    m_fSpawnCountMultiplier = fSpawnCountMultiplier;
  }

  /// The value passed to UseArea(), otherwise 1.
  float GetSpawnCountMultiplier() const { return m_fSpawnCountMultiplier; }

  /// Transforms from the space of the area into the space of the effect. Keeps the last value, once Update() doesn't return an area anymore.
  const ezTransform& GetTransform() const { return m_Transform; }

  /// Same as GetTransform(), but moved by the given position offset, which is in the space of the effect.
  ///
  /// The offset is therefore not affected by the rotation and scale of the area, so the authored distance stays the same for any area.
  ezTransform GetTransformWithOffset(const ezVec3& vOffset) const
  {
    ezTransform result = m_Transform;
    result.m_vPosition += vOffset;
    return result;
  }

private:
  const ezParticleAreaComponent* Update(const ezWorld& world, const ezParticleEffectInstance& effect, const ezRTTI* pAreaType);

  ezComponentHandle m_hArea;
  ezTransform m_Transform = ezTransform::MakeIdentity();
  float m_fSpawnCountMultiplier = 1.0f;
  bool m_bSearched = false;
  bool m_bAreaUsed = false;
};
