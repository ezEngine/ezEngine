#pragma once

#include <Core/World/Component.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>

using ezJoltHeightfieldColliderComponentManager = ezComponentManager<class ezJoltHeightfieldColliderComponent, ezBlockStorageType::FreeList>;

/// Manages a Jolt static body with a heightfield shape.
class EZ_JOLTPLUGIN_DLL ezJoltHeightfieldColliderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltHeightfieldColliderComponent, ezComponent, ezJoltHeightfieldColliderComponentManager);

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

public:
  ezJoltHeightfieldColliderComponent();
  ~ezJoltHeightfieldColliderComponent();

  ezJoltHeightfieldResourceHandle m_hHeightfield; //< [ property ]

private:
  ezUInt32 m_uiJoltBodyID = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
};
