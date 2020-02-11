#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <PhysXPlugin/PhysXPluginDLL.h>

/// \brief Base class for all PhysX components, such that they all have a common ancestor
class EZ_PHYSXPLUGIN_DLL ezPxComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezPxComponent

public:
  ezPxComponent();
  ~ezPxComponent();
};
