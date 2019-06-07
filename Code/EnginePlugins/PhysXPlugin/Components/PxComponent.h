#pragma once

#include <PhysXPlugin/PhysXPluginDLL.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

/// \brief Base class for all PhysX components, such that they all have a common ancestor
class EZ_PHYSXPLUGIN_DLL ezPxComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxComponent, ezComponent);

public:
  ezPxComponent() {}

  virtual void SerializeComponent(ezWorldWriter& stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& stream) override {}

  // ************************************* PROPERTIES ***********************************


protected:
  //ezBitflags<ezTransformComponentFlags> m_Flags;

  // ************************************* FUNCTIONS *****************************

public:


};
