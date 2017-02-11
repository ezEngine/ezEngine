#pragma once

#include <PhysXPlugin/PhysXInterface.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>

typedef ezSettingsComponentManager<class ezPxSettingsComponent> ezPxSettingsComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxSettingsComponent, ezSettingsComponent, ezPxSettingsComponentManager);

public:
  ezPxSettingsComponent();
  ~ezPxSettingsComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  const ezVec3& GetObjectGravity() const { return m_Settings.m_vObjectGravity; }
  void SetObjectGravity(const ezVec3& v);

  const ezVec3& GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }
  void SetCharacterGravity(const ezVec3& v);

  ezPxSteppingMode::Enum GetSteppingMode() const { return m_Settings.m_SteppingMode; }
  void SetSteppingMode(ezPxSteppingMode::Enum mode);

  float GetFixedFrameRate() const { return m_Settings.m_fFixedFrameRate; }
  void SetFixedFrameRate(float fFixedFrameRate);

  ezUInt32 GetMaxSubSteps() const { return m_Settings.m_uiMaxSubSteps; }
  void SetMaxSubSteps(ezUInt32 uiMaxSubSteps);

  const ezPxSettings& GetSettings() const { return m_Settings; }

private:
  ezPxSettings m_Settings;
};
