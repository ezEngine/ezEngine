#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

#include <Foundation/SimdMath/SimdNoise.h>

namespace JPH
{
  class PhysicsSystem;
} // namespace JPH

class EZ_JOLTPLUGIN_DLL ezJoltWaterVolumeComponentManager : public ezComponentManager<class ezJoltWaterVolumeComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltWaterVolumeComponentManager(ezWorld* pWorld);
  ~ezJoltWaterVolumeComponentManager();

  void UpdateWaterVolumes(ezTime deltaTime);

  ezSimdPerlinNoise m_Noise;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Creates a water volume that applies a buoyancy force to submerged dynamic actors and triggers surface interactions.
///
/// This component needs a trigger component besides it to detect when actors enter or leave the water volume.
class EZ_JOLTPLUGIN_DLL ezJoltWaterVolumeComponent : public ezJoltShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltWaterVolumeComponent, ezJoltShapeComponent, ezJoltWaterVolumeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltWaterVolumeComponent

public:
  ezJoltWaterVolumeComponent();
  ~ezJoltWaterVolumeComponent();

  ezVec3 m_vExtents = ezVec3(10.0f); // [ property ]

  /// \brief Direction and speed of the water flow in local space.
  ezVec3 m_vFlow = ezVec3::MakeZero(); // [ property ]

  /// \brief Strength of the noise that is added to vary the water surface height.
  float m_fNoiseStrength = 0.0f; // [ property ]

  /// \brief The surface resource that defines the water surface interaction. No other properties of the surface are used.
  ezSurfaceResourceHandle m_hSurface; // [ property ]

  /// \brief Which interaction should be triggered when an actor enters the water volume. See ezSurfaceResource.
  ezHashedString m_sInteraction;                          // [ property ]

private:
  void OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg); // [ msg handler ]

  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) override;

  void Update(JPH::PhysicsSystem& joltSystem, ezTime deltaTime);
  void UpdateWaterPlane(const ezVec3& vGravity);

  ezPlane m_SurfacePlane = ezPlane::MakeFromNormalAndPoint(ezVec3::MakeAxisZ(), ezVec3::MakeZero());
  ezVec3 m_vGravity = ezVec3::MakeZero();
  float m_fNoiseTime = 0.0f;

  ezHashSet<ezComponentHandle> m_SubmergedActors;
};
