#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RecastPlugin/Utils/RcMath.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <DetourCrowd.h>
#include <RecastPlugin/WorldModule/DetourCrowdWorldModule.h>
#include <RecastPlugin/Components/DetourCrowdAgentComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezDetourCrowdAgentComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Recast"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius",m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
    EZ_MEMBER_PROPERTY("Height",m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.8f)),
    EZ_MEMBER_PROPERTY("MaxSpeed",m_fMaxSpeed)->AddAttributes(new ezDefaultValueAttribute(3.5f)),
    EZ_MEMBER_PROPERTY("MaxAcceleration",m_fMaxAcceleration)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
    EZ_MEMBER_PROPERTY("StoppingDistance",m_fStoppingDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("ApplyRotation",m_bApplyRotation)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezDetourCrowdAgentComponent::ezDetourCrowdAgentComponent()
{
  m_uiTargetDirtyBit = 0;
  m_uiSteeringFailedBit = 0;
  m_uiErrorBit = 0;
}

ezDetourCrowdAgentComponent::~ezDetourCrowdAgentComponent() = default;

void ezDetourCrowdAgentComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_fRadius;
  s << m_fHeight;
  s << m_fMaxSpeed;
  s << m_fMaxAcceleration;
  s << m_fStoppingDistance;
  s << m_bApplyRotation;
}

void ezDetourCrowdAgentComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  ezStreamReader& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_fHeight;
  s >> m_fMaxSpeed;
  s >> m_fMaxAcceleration;
  s >> m_fStoppingDistance;
  s >> m_bApplyRotation;
}

void ezDetourCrowdAgentComponent::FillAgentParams(ezDetourCrowdAgentParams& out_params) const
{
  out_params.m_fRadius = m_fRadius;
  out_params.m_fHeight = m_fHeight;
  out_params.m_fMaxSpeed = m_fMaxSpeed;
  out_params.m_fMaxAcceleration = m_fMaxAcceleration;
}

void ezDetourCrowdAgentComponent::SetTargetPosition(const ezVec3& vPosition)
{
  m_vTargetPosition = vPosition;
  m_PathToTargetState = ezAgentPathFindingState::HasTargetWaitingForPath;
  m_uiTargetDirtyBit = 1;
}

void ezDetourCrowdAgentComponent::ClearTargetPosition()
{
  m_PathToTargetState = ezAgentPathFindingState::HasNoTarget;
}

void ezDetourCrowdAgentComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezCharacterControllerComponent* pCC = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<ezCharacterControllerComponent>(pCC))
  {
    m_hCharacterController = pCC->GetHandle();
  }
}

void ezDetourCrowdAgentComponent::SyncTransform(const ezVec3& vPosition, const ezVec3& vVelocity, bool bTeleport)
{
  if (m_hCharacterController.IsInvalidated())
  {
    m_vVelocity = vVelocity;

    ezTransform xform = GetOwner()->GetGlobalTransform();
    xform.m_vPosition = vPosition;
    if (m_bApplyRotation)
    {
      ezVec3 vDirection = vVelocity;
      vDirection.z = 0;
      if (!vDirection.IsZero())
      {
        vDirection.Normalize();
        xform.m_qRotation = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vDirection);
      }
    }
    GetOwner()->SetGlobalTransform(xform);
  }
  else
  {
    ezCharacterControllerComponent* pCharacter = nullptr;
    if (GetWorld()->TryGetComponent(m_hCharacterController, pCharacter))
    {
      if (!bTeleport)
      {
        const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

        ezVec3 vDiff = vPosition - GetOwner()->GetGlobalPosition();
        vDiff.z = 0;
        const float fDistance = vDiff.GetLengthAndNormalize();
        const float fSpeed = ezMath::Min(m_fMaxSpeed, fDistance / fDeltaTime);

        m_vVelocity = vDiff * fSpeed;

        if (m_bApplyRotation)
        {
          GetOwner()->SetGlobalRotation(ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vDiff));
        }

        if (fDistance < 0.01f)
          return;

        if (fDistance > 0.25f)
        {
          ezAgentSteeringEvent e;
          e.m_pComponent = this;
          e.m_Type = ezAgentSteeringEvent::ErrorSteeringFailed;
          m_SteeringEvents.Broadcast(e);

          ClearTargetPosition();
          m_uiSteeringFailedBit = 1;

          return;
        }

        const ezVec3 vRelativeVelocity = GetOwner()->GetGlobalRotation().GetInverse() * m_vVelocity;

        ezMsgMoveCharacterController msg;
        msg.m_fMoveForwards = ezMath::Max(0.0f, vRelativeVelocity.x);
        msg.m_fMoveBackwards = ezMath::Max(0.0f, -vRelativeVelocity.x);
        msg.m_fStrafeLeft = ezMath::Max(0.0f, -vRelativeVelocity.y);
        msg.m_fStrafeRight = ezMath::Max(0.0f, vRelativeVelocity.y);

        pCharacter->MoveCharacter(msg);
      }
      else
      {
        m_vVelocity = vVelocity;

        pCharacter->TeleportCharacter(vPosition);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezDetourCrowdAgentComponentManager::ezDetourCrowdAgentComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}
ezDetourCrowdAgentComponentManager::~ezDetourCrowdAgentComponentManager() = default;

void ezDetourCrowdAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  m_pDetourCrowdModule = GetWorld()->GetOrCreateModule<ezDetourCrowdWorldModule>();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezDetourCrowdAgentComponentManager::Update, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;

    RegisterUpdateFunction(desc);
  }
}

