#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltHingeConstraintComponentManager = ezComponentManager<class ezJoltHingeConstraintComponent, ezBlockStorageType::Compact>;

/// \brief Implements a rotational physics constraint.
///
/// Hinge constraints are typically used for doors and wheels. They may either rotate freely
/// or be limited between an upper and lower angle.
/// It is possible to enable a drive to make the hinge rotate at a certain speed, or return to a desired angle.
class EZ_JOLTPLUGIN_DLL ezJoltHingeConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltHingeConstraintComponent, ezJoltConstraintComponent, ezJoltHingeConstraintComponentManager);

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
  // ezJoltHingeConstraintComponent

public:
  ezJoltHingeConstraintComponent();
  ~ezJoltHingeConstraintComponent();

  /// \brief Enables a rotational limit on the hinge.
  void SetLimitMode(ezJoltConstraintLimitMode::Enum mode);                     // [ property ]
  ezJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  /// \brief Sets how far the hinge may rotate in one direction.
  void SetLowerLimitAngle(ezAngle f);                         // [ property ]
  ezAngle GetLowerLimitAngle() const { return m_LowerLimit; } // [ property ]

  /// \brief Sets how far the hinge may rotate in the other direction.
  void SetUpperLimitAngle(ezAngle f);                         // [ property ]
  ezAngle GetUpperLimitAngle() const { return m_UpperLimit; } // [ property ]

  /// \brief Sets how difficult it is to rotate the hinge.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// \brief Enables a drive for the hinge to either constantly rotate or attempt to reach a certain rotation angle.
  void SetDriveMode(ezJoltConstraintDriveMode::Enum mode);                     // [ property ]
  ezJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  /// \brief Sets the drive target angle or velocity.
  void SetDriveTargetValue(ezAngle f);                               // [ property ]
  ezAngle GetDriveTargetValue() const { return m_DriveTargetValue; } // [ property ]

  /// \brief Sets how much force the drive may use to reach its target.
  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

protected:
  ezEnum<ezJoltConstraintLimitMode> m_LimitMode;
  ezAngle m_LowerLimit;
  ezAngle m_UpperLimit;
  float m_fFriction = 0;
  ezEnum<ezJoltConstraintDriveMode> m_DriveMode;
  ezAngle m_DriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
