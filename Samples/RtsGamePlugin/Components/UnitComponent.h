#pragma once

#include <RtsGamePlugin/AI/AiUtilitySystem.h>
#include <RtsGamePlugin/Components/ComponentMessages.h>

class RtsUnitComponentManager : public ezComponentManager<class RtsUnitComponent, ezBlockStorageType::FreeList>
{
public:
  RtsUnitComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

  void UnitUpdate(const ezWorldModule::UpdateContext& context);
};

enum class RtsUnitMode
{
  GuardLocation,
  MoveToPosition,
  AttackUnit,
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
  void OnMsgAssignPosition(RtsMsgAssignPosition& msg);
  void OnMsgSetTarget(RtsMsgSetTarget& msg);
  void OnMsgApplyDamage(RtsMsgApplyDamage& msg);
  void OnMsgGatherUnitStats(RtsMsgGatherUnitStats& msg);
  void OnMsgArrivedAtLocation(RtsMsgArrivedAtLocation& msg);

public:
  //////////////////////////////////////////////////////////////////////////
  //

  ezGameObject* FindClosestEnemy(float fMaxRadius) const;
  void FireAt(ezGameObjectHandle hUnit);
  bool AttackClosestEnemey(float fSearchRadius, float fIgnoreRadius);

protected:
  virtual void OnUnitDestroyed();

  friend class RtsAttackUnitAiUtility;
  friend class RtsGuardLocationAiUtility;
  friend class RtsMoveToPositionAiUtility;

  bool m_bModeChanged = true;
  RtsUnitMode m_UnitMode;

  ezVec2 m_vAssignedPosition;

  ezGameObjectHandle m_hAssignedUnitToAttack;
  ezGameObjectHandle m_hCurrentUnitToAttack;

  ezTime m_TimeLastShot;
  ezUniquePtr<RtsAiUtilitySystem> m_pAiSystem; // has to be a pointer because RtsAiUtilitySystem isn't copyable 

  void UpdateUnit();
};
