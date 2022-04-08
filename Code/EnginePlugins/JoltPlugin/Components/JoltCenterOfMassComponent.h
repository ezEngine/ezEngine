#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

using ezJoltCenterOfMassComponentManager = ezComponentManager<class ezJoltCenterOfMassComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJoltCenterOfMassComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltCenterOfMassComponent, ezComponent, ezJoltCenterOfMassComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezJoltCenterOfMassComponent

public:
  ezJoltCenterOfMassComponent();
  ~ezJoltCenterOfMassComponent();
};
