#include <PCH.h>

#include <Foundation/Utilities/Stats.h>
#include <RtsGamePlugin/AI/HuntEnemyUtility.h>
#include <RtsGamePlugin/AI/ShootAtUtility.h>
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
  m_UnitMode = RtsUnitMode::Idle;
  m_vAssignedPosition = msg.m_vTargetPosition;
}

void RtsUnitComponent::OnMsgSetTarget(RtsMsgSetTarget& msg)
{
  m_vAssignedPosition = msg.m_vPosition;
  m_hAssignedUnitToAttack = msg.m_hObject;

  if (!m_hAssignedUnitToAttack.IsInvalidated())
    m_UnitMode = RtsUnitMode::AttackUnit;
  else
    m_UnitMode = RtsUnitMode::ShootAtPosition;
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

  m_pAiSystem->Reevaluate(GetOwner(), this, tNow, ezTime::Seconds(0.5));
  m_pAiSystem->Execute(GetOwner(), this, tNow);
}

void RtsUnitComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // 0 means 'whatever max Health is set to'
  if (m_uiCurHealth == 0)
    m_uiCurHealth = m_uiMaxHealth;

  m_uiCurHealth = ezMath::Min(m_uiCurHealth, m_uiMaxHealth);
  m_vAssignedPosition = GetOwner()->GetGlobalPosition().GetAsVec2();
  m_vAssignedShootAtPosition.SetZero();

  // Setup the AI system
  {
    m_pAiSystem = EZ_DEFAULT_NEW(RtsAiUtilitySystem);

    {
      ezUniquePtr<RtsHuntEnemyAiUtility> pUtility = EZ_DEFAULT_NEW(RtsHuntEnemyAiUtility);
      m_pAiSystem->AddUtility(std::move(pUtility));
    }

    {
      ezUniquePtr<RtsShootAtAiUtility> pUtility = EZ_DEFAULT_NEW(RtsShootAtAiUtility);
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
