#pragma once

#include <RtsGamePlugin/Components/ComponentMessages.h>
#include <RtsGamePlugin/RtsGamePlugin.h>

class RtsUnitComponentManager : public ezComponentManager<class RtsUnitComponent, ezBlockStorageType::Compact>
{
public:
  RtsUnitComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

  void UnitUpdate(const ezWorldModule::UpdateContext& context);
};

enum class RtsUnitMode
{
  Idle,
  ShootAtPosition,
  ShootAtUnit,
};

class RtsUnitComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RtsUnitComponent, ezComponent, RtsUnitComponentManager);

public:
  RtsUnitComponent();
  ~RtsUnitComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  ezUInt16 m_uiMaxHealth = 100;
  ezUInt16 m_uiCurHealth = 0;

  void SetOnDestroyedPrefab(const char* szPrefab);
  const char* GetOnDestroyedPrefab() const;

private:
  ezPrefabResourceHandle m_hOnDestroyedPrefab;

  //////////////////////////////////////////////////////////////////////////
  // Message Handlers
  void OnMsgNavigateTo(RtsMsgNavigateTo& msg);
  void OnMsgSetTarget(RtsMsgSetTarget& msg);
  void OnMsgApplyDamage(RtsMsgApplyDamage& msg);

public:
  //////////////////////////////////////////////////////////////////////////
  //

protected:
  virtual void OnUnitDestroyed();

  RtsUnitMode m_UnitMode;
  ezTime m_TimeLastShot;
  ezGameObjectHandle m_hShootAtUnit;
  ezVec2 m_vShootAtPosition;

  void UpdateUnit();
};
