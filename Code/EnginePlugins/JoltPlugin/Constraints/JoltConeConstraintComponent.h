#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltConeConstraintComponentManager = ezComponentManager<class ezJoltConeConstraintComponent, ezBlockStorageType::Compact>;

/// \brief Implements a conical physics constraint.
///
/// The child actor can swing in a cone with a given angle around the anchor point on the parent actor.
class EZ_JOLTPLUGIN_DLL ezJoltConeConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltConeConstraintComponent, ezJoltConstraintComponent, ezJoltConeConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltConeConstraintComponent

public:
  ezJoltConeConstraintComponent();
  ~ezJoltConeConstraintComponent();

  void SetConeAngle(ezAngle f);                        // [ property ]
  ezAngle GetConeAngle() const { return m_ConeAngle; } // [ property ]

protected:
  ezAngle m_ConeAngle;
};
