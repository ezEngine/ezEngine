#include <PCH.h>
// Blank line to prevent Clang format from reordering this
#include <Foundation/Utilities/Stats.h>
#include <RtsGamePlugin/Components/UnitComponent.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(RtsUnitComponent, 1, ezComponentMode::Dynamic)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(RtsMsgSetTarget, OnMsgSetTarget),
    EZ_MESSAGE_HANDLER(RtsMsgNavigateTo, OnMsgNavigateTo),
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
}

void RtsUnitComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();
}

void RtsUnitComponent::OnMsgNavigateTo(RtsMsgNavigateTo& msg)
{
  m_UnitMode = RtsUnitMode::Idle;
}

void RtsUnitComponent::OnMsgSetTarget(RtsMsgSetTarget& msg)
{
  m_vShootAtPosition = msg.m_vPosition;
  m_hShootAtUnit = msg.m_hObject;

  if (!m_hShootAtUnit.IsInvalidated())
    m_UnitMode = RtsUnitMode::ShootAtUnit;
  else
    m_UnitMode = RtsUnitMode::ShootAtPosition;
}

void RtsUnitComponent::UpdateUnit()
{
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_UnitMode == RtsUnitMode::ShootAtUnit)
  {
    // check whether the target has died or is invalid

    ezGameObject* pTarget = nullptr;
    if (!GetWorld()->TryGetObject(m_hShootAtUnit, pTarget) || pTarget == GetOwner())
    {
      m_hShootAtUnit.Invalidate();
      m_UnitMode = RtsUnitMode::Idle;
    }
  }

  if (m_UnitMode == RtsUnitMode::ShootAtPosition || m_UnitMode == RtsUnitMode::ShootAtUnit)
  {
    if (tNow - m_TimeLastShot >= ezTime::Seconds(0.75))
    {
      m_TimeLastShot = tNow;

      RtsMsgSetTarget msg;

      if (m_UnitMode == RtsUnitMode::ShootAtUnit)
        msg.m_hObject = m_hShootAtUnit;
      else
        msg.m_vPosition = m_vShootAtPosition;

      ezGameObject* pSpawned = RtsGameState::GetSingleton()->SpawnNamedObjectAt(GetOwner()->GetGlobalTransform(), "ProtonTorpedo1", GetOwner()->GetTeamID());

      pSpawned->PostMessage(msg, ezObjectMsgQueueType::AfterInitialized);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

RtsUnitComponentManager::RtsUnitComponentManager(ezWorld* pWorld)
    : ezComponentManager<class RtsUnitComponent, ezBlockStorageType::Compact>(pWorld)
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
  if (RtsGameState::GetSingleton()->GetActiveGameMode() != RtsActiveGameMode::BattleMode)
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
    {
      it->UpdateUnit();
    }
  }
}
