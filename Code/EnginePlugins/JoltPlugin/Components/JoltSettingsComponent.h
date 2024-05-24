#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <JoltPlugin/Declarations.h>

using ezJoltSettingsComponentManager = ezSettingsComponentManager<class ezJoltSettingsComponent>;

class EZ_JOLTPLUGIN_DLL ezJoltSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltSettingsComponent, ezSettingsComponent, ezJoltSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltSettingsComponent

public:
  ezJoltSettingsComponent();
  ~ezJoltSettingsComponent();

  const ezJoltSettings& GetSettings() const { return m_Settings; }

  const ezVec3& GetObjectGravity() const { return m_Settings.m_vObjectGravity; }           // [ property ]
  void SetObjectGravity(const ezVec3& v);                                                  // [ property ]

  const ezVec3& GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }     // [ property ]
  void SetCharacterGravity(const ezVec3& v);                                               // [ property ]

  ezJoltSteppingMode::Enum GetSteppingMode() const { return m_Settings.m_SteppingMode; }   // [ property ]
  void SetSteppingMode(ezJoltSteppingMode::Enum mode);                                     // [ property ]

  float GetFixedFrameRate() const { return m_Settings.m_fFixedFrameRate; }                 // [ property ]
  void SetFixedFrameRate(float fFixedFrameRate);                                           // [ property ]

  ezUInt32 GetMaxSubSteps() const { return m_Settings.m_uiMaxSubSteps; }                   // [ property ]
  void SetMaxSubSteps(ezUInt32 uiMaxSubSteps);                                             // [ property ]

  ezUInt32 GetMaxBodies() const { return m_Settings.m_uiMaxBodies; }                       // [ property ]
  void SetMaxBodies(ezUInt32 uiMaxBodies);                                                 // [ property ]

  float GetSleepVelocityThreshold() const { return m_Settings.m_fSleepVelocityThreshold; } // [ property ]
  void SetSleepVelocityThreshold(float fSleepVelocityThreshold);                           // [ property ]

protected:
  ezJoltSettings m_Settings;
};
