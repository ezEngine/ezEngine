#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltFixedConstraintComponentManager = ezComponentManager<class ezJoltFixedConstraintComponent, ezBlockStorageType::Compact>;

/// \brief Implements a fixed physics constraint.
///
/// Actors constrained this way may not move apart, at all.
/// This is mainly useful for adding constraints dynamically, for example to attach a dynamic object to another one once it hits it,
/// or to make it breakable, such that it gets removed when too much force acts on it.
class EZ_JOLTPLUGIN_DLL ezJoltFixedConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltFixedConstraintComponent, ezJoltConstraintComponent, ezJoltFixedConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezJoltFixedConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltFixedConstraintComponent

public:
  ezJoltFixedConstraintComponent();
  ~ezJoltFixedConstraintComponent();
};
