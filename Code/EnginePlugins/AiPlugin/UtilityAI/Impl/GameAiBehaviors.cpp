#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Impl/GameAiBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <Foundation/Memory/InstanceDataAllocator.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorWithTargetPosition, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorGoToPOI, 1, ezRTTIDefaultAllocator<ezAiBehaviorGoToPOI>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("POIs", m_POIs)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionPOI"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Command, 0.5f),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorWander, 1, ezRTTIDefaultAllocator<ezAiBehaviorWander>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WanderPoints", m_WanderPoints)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionWander"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::ActiveIdle, 0.1f),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorGoToCheckpoint, 1, ezRTTIDefaultAllocator<ezAiBehaviorGoToCheckpoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Checkpoints", m_Checkpoints)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionCheckpoint"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::ActiveIdle, 0.2f),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorShoot, 1, ezRTTIDefaultAllocator<ezAiBehaviorShoot>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("POIs", m_POIs)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionPOI"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Combat, 0.1f),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorQuip, 1, ezRTTIDefaultAllocator<ezAiBehaviorQuip>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("POIs", m_POIs)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionPOI"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Command, 0.6f),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool ezAiBehaviorWithTargetPosition::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorGoToPOI::ezAiBehaviorGoToPOI() = default;

ezAiBehaviorGoToPOI::~ezAiBehaviorGoToPOI() = default;

void ezAiBehaviorGoToPOI::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_POIs);
}

ezAiBehaviorScore ezAiBehaviorGoToPOI::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const ezAiPerceptionManager& perceptionManager = ctx.m_PerceptionManager;
  ezGameObject& owner = ctx.m_Owner;

  if (!perceptionManager.HasPerceptions(m_POIs))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptions(m_POIs, perceptions);

  const ezVec3 vOwnerPos = owner.GetGlobalPosition();
  float fClosestSqr = ezMath::HighValue<float>();

  ezAiBehaviorScore res;

  for (const ezAiPerception* pPerception0 : perceptions)
  {
    const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

    const float fDistSqr = (pPerception->m_vGlobalPosition - vOwnerPos).GetLengthSquared();

    if (fDistSqr < fClosestSqr)
    {
      fClosestSqr = fDistSqr;
      res.m_pPerception = pPerception;
    }
  }

  if (res.m_pPerception == nullptr)
    return {};

  return MakeScore(ctx, res.m_pPerception);
}

void ezAiBehaviorGoToPOI::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception0) const
{
  ezGameObject& owner = ctx.m_Owner;
  ezAiActionQueue& inout_ActionQueue = ctx.m_ActionQueue;
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = ezAiActionLerpRotationTowards::Create();
  //  pAct->InstanceData* pData = static_cast<InstanceData*>(pInstanceData);
  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("MoveForwards");
    pCmd->m_Value = 1.0f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = ezAiActionCCMoveTo::Create();
  //  pAct->InstanceData* pData = static_cast<InstanceData*>(pInstanceData);
  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 1.0f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = ezAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &pData->m_vTargetPosition;
    pAct->m_fSpeed = 3.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

void ezAiBehaviorGoToPOI::OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception0) const
{
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorWander::ezAiBehaviorWander() = default;

ezAiBehaviorWander::~ezAiBehaviorWander() = default;

void ezAiBehaviorWander::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_WanderPoints);
}

ezAiBehaviorScore ezAiBehaviorWander::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const ezAiPerceptionManager& perceptionManager = ctx.m_PerceptionManager;
  ezGameObject& owner = ctx.m_Owner;

  if (!perceptionManager.HasPerceptions(m_WanderPoints))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptions(m_WanderPoints, perceptions);

  if (perceptions.IsEmpty())
    return {};

  const ezUInt32 uiPerceptionIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(perceptions.GetCount());

  return MakeScore(ctx, perceptions[uiPerceptionIdx]);
}

void ezAiBehaviorWander::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception0) const
{
  ezGameObject& owner = ctx.m_Owner;
  ezAiActionQueue& inout_ActionQueue = ctx.m_ActionQueue;
  const ezAiPerceptionWander* pPerception = static_cast<const ezAiPerceptionWander*>(pPerception0);
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = ezAiActionLerpRotationTowards::Create();
  //  pAct->InstanceData* pData = static_cast<InstanceData*>(pInstanceData);
  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("MoveForwards");
    pCmd->m_Value = 0.5f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = ezAiActionCCMoveTo::Create();
  //  pAct->InstanceData* pData = static_cast<InstanceData*>(pInstanceData);
  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 0.5f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = ezAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &pData->m_vTargetPosition;
    pAct->m_fSpeed = 2.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorGoToCheckpoint::ezAiBehaviorGoToCheckpoint() = default;

ezAiBehaviorGoToCheckpoint::~ezAiBehaviorGoToCheckpoint() = default;

void ezAiBehaviorGoToCheckpoint::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Checkpoints);
}

