#pragma once

#include <PhysXPlugin/Basics.h>
#include <GameUtils/Components/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>


typedef ezSettingsComponentManager<class ezPxSettingsComponent> ezPxSettingsComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxSettingsComponent, ezSettingsComponent, ezPxSettingsComponentManager);

public:
  ezPxSettingsComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  const ezVec3& GetObjectGravity() const { return m_vObjectGravity; }
  void SetObjectGravity(const ezVec3& v) { m_vObjectGravity = v; SetModified(EZ_BIT(0)); }

  const ezVec3& GetCharacterGravity() const { return m_vCharacterGravity; }
  void SetCharacterGravity(const ezVec3& v) { m_vCharacterGravity = v; SetModified(EZ_BIT(1)); }

private:
  ezVec3 m_vObjectGravity;
  ezVec3 m_vCharacterGravity;

};
