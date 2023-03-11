#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

using ezShapeIconComponentManager = ezComponentManager<class ezShapeIconComponent, ezBlockStorageType::Compact>;

/// \brief This is a dummy component that the editor creates on all 'empty' nodes for the sole purpose to render a shape icon and enable picking.
///
/// Though in the future one could potentially use them for other editor functionality, such as displaying the object name or some other useful text.
class EZ_ENGINEPLUGINSCENE_DLL ezShapeIconComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezShapeIconComponent, ezComponent, ezShapeIconComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezShapeIconComponent

public:
  ezShapeIconComponent();
  ~ezShapeIconComponent();
};