ezAiBehaviorScore ezAiBehaviorGoToCheckpoint::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const ezAiPerceptionManager& perceptionManager = ctx.m_PerceptionManager;
  ezGameObject& owner = ctx.m_Owner;

  if (!perceptionManager.HasPerceptions(m_Checkpoints))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptions(m_Checkpoints, perceptions);

  if (perceptions.IsEmpty())
    return {};

  const ezUInt32 uiPerceptionIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(perceptions.GetCount());

  return MakeScore(ctx, perceptions[uiPerceptionIdx]);
}

void ezAiBehaviorGoToCheckpoint::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception0) const
{
  ezGameObject& owner = ctx.m_Owner;
  ezAiActionQueue& inout_ActionQueue = ctx.m_ActionQueue;
  const ezAiPerceptionCheckpoint* pPerception = static_cast<const ezAiPerceptionCheckpoint*>(pPerception0);
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = ezAiActionLerpRotationTowards::Create();
  //  pAct->InstanceData* pData = static_cast<InstanceData*>(pInstanceData);
  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("MoveForwards");
    pCmd->m_Value = 0.5f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = ezAiActionCCMoveTo::Create();
  //  pAct->InstanceData* pData = static_cast<InstanceData*>(pInstanceData);
  pData->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 0.5f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = ezAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &pData->m_vTargetPosition;
    pAct->m_fSpeed = 2.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorShoot::ezAiBehaviorShoot() = default;

ezAiBehaviorShoot::~ezAiBehaviorShoot() = default;

void ezAiBehaviorShoot::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_POIs);
}

ezAiBehaviorScore ezAiBehaviorShoot::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const ezAiPerceptionManager& perceptionManager = ctx.m_PerceptionManager;
  ezGameObject& owner = ctx.m_Owner;

  if (!perceptionManager.HasPerceptions(m_POIs))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptions(m_POIs, perceptions);

  const ezVec3 vOwnerPos = owner.GetGlobalPosition();
  const ezVec3 vOwnerDir = owner.GetGlobalDirForwards();
  float fClosest = ezMath::HighValue<float>();

  ezAiBehaviorScore res;

  for (const ezAiPerception* pPerception0 : perceptions)
  {
    const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

    ezVec3 vDirTo = pPerception->m_vGlobalPosition - vOwnerPos;
    const float fDist = vDirTo.GetLengthAndNormalize();

    if (fDist < 2.0f || fDist > 5.0f)
      continue;

    if (fDist >= fClosest)
      continue;

    fClosest = fDist;
    res.m_pPerception = pPerception;
  }

  if (res.m_pPerception == nullptr)
    return {};

  return MakeScore(ctx, res.m_pPerception);
}

void ezAiBehaviorShoot::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception0) const
{
  ezGameObject& owner = ctx.m_Owner;
  ezAiActionQueue& inout_ActionQueue = ctx.m_ActionQueue;
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

  inout_ActionQueue.CancelCurrentActions(owner);

  {
    auto pAct = ezAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
    pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("Aim");
    pCmd->m_Value = 1;
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionWait::Create();
    pCmd->m_Duration = ezTime::Seconds(0.5);
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto pAct = ezAiActionSpawn::Create();
    pAct->m_sChildObjectName = ezTempHashedString("Spawn");
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetAndWait::Create();
    pCmd->m_sEntryName = ezMakeHashedString("Shoot");
    pCmd->m_SetValue = 1;
    pCmd->m_WaitValue = 0;
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionWait::Create();
    pCmd->m_Duration = ezTime::Seconds(0.5);
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezMakeHashedString("Aim");
    pCmd->m_Value = 0;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorQuip::ezAiBehaviorQuip() = default;

ezAiBehaviorQuip::~ezAiBehaviorQuip() = default;

void ezAiBehaviorQuip::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_POIs);
}

ezAiBehaviorScore ezAiBehaviorQuip::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const ezAiPerceptionManager& perceptionManager = ctx.m_PerceptionManager;

  if (!perceptionManager.HasPerceptions(m_POIs))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptions(m_POIs, perceptions);

  return MakeScore(ctx);
}

void ezAiBehaviorQuip::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception0) const
{
  ezGameObject& owner = ctx.m_Owner;
  ezAiActionQueue& inout_ActionQueue = ctx.m_ActionQueue;
  inout_ActionQueue.CancelCurrentActions(owner);

  {
    auto pAct = ezAiActionQuip::Create();
    pAct->m_sLogMsg = "I SEE YOU !";
    inout_ActionQueue.QueueAction(pAct);
  }
}

ezTime ezAiBehaviorQuip::GetCooldownDuration(const void* pInstanceData) const
{
  return ezTime::Seconds(5);
}
