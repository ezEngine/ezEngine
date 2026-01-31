#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/World/World.h>

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
};

//////////////////////////////////////////////////////////////////////////

/// \brief TODO
class EZ_JOLTPLUGIN_DLL ezJoltWaterVolumeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltWaterVolumeComponent, ezComponent, ezJoltWaterVolumeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltWaterVolumeComponent

public:
  ezJoltWaterVolumeComponent();
  ~ezJoltWaterVolumeComponent();

  ezVec3 m_vExtents = ezVec3(10.0f);   // [ property ]
  ezVec3 m_vFlow = ezVec3::MakeZero(); // [ property ]

private:
  void Update(JPH::PhysicsSystem& joltSystem, ezTime deltaTime);

  void UpdateWaterPlane(const ezVec3& vGravity);

  ezPlane m_surfacePlane;
  ezVec3 m_vGravity = ezVec3::MakeZero();
};
