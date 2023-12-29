#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltSwingTwistConstraintComponentManager = ezComponentManager<class ezJoltSwingTwistConstraintComponent, ezBlockStorageType::Compact>;

/// \brief Implements a swing-twist physics constraint.
///
/// This is similar to a cone constraint but with more control.
/// The swing angle can be limited along Y and Z, so it can have a squashed shape, rather than perfectly round.
/// Additionally it can be limited how far the child actor may twist around the main axis.
class EZ_JOLTPLUGIN_DLL ezJoltSwingTwistConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltSwingTwistConstraintComponent, ezJoltConstraintComponent, ezJoltSwingTwistConstraintComponentManager);

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
  // ezJoltSwingTwistConstraintComponent

public:
  ezJoltSwingTwistConstraintComponent();
  ~ezJoltSwingTwistConstraintComponent();

  /// \brief Sets how far the child actor may swing along the Y axis.
  void SetSwingLimitY(ezAngle f);                          // [ property ]
  ezAngle GetSwingLimitY() const { return m_SwingLimitY; } // [ property ]

  /// \brief Sets how far the child actor may swing along the Z axis.
  void SetSwingLimitZ(ezAngle f);                          // [ property ]
  ezAngle GetSwingLimitZ() const { return m_SwingLimitZ; } // [ property ]

  /// \brief Sets how difficult it is to rotate the constraint.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// \brief Sets how far the child actor can rotate around the twist axis in one direction.
  void SetLowerTwistLimit(ezAngle f);                              // [ property ]
  ezAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  /// \brief Sets how far the child actor can rotate around the twist axis in the other direction.
  void SetUpperTwistLimit(ezAngle f);                              // [ property ]
  ezAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  // void SetTwistDriveMode(ezJoltConstraintDriveMode::Enum mode);                          // [ property ]
  // ezJoltConstraintDriveMode::Enum GetTwistDriveMode() const { return m_TwistDriveMode; } // [ property ]

  // void SetTwistDriveTargetValue(ezAngle f);                                    // [ property ]
  // ezAngle GetTwistDriveTargetValue() const { return m_TwistDriveTargetValue; } // [ property ]

  // void SetTwistDriveStrength(float f);                                  // [ property ]
  // float GetTwistDriveStrength() const { return m_fTwistDriveStrength; } // [ property ]

protected:
  ezAngle m_SwingLimitY;
  ezAngle m_SwingLimitZ;

  float m_fFriction = 0.0f;

  ezAngle m_LowerTwistLimit = ezAngle::MakeFromDegree(90);
  ezAngle m_UpperTwistLimit = ezAngle::MakeFromDegree(90);

  // not sure whether these are useful
  // maybe just expose an 'untwist' feature, with strength/frequency and drive to position 0 ?
  // driving to velocity makes no sense, since the constraint always has a lower/upper twist limit
  // probably would need a 6DOF joint for more advanced use cases
  // ezEnum<ezJoltConstraintDriveMode> m_TwistDriveMode;
  // ezAngle m_TwistDriveTargetValue;
  // float m_fTwistDriveStrength = 0; // 0 means maximum strength
};
