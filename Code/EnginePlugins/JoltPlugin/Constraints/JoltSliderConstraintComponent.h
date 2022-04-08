#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltSliderConstraintComponentManager = ezComponentManager<class ezJoltSliderConstraintComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJoltSliderConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltSliderConstraintComponent, ezJoltConstraintComponent, ezJoltSliderConstraintComponentManager);

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
  // ezJoltSliderConstraintComponent

public:
  ezJoltSliderConstraintComponent();
  ~ezJoltSliderConstraintComponent();

  void SetLimitMode(ezJoltConstraintLimitMode::Enum mode);                     // [ property ]
  ezJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  void SetLowerLimitDistance(float f);                                  // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  void SetUpperLimitDistance(float f);                                  // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  void SetDriveMode(ezJoltConstraintDriveMode::Enum mode);                     // [ property ]
  ezJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  void SetDriveTargetValue(float f);                                // [ property ]
  float GetDriveTargetValue() const { return m_fDriveTargetValue; } // [ property ]

  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

  virtual void ApplySettings() final override;

protected:
  ezEnum<ezJoltConstraintLimitMode> m_LimitMode;
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fFriction = 0;

  ezEnum<ezJoltConstraintDriveMode> m_DriveMode;
  float m_fDriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
