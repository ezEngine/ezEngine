#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <FmodPlugin/FmodPluginDLL.h>

/// \brief Base class for all FMOD components, such that they all have a common ancestor
class EZ_FMODPLUGIN_DLL ezFmodComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezFmodComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override {}

  //////////////////////////////////////////////////////////////////////////
  // ezFmodComponent

public:
  ezFmodComponent();
  ~ezFmodComponent();

private:
  virtual void ezFmodComponentIsAbstract() = 0; // abstract classes are not shown in the UI, since this class has no other abstract functions so far, this is a dummy
};
