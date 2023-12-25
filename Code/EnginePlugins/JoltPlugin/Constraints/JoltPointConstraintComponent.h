#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltPointConstraintComponentManager = ezComponentManager<class ezJoltPointConstraintComponent, ezBlockStorageType::Compact>;

/// \brief Implements a physics constraint that allows rotation around one point.
///
/// The joined actors can freely rotate around the constraint position.
class EZ_JOLTPLUGIN_DLL ezJoltPointConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltPointConstraintComponent, ezJoltConstraintComponent, ezJoltPointConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltConstraintComponent

protected:
  virtual void ApplySettings() override;
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual bool ExceededBreakingPoint() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltPointConstraintComponent

public:
  ezJoltPointConstraintComponent();
  ~ezJoltPointConstraintComponent();
};