void ezDetourCrowdAgentComponentManager::Update(const ezWorldModule::UpdateContext& ctx)
{
  if (!m_pDetourCrowdModule->IsInitializedAndReady())
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(ctx.m_uiFirstComponentIndex, ctx.m_uiComponentCount); it.IsValid(); ++it)
  {
    ezDetourCrowdAgentComponent* pAgent = it;
    const dtCrowdAgent* pDtAgent = pAgent->m_iAgentId != -1 ? m_pDetourCrowdModule->GetAgentById(pAgent->m_iAgentId) : nullptr;

    if (pAgent->IsActiveAndSimulating())
    {
      bool bTeleport = false;

      // If active and sumulating ezAgent doesn't have a corresponding dtAgent, create one
      if (pDtAgent == nullptr || !pDtAgent->active || static_cast<ezUInt32>(reinterpret_cast<std::uintptr_t>(pDtAgent->params.userData)) != pAgent->m_uiOwnerId)
      {
        ezDetourCrowdAgentParams params = ezDetourCrowdAgentParams::Default();
        pAgent->FillAgentParams(params);
        params.m_pUserData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(m_uiNextOwnerId));

        ezInt32 iAgentId = m_pDetourCrowdModule->CreateAgent(pAgent->GetOwner()->GetGlobalPosition(), params);
        if (iAgentId == -1)
        {
          // Check m_uiErrorBit to prevent spamming into the log
          if (!pAgent->m_uiErrorBit)
          {
            ezLog::Warning("Couldn't create DetourCrowd agent for '{0}'", pAgent->GetOwner()->GetName());
            pAgent->m_uiErrorBit = 1;
          }
          continue;
        }

        pAgent->m_iAgentId = iAgentId;
        pAgent->m_uiOwnerId = m_uiNextOwnerId;
        pAgent->m_uiErrorBit = 0;

        m_uiNextOwnerId += 1;

        pDtAgent = m_pDetourCrowdModule->GetAgentById(pAgent->m_iAgentId);

        // If dtAgent was just created, we want to teleport ezAgent to its position
        bTeleport = true;
      }

      pAgent->SyncTransform(ezRcPos(pDtAgent->npos), ezRcPos(pDtAgent->vel), bTeleport);

      // Steering failed. Can only happen if ezAgent uses CharacterController for movement and it got out of sync with dtAgent
      // We can do nothing about it, so clear the target and delete the dtAgent (it will be recreated at correct postion next frame)
      if (pAgent->m_uiSteeringFailedBit)
      {
        m_pDetourCrowdModule->DestroyAgent(pAgent->m_iAgentId);
        pAgent->m_iAgentId = -1;
        pAgent->m_uiSteeringFailedBit = 0;
        pAgent->m_uiTargetDirtyBit = 0;
        pAgent->m_PathToTargetState = ezAgentPathFindingState::HasNoTarget;
      }

      switch (pAgent->m_PathToTargetState)
      {
        // If ezAgent has no target, but dtAgent has one, clear dtAgent's target
        case ezAgentPathFindingState::HasNoTarget:
          if (pDtAgent->targetState != DT_CROWDAGENT_TARGET_NONE && pDtAgent->targetState != DT_CROWDAGENT_TARGET_FAILED)
          {
            m_pDetourCrowdModule->ClearAgentTargetPosition(pAgent->m_iAgentId);

            ezAgentSteeringEvent e;
            e.m_pComponent = pAgent;
            e.m_Type = ezAgentSteeringEvent::TargetCleared;
            pAgent->m_SteeringEvents.Broadcast(e);
          }
          break;
        case ezAgentPathFindingState::HasTargetWaitingForPath:
          // If ezAgent has a target, but dtAgent has none, set target
          // Likewise, if ezAgent's target has changed since the last time (m_uiTargetDirtyBit is set), set target
          if (pAgent->m_uiTargetDirtyBit || pDtAgent->targetState == DT_CROWDAGENT_TARGET_NONE || pDtAgent->targetState == DT_CROWDAGENT_TARGET_FAILED)
          {
            m_pDetourCrowdModule->SetAgentTargetPosition(pAgent->m_iAgentId, pAgent->m_vTargetPosition);
            pAgent->m_uiTargetDirtyBit = 0;
          }
          
          // If ezAgent is waiting for path status, but dtAgent has already got it, sync it and fire events
          if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_VALID)
          {
            pAgent->m_PathToTargetState = ezAgentPathFindingState::HasTargetAndValidPath;

            ezAgentSteeringEvent e;
            e.m_pComponent = pAgent;
            e.m_Type = pDtAgent->partial ? ezAgentSteeringEvent::WarningNoFullPathToTarget : ezAgentSteeringEvent::PathToTargetFound;
            pAgent->m_SteeringEvents.Broadcast(e);
          }
          else if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_FAILED)
          {
            pAgent->m_PathToTargetState = ezAgentPathFindingState::HasTargetPathFindingFailed;

            ezAgentSteeringEvent e;
            e.m_pComponent = pAgent;
            e.m_Type = ezAgentSteeringEvent::ErrorNoPathToTarget;
            pAgent->m_SteeringEvents.Broadcast(e);
          }
          break;
        case ezAgentPathFindingState::HasTargetPathFindingFailed:
          // Pathfinding failed, do nothing
          break;
        case ezAgentPathFindingState::HasTargetAndValidPath:
          // If ezAgent thinks it has a path, but dtAgent has none (probably because it was deleted), repeat the process
          if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_NONE)
          {
            m_pDetourCrowdModule->SetAgentTargetPosition(pAgent->m_iAgentId, pAgent->m_vTargetPosition);
            pAgent->m_PathToTargetState = ezAgentPathFindingState::HasTargetWaitingForPath;
          }
          // If ezAgent and dtAgent both agree they have a target and a valid path, check if target is reached
          else
          {
            ezVec3 vTargetPos = ezRcPos(pDtAgent->targetPos);
            ezVec3 vDiff = vTargetPos - pAgent->GetOwner()->GetGlobalPosition();
            vDiff.z = 0;

            if (vDiff.GetLengthSquared() < pAgent->m_fStoppingDistance * pAgent->m_fStoppingDistance)
            {
              m_pDetourCrowdModule->ClearAgentTargetPosition(pAgent->m_iAgentId);

              pAgent->m_PathToTargetState = ezAgentPathFindingState::HasNoTarget;
              
              ezAgentSteeringEvent e;
              e.m_pComponent = pAgent;
              e.m_Type = ezAgentSteeringEvent::TargetReached;
              pAgent->m_SteeringEvents.Broadcast(e);
            }
          }
          break;
        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
      };
    }
    else
    {
      // If ezAgent is inactive, but still has a corresponding dtAgent, destroy the dtAgent
      if (pDtAgent)
      {
        // Only destroy dtAgent if it was actually owned by ezAgent (could be just stale m_iAgentId)
        if (pDtAgent->active && static_cast<ezUInt32>(reinterpret_cast<intptr_t>(pDtAgent->params.userData)) == pAgent->m_uiOwnerId)
        {
          m_pDetourCrowdModule->DestroyAgent(pAgent->m_iAgentId);
        }
        pAgent->m_iAgentId = -1;
        pAgent->m_uiTargetDirtyBit = 0;
        pAgent->m_uiSteeringFailedBit = 0;
        pAgent->m_PathToTargetState = ezAgentPathFindingState::HasNoTarget;
      }
    }
  }
}
