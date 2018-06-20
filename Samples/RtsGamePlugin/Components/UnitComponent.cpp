#include <PCH.h>

#include <Foundation/Utilities/Stats.h>
#include <RtsGamePlugin/AI/AttackUnitAiUtility.h>
#include <RtsGamePlugin/AI/GuardLocationUtility.h>
#include <RtsGamePlugin/AI/MoveToPositionUtility.h>
#include <RtsGamePlugin/Components/UnitComponent.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(RtsUnitComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxHealth", m_uiMaxHealth)->AddAttributes(new ezDefaultValueAttribute(100)),
    EZ_MEMBER_PROPERTY("CurHealth", m_uiCurHealth),
    EZ_ACCESSOR_PROPERTY("OnDestroyedPrefab", GetOnDestroyedPrefab, SetOnDestroyedPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
  }
  EZ_END_PROPERTIES

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(RtsMsgSetTarget, OnMsgSetTarget),
    EZ_MESSAGE_HANDLER(RtsMsgAssignPosition, OnMsgAssignPosition),
    EZ_MESSAGE_HANDLER(RtsMsgApplyDamage, OnMsgApplyDamage),
    EZ_MESSAGE_HANDLER(RtsMsgGatherUnitStats, OnMsgGatherUnitStats),
    EZ_MESSAGE_HANDLER(RtsMsgArrivedAtLocation, OnMsgArrivedAtLocation),
  }
  EZ_END_MESSAGEHANDLERS

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RTS Sample"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE;
// clang-format on

RtsUnitComponent::RtsUnitComponent() = default;
RtsUnitComponent::~RtsUnitComponent() = default;

void RtsUnitComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_uiMaxHealth;
  s << m_uiCurHealth;
  s << m_hOnDestroyedPrefab;
}

void RtsUnitComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_uiMaxHealth;
  s >> m_uiCurHealth;

  if (uiVersion >= 2)
  {
    s >> m_hOnDestroyedPrefab;
  }
}

void RtsUnitComponent::OnMsgAssignPosition(RtsMsgAssignPosition& msg)
{
  m_UnitMode = RtsUnitMode::MoveToPosition;
  m_vAssignedPosition = msg.m_vTargetPosition;
  m_bModeChanged = true;
}

void RtsUnitComponent::OnMsgSetTarget(RtsMsgSetTarget& msg)
{
  m_hAssignedUnitToAttack = msg.m_hObject;
  m_UnitMode = RtsUnitMode::AttackUnit;
  m_bModeChanged = true;
}

void RtsUnitComponent::OnMsgApplyDamage(RtsMsgApplyDamage& msg)
{
  ezInt32 lastHealth = m_uiCurHealth;

  if (msg.m_iDamage >= m_uiCurHealth)
  {
    m_uiCurHealth = 0;
  }
  else
  {
    m_uiCurHealth -= (ezInt16)msg.m_iDamage;
  }

  // in case damage was negative
  m_uiCurHealth = ezMath::Min(m_uiCurHealth, m_uiMaxHealth);

  RtsMsgUnitHealthStatus msg2;
  msg2.m_uiCurHealth = m_uiCurHealth;
  msg2.m_uiMaxHealth = m_uiMaxHealth;
  msg2.m_iDifference = (m_uiCurHealth - lastHealth);

  // theoretically the sub-systems could give us a health boost (or additional damage) here
  GetOwner()->SendMessageRecursive(msg2);

  if (m_uiCurHealth == 0)
  {
    OnUnitDestroyed();
  }
}

void RtsUnitComponent::OnMsgGatherUnitStats(RtsMsgGatherUnitStats& msg)
{
  msg.m_uiCurHealth = m_uiCurHealth;
  msg.m_uiMaxHealth = m_uiMaxHealth;
}


void RtsUnitComponent::OnMsgArrivedAtLocation(RtsMsgArrivedAtLocation& msg)
{
  if (m_UnitMode == RtsUnitMode::MoveToPosition)
  {
    m_UnitMode = RtsUnitMode::GuardLocation;
    m_vAssignedPosition = GetOwner()->GetGlobalPosition().GetAsVec2();
  }
}

void RtsUnitComponent::OnUnitDestroyed()
{
  if (m_hOnDestroyedPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pPrefab(m_hOnDestroyedPrefab);

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), ezGameObjectHandle(), nullptr, &GetOwner()->GetTeamID(), nullptr);
  }

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

void RtsUnitComponent::UpdateUnit()
{
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  // Unit state machine for defining what the unit is 'supposed to do'
  // affects what the unit decides to actually do through the utility system
  switch (m_UnitMode)
  {
    case RtsUnitMode::GuardLocation:
      break;

    case RtsUnitMode::MoveToPosition:
      break;

    case RtsUnitMode::AttackUnit:
    {
      // if the target unit is dead, revert to guarding the current location

      ezGameObject* pTarget;
      if (!GetWorld()->TryGetObject(m_hAssignedUnitToAttack, pTarget) || pTarget == GetOwner())
      {
        m_UnitMode = RtsUnitMode::GuardLocation;
        m_vAssignedPosition = GetOwner()->GetGlobalPosition().GetAsVec2();

        // could add an offset in the current travel direction for smoother stops
      }

      break;
    }
  }

  m_pAiSystem->Reevaluate(GetOwner(), this, tNow, m_bModeChanged ? ezTime() : ezTime::Seconds(0.5));
  m_pAiSystem->Execute(GetOwner(), this, tNow);

  m_bModeChanged = false;
}

void RtsUnitComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // 0 means 'whatever max Health is set to'
  if (m_uiCurHealth == 0)
    m_uiCurHealth = m_uiMaxHealth;

  m_uiCurHealth = ezMath::Min(m_uiCurHealth, m_uiMaxHealth);
  m_vAssignedPosition = GetOwner()->GetGlobalPosition().GetAsVec2();

  // Setup the AI system
  {
    m_pAiSystem = EZ_DEFAULT_NEW(RtsAiUtilitySystem);

    {
      ezUniquePtr<RtsGuardLocationAiUtility> pUtility = EZ_DEFAULT_NEW(RtsGuardLocationAiUtility);
      m_pAiSystem->AddUtility(std::move(pUtility));
    }

    {
      ezUniquePtr<RtsAttackUnitAiUtility> pUtility = EZ_DEFAULT_NEW(RtsAttackUnitAiUtility);
      m_pAiSystem->AddUtility(std::move(pUtility));
    }

    {
      ezUniquePtr<RtsMoveToPositionAiUtility> pUtility = EZ_DEFAULT_NEW(RtsMoveToPositionAiUtility);
      m_pAiSystem->AddUtility(std::move(pUtility));
    }
  }
}

void RtsUnitComponent::SetOnDestroyedPrefab(const char* szPrefab)
{
  ezPrefabResourceHandle hPrefab;

  if (!ezStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(szPrefab);
  }

  m_hOnDestroyedPrefab = hPrefab;
}

const char* RtsUnitComponent::GetOnDestroyedPrefab() const
{
  if (!m_hOnDestroyedPrefab.IsValid())
    return "";

  return m_hOnDestroyedPrefab.GetResourceID();
}

//////////////////////////////////////////////////////////////////////////

RtsUnitComponentManager::RtsUnitComponentManager(ezWorld* pWorld)
    : ezComponentManager<class RtsUnitComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

void RtsUnitComponentManager::Initialize()
{
  // configure this system to update all components multi-threaded (async phase)

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(RtsUnitComponentManager::UnitUpdate, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
  //desc.m_uiGranularity = 8;

  RegisterUpdateFunction(desc);
}

void RtsUnitComponentManager::UnitUpdate(const ezWorldModule::UpdateContext& context)
{
  if (RtsGameState::GetSingleton() == nullptr || RtsGameState::GetSingleton()->GetActiveGameMode() != RtsActiveGameMode::BattleMode)
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
    {
      it->UpdateUnit();
    }
  }
}

ezGameObject* RtsUnitComponent::FindClosestEnemy(float fMaxRadius) const
{
  struct Payload
  {
    ezGameObject* pBestObject = nullptr;
    float fBestDistSQR;
    ezVec3 m_vOwnPosition;
    ezUInt16 m_uiOwnTeamID;
  };

  Payload pl;
  pl.fBestDistSQR = ezMath::Square(fMaxRadius);
  pl.m_vOwnPosition = GetOwner()->GetGlobalPosition();
  pl.m_uiOwnTeamID = GetOwner()->GetTeamID();

  ezSpatialSystem::QueryCallback cb = [&pl](ezGameObject* pObject) -> ezVisitorExecution::Enum {

    if (pObject->GetTeamID() == pl.m_uiOwnTeamID)
      return ezVisitorExecution::Skip;

    RtsUnitComponent* pUnit = nullptr;
    if (pObject->TryGetComponentOfBaseType(pUnit))
    {
      const float dist = (pObject->GetGlobalPosition() - pl.m_vOwnPosition).GetLengthSquared();

      if (dist < pl.fBestDistSQR)
      {
        pl.fBestDistSQR = dist;
        pl.pBestObject = pObject;
      }
    }

    return ezVisitorExecution::Continue;
  };

  RtsGameState::GetSingleton()->InspectObjectsInArea(pl.m_vOwnPosition.GetAsVec2(), fMaxRadius, cb);

  return pl.pBestObject;
}

void RtsUnitComponent::FireAt(ezGameObjectHandle hUnit)
{
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (tNow - m_TimeLastShot >= ezTime::Seconds(0.75))
  {
    m_TimeLastShot = tNow;

    RtsMsgSetTarget msg;
    msg.m_hObject = hUnit;

    ezGameObject* pSpawned = RtsGameState::GetSingleton()->SpawnNamedObjectAt(GetOwner()->GetGlobalTransform(), "ProtonTorpedo1", GetOwner()->GetTeamID());

    pSpawned->PostMessage(msg, ezObjectMsgQueueType::AfterInitialized);
  }
}

bool RtsUnitComponent::AttackClosestEnemey(float fSearchRadius, float fIgnoreRadius)
{
  if (m_hCurrentUnitToAttack.IsInvalidated())
  {
    // TODO: don't check all the time

    if (ezGameObject* pEnemy = FindClosestEnemy(fSearchRadius))
    {
      m_hCurrentUnitToAttack = pEnemy->GetHandle();
    }
  }

  if (m_hCurrentUnitToAttack.IsInvalidated())
    return false;

  ezGameObject* pEnemy = nullptr;
  if (!GetWorld()->TryGetObject(m_hCurrentUnitToAttack, pEnemy))
  {
    m_hCurrentUnitToAttack.Invalidate();
    return false;
  }

  if ((pEnemy->GetGlobalPosition() - GetOwner()->GetGlobalPosition()).GetLengthSquared() > ezMath::Square(fIgnoreRadius))
  {
    m_hCurrentUnitToAttack.Invalidate();
    return false;
  }

  FireAt(m_hCurrentUnitToAttack);
  return true;
}
