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

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  //////////////////////////////////////////////////////////////////////////
  // Message Handlers
  void OnMsgNavigateTo(RtsMsgNavigateTo& msg);
  void OnMsgSetTarget(RtsMsgSetTarget& msg);

public:
  //////////////////////////////////////////////////////////////////////////
  //

protected:
  RtsUnitMode m_UnitMode;
  ezTime m_TimeLastShot;
  ezGameObjectHandle m_hShootAtUnit;
  ezVec2 m_vShootAtPosition;

  void UpdateUnit();
};
