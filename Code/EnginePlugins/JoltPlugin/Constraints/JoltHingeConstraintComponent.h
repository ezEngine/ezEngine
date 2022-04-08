#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltHingeConstraintComponentManager = ezComponentManager<class ezJoltHingeConstraintComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJoltHingeConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltHingeConstraintComponent, ezJoltConstraintComponent, ezJoltHingeConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltHingeConstraintComponent

public:
  ezJoltHingeConstraintComponent();
  ~ezJoltHingeConstraintComponent();

  void SetLimitMode(ezJoltConstraintLimitMode::Enum mode);                     // [ property ]
  ezJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  void SetLowerLimitAngle(ezAngle f);                         // [ property ]
  ezAngle GetLowerLimitAngle() const { return m_LowerLimit; } // [ property ]

  void SetUpperLimitAngle(ezAngle f);                         // [ property ]
  ezAngle GetUpperLimitAngle() const { return m_UpperLimit; } // [ property ]

  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  void SetDriveMode(ezJoltConstraintDriveMode::Enum mode);                     // [ property ]
  ezJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  void SetDriveTargetValue(ezAngle f);                               // [ property ]
  ezAngle GetDriveTargetValue() const { return m_DriveTargetValue; } // [ property ]

  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

  virtual void ApplySettings() final override;

protected:
  ezEnum<ezJoltConstraintLimitMode> m_LimitMode;
  ezAngle m_LowerLimit;
  ezAngle m_UpperLimit;
  float m_fFriction = 0;
  ezEnum<ezJoltConstraintDriveMode> m_DriveMode;
  ezAngle m_DriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
