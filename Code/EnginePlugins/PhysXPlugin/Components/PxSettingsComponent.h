#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <PhysXPlugin/PhysXInterface.h>

using ezPxSettingsComponentManager = ezSettingsComponentManager<class ezPxSettingsComponent>;

class EZ_PHYSXPLUGIN_DLL ezPxSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxSettingsComponent, ezSettingsComponent, ezPxSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxSettingsComponent

public:
  ezPxSettingsComponent();
  ~ezPxSettingsComponent();

  const ezPxSettings& GetSettings() const { return m_Settings; }

  const ezVec3& GetObjectGravity() const { return m_Settings.m_vObjectGravity; }               // [ property ]
  void SetObjectGravity(const ezVec3& v);                                                      // [ property ]

  const ezVec3& GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }         // [ property ]
  void SetCharacterGravity(const ezVec3& v);                                                   // [ property ]

  float GetMaxDepenetrationVelocity() const { return m_Settings.m_fMaxDepenetrationVelocity; } // [ property ]
  void SetMaxDepenetrationVelocity(float fMaxVelocity);                                        // [ property ]

  ezPxSteppingMode::Enum GetSteppingMode() const { return m_Settings.m_SteppingMode; }         // [ property ]
  void SetSteppingMode(ezPxSteppingMode::Enum mode);                                           // [ property ]

  float GetFixedFrameRate() const { return m_Settings.m_fFixedFrameRate; }                     // [ property ]
  void SetFixedFrameRate(float fFixedFrameRate);                                               // [ property ]

  ezUInt32 GetMaxSubSteps() const { return m_Settings.m_uiMaxSubSteps; }                       // [ property ]
  void SetMaxSubSteps(ezUInt32 uiMaxSubSteps);                                                 // [ property ]

protected:
  ezPxSettings m_Settings;
};
