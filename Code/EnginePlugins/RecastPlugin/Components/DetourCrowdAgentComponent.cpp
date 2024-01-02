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
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezDetourCrowdAgentComponent::ezDetourCrowdAgentComponent() = default;
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
  m_bTargetDirty = true;
}

void ezDetourCrowdAgentComponent::ClearTargetPosition()
{
  m_PathToTargetState = ezAgentPathFindingState::HasNoTarget;
}

void ezDetourCrowdAgentComponent::OnActivated()
{
  SUPER::OnActivated();
}

void ezDetourCrowdAgentComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
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

void ezDetourCrowdAgentComponent::SyncTransform(const ezVec3& vPosition, const ezVec3& vVelocity)
{
  m_vVelocity = m_vVelocity;

  GetOwner()->SetGlobalPosition(vPosition);

  //ezSimdTransform xform = GetOwner()->GetGlobalTransformSimd();
  //xform.m_Position.Set(ezRcPos(pDtAgent->npos));
  //xform.m_Rotation.
  //GetOwner()->SetGlobalTransform(xform);
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

void ezDetourCrowdAgentComponentManager::Deinitialize()
{
  SUPER::Deinitialize();
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
      if (pDtAgent == nullptr || !pDtAgent->active || static_cast<ezUInt32>(reinterpret_cast<std::uintptr_t>(pDtAgent->params.userData)) != pAgent->m_uiOwnerId)
      {
        ezDetourCrowdAgentParams params = ezDetourCrowdAgentParams::Default();
        pAgent->FillAgentParams(params);
        params.m_pUserData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(m_uiNextOwnerId));

        pAgent->m_iAgentId = m_pDetourCrowdModule->CreateAgent(pAgent->GetOwner()->GetGlobalPosition(), params);
        pAgent->m_uiOwnerId = m_uiNextOwnerId;

        m_uiNextOwnerId += 1;

        pDtAgent = m_pDetourCrowdModule->GetAgentById(pAgent->m_iAgentId);
      }

      pAgent->SyncTransform(ezRcPos(pDtAgent->npos), ezRcPos(pDtAgent->vel));

      switch (pAgent->m_PathToTargetState)
      {
        case ezAgentPathFindingState::HasNoTarget:
          if (pDtAgent->targetState != DT_CROWDAGENT_TARGET_NONE)
            m_pDetourCrowdModule->ClearAgentTargetPosition(pAgent->m_iAgentId);
          break;
        case ezAgentPathFindingState::HasTargetWaitingForPath:
          if (pAgent->m_bTargetDirty || pDtAgent->targetState == DT_CROWDAGENT_TARGET_NONE)
          {
            m_pDetourCrowdModule->SetAgentTargetPosition(pAgent->m_iAgentId, pAgent->m_vTargetPosition);
          }
          else if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_VALID)
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
          // Nothing to do here
          break;
        case ezAgentPathFindingState::HasTargetAndValidPath:
          if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_NONE)
          {
            m_pDetourCrowdModule->SetAgentTargetPosition(pAgent->m_iAgentId, pAgent->m_vTargetPosition);
          }
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
      if (pDtAgent)
      {
        if (pDtAgent->active && static_cast<ezUInt32>(reinterpret_cast<intptr_t>(pDtAgent->params.userData)) == pAgent->m_uiOwnerId)
        {
          m_pDetourCrowdModule->DestroyAgent(pAgent->m_iAgentId);
        }
        pAgent->m_iAgentId = -1;
        pAgent->m_PathToTargetState = ezAgentPathFindingState::HasNoTarget;
      }
    }
  }
}
