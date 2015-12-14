#pragma once

#include <FmodPlugin/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

typedef ezComponentManagerAbstract<class ezFmodComponent> ezFmodComponentManager;

/// \brief Base class for all PhysX components, such that they all have a common ancestor
class EZ_FMODPLUGIN_DLL ezFmodComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodComponent, ezComponent, ezFmodComponentManager);
  virtual void ezFmodComponentIsAbstract() = 0; // abstract classes are not shown in the UI, since this class has no other abstract functions so far, this is a dummy

public:
  ezFmodComponent() {}

  virtual void SerializeComponent(ezWorldWriter& stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& stream) override {}

  // ************************************* PROPERTIES ***********************************


protected:

  // ************************************* FUNCTIONS *****************************

public:


};
