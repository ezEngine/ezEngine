#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltPointConstraintComponentManager = ezComponentManager<class ezJoltPointConstraintComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJoltPointConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltPointConstraintComponent, ezJoltConstraintComponent, ezJoltPointConstraintComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltConstraintComponent

protected:
  virtual void ApplySettings() override;
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltPointConstraintComponent

public:
  ezJoltPointConstraintComponent();
  ~ezJoltPointConstraintComponent();
};
