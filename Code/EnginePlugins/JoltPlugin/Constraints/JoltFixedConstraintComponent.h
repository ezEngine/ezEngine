#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltFixedConstraintComponentManager = ezComponentManager<class ezJoltFixedConstraintComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJoltFixedConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltFixedConstraintComponent, ezJoltConstraintComponent, ezJoltFixedConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezJoltFixedConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltFixedConstraintComponent

public:
  ezJoltFixedConstraintComponent();
  ~ezJoltFixedConstraintComponent();

  virtual void ApplySettings() final override;
};
