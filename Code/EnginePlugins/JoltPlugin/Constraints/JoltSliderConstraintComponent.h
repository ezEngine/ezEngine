#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltSliderConstraintComponentManager = ezComponentManager<class ezJoltSliderConstraintComponent, ezBlockStorageType::Compact>;

/// \brief Implements a sliding physics constraint.
///
/// The child actor may move along the parent actor along the positive X axis of the constraint.
/// Usually lower and upper limits are used to prevent infinite movement.
class EZ_JOLTPLUGIN_DLL ezJoltSliderConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltSliderConstraintComponent, ezJoltConstraintComponent, ezJoltSliderConstraintComponentManager);

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
  // ezJoltSliderConstraintComponent

public:
  ezJoltSliderConstraintComponent();
  ~ezJoltSliderConstraintComponent();

  /// \brief Enables a translational limit on the slider.
  void SetLimitMode(ezJoltConstraintLimitMode::Enum mode);                     // [ property ]
  ezJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  /// \brief Sets how far child actor may move in one direction.
  void SetLowerLimitDistance(float f);                                  // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  /// \brief Sets how far child actor may move in the other direction.
  void SetUpperLimitDistance(float f);                                  // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  /// \brief Sets how difficult it is to move the child actor.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// \brief Enables a drive for the slider to either constantly move or attempt to reach a certain position.
  void SetDriveMode(ezJoltConstraintDriveMode::Enum mode);                     // [ property ]
  ezJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  /// \brief Sets the drive target position or velocity.
  void SetDriveTargetValue(float f);                                // [ property ]
  float GetDriveTargetValue() const { return m_fDriveTargetValue; } // [ property ]

  /// \brief Sets how much force the drive may use to reach its target.
  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

protected:
  ezEnum<ezJoltConstraintLimitMode> m_LimitMode;
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fFriction = 0;

  ezEnum<ezJoltConstraintDriveMode> m_DriveMode;
  float m_fDriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
