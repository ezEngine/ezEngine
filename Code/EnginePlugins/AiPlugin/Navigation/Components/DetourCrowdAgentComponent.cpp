#include <AiPlugin/AiPluginPCH.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <DetourCrowd.h>
#include <AiPlugin/Utils/RcMath.h>
#include <AiPlugin/Navigation/Components/DetourCrowdAgentComponent.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Rect.h>

ezCVarBool cvar_DetourCrowdVisAgents("DetourCrowd.Debug.VisAgents", false, ezCVarFlags::Default, "Draws DetourCrowd agents, if any");
ezCVarBool cvar_DetourCrowdVisCorners("DetourCrowd.Debug.VisCorners", false, ezCVarFlags::Default, "Draws next few path corners of the DetourCrowd agents");
ezCVarBool cvar_DetourCrowdVisDestination("DetourCrowd.Debug.VisDestination", false, ezCVarFlags::Default, "Draws destination points of the DetourCrowd agents");

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezDetourCrowdAgentRotationMode, 1)
  EZ_ENUM_CONSTANTS(ezDetourCrowdAgentRotationMode::LookAtNextPathCorner, ezDetourCrowdAgentRotationMode::MatchVelocityDirection)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezDetourCrowdAgentComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Navigation"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new ezDynamicStringEnumAttribute("AiNavmeshConfig")),
    EZ_MEMBER_PROPERTY("Radius",m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.3f),new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height",m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.8f),new ezClampValueAttribute(0.01f, ezVariant())),
    EZ_MEMBER_PROPERTY("MaxSpeed",m_fMaxSpeed)->AddAttributes(new ezDefaultValueAttribute(3.5f),new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("MaxAcceleration",m_fMaxAcceleration)->AddAttributes(new ezDefaultValueAttribute(10.0f),new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("StoppingDistance",m_fStoppingDistance)->AddAttributes(new ezDefaultValueAttribute(0.3f),new ezClampValueAttribute(0.001f, ezVariant())),
    EZ_MEMBER_PROPERTY("MaxAngularSpeed",m_MaxAngularSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(360.0f)),new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_MEMBER_PROPERTY("RotationMode",ezDetourCrowdAgentRotationMode,m_RotationMode),
    EZ_MEMBER_PROPERTY("Pushiness",m_fPushiness)->AddAttributes(new ezDefaultValueAttribute(1.0f),new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetDestination, In, "Destination", In, "AllowPartialPaths"),
    EZ_SCRIPT_FUNCTION_PROPERTY(CancelNavigation),
    EZ_SCRIPT_FUNCTION_PROPERTY(HasDestination),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezDetourCrowdAgentComponent::ezDetourCrowdAgentComponent()
{
  m_uiHasDestinationBit = 0;
  m_uiDestinationChangedBit = 0;
  m_uiSteeringFailedBit = 0;
  m_uiParamsChangedBit = 0;
  m_uiAllowPartialPathBit = 0;
}

ezDetourCrowdAgentComponent::~ezDetourCrowdAgentComponent() = default;

void ezDetourCrowdAgentComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_sNavmeshConfig;
  s << m_fRadius;
  s << m_fHeight;
  s << m_fMaxSpeed;
  s << m_fMaxAcceleration;
  s << m_fStoppingDistance;
  s << m_MaxAngularSpeed;
  s << m_RotationMode;
  s << m_fPushiness;
}

void ezDetourCrowdAgentComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_sNavmeshConfig;
  s >> m_fRadius;
  s >> m_fHeight;
  s >> m_fMaxSpeed;
  s >> m_fMaxAcceleration;
  s >> m_fStoppingDistance;
  s >> m_MaxAngularSpeed;
  s >> m_RotationMode;
  s >> m_fPushiness;
}

void ezDetourCrowdAgentComponent::SetRadius(float fRadius)
{
  m_fRadius = ezMath::Max(fRadius, 0.0f);
  m_uiParamsChangedBit = 1;
}

void ezDetourCrowdAgentComponent::SetHeight(float fHeight)
{
  m_fHeight = ezMath::Max(fHeight, 0.01f);
  m_uiParamsChangedBit = 1;
}

void ezDetourCrowdAgentComponent::SetMaxSpeed(float fMaxSpeed)
{
  m_fMaxSpeed = ezMath::Max(fMaxSpeed, 0.0f);
  m_uiParamsChangedBit = 1;
}

void ezDetourCrowdAgentComponent::SetMaxAcceleration(float fMaxAcceleration)
{
  m_fMaxAcceleration = ezMath::Max(fMaxAcceleration, 0.0f);
  m_uiParamsChangedBit = 1;
}

void ezDetourCrowdAgentComponent::SetStoppingDistance(float fStoppingDistance)
{
  m_fStoppingDistance = ezMath::Max(fStoppingDistance, 0.001f);
}

void ezDetourCrowdAgentComponent::SetMaxAngularSpeed(ezAngle maxAngularSpeed)
{
  if (maxAngularSpeed.GetRadian() < 0.0f)
    maxAngularSpeed.SetRadian(0.0f);

  m_MaxAngularSpeed = maxAngularSpeed;
}

void ezDetourCrowdAgentComponent::SetPushiness(float fPushiness)
{
  m_fPushiness = ezMath::Max(fPushiness, 0.0f);
  m_uiParamsChangedBit = 1;
}

void ezDetourCrowdAgentComponent::SetDestination(const ezVec3& vGlobalPos, bool bAllowPartialPath)
{
  auto* pNavMeshModule = GetWorld()->GetOrCreateModule<ezAiNavMeshWorldModule>();
  auto* pNavMesh = pNavMeshModule->GetNavMesh(m_sNavmeshConfig);

  if (pNavMesh)
  {
    m_uiAllowPartialPathBit = bAllowPartialPath ? 1 : 0;
    m_uiSteeringFailedBit = 0;
    m_vDestination = vGlobalPos;
    m_uiDestinationChangedBit = 1;
    m_uiHasDestinationBit = 1;

    ezRectFloat r = ezRectFloat::MakeInvalid();
    r.ExpandToInclude(GetOwner()->GetGlobalPosition().GetAsVec2());
    r.ExpandToInclude(vGlobalPos.GetAsVec2());
    r.Grow(ezAiNavigation::c_fPathSearchBoundary);

    pNavMesh->RequestSector(r.GetCenter(), r.GetHalfExtents());
  }
  else
  {
    ezLog::Error("NavMesh '{}' does not exist (referenced by '{}')", m_sNavmeshConfig, GetOwner()->GetName());
  }
}

void ezDetourCrowdAgentComponent::CancelNavigation()
{
  m_uiDestinationChangedBit = m_uiHasDestinationBit;
  m_uiHasDestinationBit = 0;
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

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezDetourCrowdAgentComponentManager::AsyncUpdate, this);
    desc.m_Phase = ezWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezDetourCrowdAgentComponentManager::SyncTransforms, this);
    desc.m_Phase = ezWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(desc);
  }
}

void ezDetourCrowdAgentComponentManager::FillDtCrowdAgentParams(const ezDetourCrowdAgentComponent* pAgent, dtCrowdAgentParams& out_params)
{
  out_params.radius = pAgent->GetRadius();
  out_params.height = pAgent->GetHeight();
  out_params.maxAcceleration = pAgent->GetMaxAcceleration();
  out_params.maxSpeed = pAgent->GetMaxSpeed();
  out_params.collisionQueryRange = ezMath::Max(1.2f, 12.0f * out_params.radius);
  out_params.pathOptimizationRange = ezMath::Max(3.0f, 30.0f * out_params.radius);
  out_params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO
    | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
  out_params.obstacleAvoidanceType = 3;
  out_params.separationWeight = pAgent->GetPushiness();
  out_params.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(pAgent->m_uiUniqueAgentId));
}

dtCrowdAgent* ezDetourCrowdAgentComponentManager::TryGetValidDtAgent(dtCrowd* pDtCrowd, const ezDetourCrowdAgentComponent* pAgent)
{
  dtCrowdAgent* pDtAgent = pAgent->m_uiAgentId ? pDtCrowd->getEditableAgent(static_cast<int>(pAgent->m_uiAgentId - 1)) : nullptr;

  if (pDtAgent && pDtAgent->active && static_cast<ezUInt32>(reinterpret_cast<std::uintptr_t>(pDtAgent->params.userData)) == pAgent->m_uiUniqueAgentId)
    return pDtAgent;

  return nullptr;
}

void ezDetourCrowdAgentComponentManager::AsyncUpdate(const UpdateContext& ctx)
{
  auto* pNavMeshModule = GetWorld()->GetModuleReadOnly<ezAiNavMeshWorldModule>();
  if (!pNavMeshModule)
    return;

  // For each dtCrowd, check if the underlying navmesh has changed
  for (auto pair : m_CrowdPerNavMesh)
  {
    const ezAiNavMesh* pNavMesh = pNavMeshModule->GetNavMesh(pair.key);
    if (!pNavMesh) continue;

    if (pNavMesh->GetDetourNavMesh() != pair.value->getNavMeshQuery()->getAttachedNavMesh())
    {
      const ezInt32 iMaxAgents = 128;
      const float fMaxAgentRadius = pNavMesh->GetConfig().m_fAgentRadius;

      pair.value->init(iMaxAgents, fMaxAgentRadius, const_cast<dtNavMesh*>(pNavMesh->GetDetourNavMesh()));
    }
  }

  for (auto it = this->m_ComponentStorage.GetIterator(ctx.m_uiFirstComponentIndex, ctx.m_uiComponentCount); it.IsValid(); ++it)
  {
    ezDetourCrowdAgentComponent* pAgent = it;

    if (pAgent->IsActiveAndSimulating())
    {
      // Fetch or create a dtCrowd for agent's navmesh config
      dtCrowd* pDtCrowd = nullptr;
      if (pAgent->m_uiCrowdId)
      {
        pDtCrowd = m_CrowdPerNavMesh.GetValue(pAgent->m_uiCrowdId - 1);
      }
      else
      {
        ezUInt32 uiCrowdIdx = m_CrowdPerNavMesh.Find(pAgent->m_sNavmeshConfig);
        if (uiCrowdIdx != ezInvalidIndex)
        {
          pDtCrowd = m_CrowdPerNavMesh.GetValue(uiCrowdIdx);
          pAgent->m_uiCrowdId = uiCrowdIdx + 1;
        }
        else if (const ezAiNavMesh* pNavMesh = pNavMeshModule->GetNavMesh(pAgent->m_sNavmeshConfig))
        {
          pDtCrowd = dtAllocCrowd();

          const ezInt32 iMaxAgents = 128;
          const float fMaxAgentRadius = pNavMesh->GetConfig().m_fAgentRadius;

          pDtCrowd->init(iMaxAgents, fMaxAgentRadius, const_cast<dtNavMesh*>(pNavMesh->GetDetourNavMesh()));

          pAgent->m_uiCrowdId = m_CrowdPerNavMesh.Insert(pAgent->m_sNavmeshConfig, pDtCrowd) + 1;
        }
      }

      if (!pDtCrowd)
      {
        continue;
      }

      ezInt32 iAgentId = static_cast<ezInt32>(pAgent->m_uiAgentId - 1);
      dtCrowdAgent* pDtAgent = TryGetValidDtAgent(pDtCrowd, pAgent);

      // If an agent was created out of navmesh, which is entirely possible because we create navmesh on demand,
      // then it enters the invalid state. The only way to exit the invalid state is to recreate the agent.
      if (pDtAgent && pDtAgent->state == DT_CROWDAGENT_STATE_INVALID)
      {
        pDtCrowd->removeAgent(iAgentId);
        pDtAgent = nullptr;
        pAgent->m_uiHasDestinationBit = 0;
      }

      // If ezAgent doesn't have a corresponding dtAgent, create one
      if (!pDtAgent)
      {
        pAgent->m_uiUniqueAgentId = ++m_uiNextUniqueId;

        dtCrowdAgentParams dtParams{};
        FillDtCrowdAgentParams(pAgent, dtParams);

        iAgentId = pDtCrowd->addAgent(ezRcPos(pAgent->GetOwner()->GetGlobalPosition()), &dtParams);

        if (iAgentId == -1)
        {
          ezLog::Warning("Couldn't create DetourCrowd agent for '{0}'. The component will be disabled.", pAgent->GetOwner()->GetName());
          pAgent->m_uiAgentId = 0;
          pAgent->SetActiveFlag(false);
          continue;
        }

        pDtAgent = pDtCrowd->getEditableAgent(iAgentId);

        pAgent->m_uiAgentId = iAgentId + 1;
        pAgent->m_uiParamsChangedBit = 0;
      }

      // Update dtAgent's parameters if any of the ezAgent's properties (Height, Radius, etc) changed
      if (pAgent->m_uiParamsChangedBit)
      {
        pAgent->m_uiParamsChangedBit = 0;

        dtCrowdAgentParams dtParams{};
        FillDtCrowdAgentParams(pAgent, dtParams);

        pDtCrowd->updateAgentParameters(iAgentId, &dtParams);
      }

      if (pAgent->m_uiDestinationChangedBit)
      {
        pAgent->m_uiDestinationChangedBit = 0;

        if (pAgent->m_uiHasDestinationBit)
        {
          float vNavPos[3];
          dtPolyRef navPolyRef = 0;
          ezVec3 vQueryHalfExtents = ezVec3(1, 1, 2);

          // Grow search extents until a polygon is found, up to 3 attempts.
          for (int iTry = 0; iTry < 3 && navPolyRef == 0; ++iTry, vQueryHalfExtents *= 2.0f)
          {
            pDtCrowd->getNavMeshQuery()->findNearestPoly(ezRcPos(pAgent->m_vDestination), ezRcPos(vQueryHalfExtents), pDtCrowd->getFilter(0), &navPolyRef, vNavPos);
          }

          if (navPolyRef != 0)
          {
            pDtCrowd->requestMoveTarget(iAgentId, navPolyRef, vNavPos);
            pAgent->m_vActualDestination = ezRcPos(pDtAgent->targetPos);
          }
          else
          {
            pDtCrowd->resetMoveTarget(iAgentId);
            pAgent->m_uiHasDestinationBit = 0;
            if (!pAgent->m_uiAllowPartialPathBit)
              pAgent->m_uiSteeringFailedBit = 1;
          }
        }
        else
        {
          pDtCrowd->resetMoveTarget(iAgentId);
        }
      }

      // Check if we've reached the destination
      if (pAgent->m_uiHasDestinationBit)
      {
        ezVec3 vTargetPos = ezRcPos(pDtAgent->targetPos);
        if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_VALID)
          vTargetPos = ezRcPos(pDtAgent->corridor.getTarget());
        const float fDistSquared = vTargetPos.GetSquaredDistanceTo(ezRcPos(pDtAgent->npos));

        if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_FAILED)
        {
          pDtCrowd->resetMoveTarget(iAgentId);
          pAgent->m_uiHasDestinationBit = 0;
          if (!pAgent->m_uiAllowPartialPathBit)
            pAgent->m_uiSteeringFailedBit = 1;
        }
        else if (fDistSquared < pAgent->m_fStoppingDistance * pAgent->m_fStoppingDistance)
        {
          pDtCrowd->resetMoveTarget(iAgentId);
          pAgent->m_uiHasDestinationBit = 0;
        }
      }
    }
    else if (pAgent->m_uiCrowdId && pAgent->m_uiAgentId)
    {
      // If ezAgent is inactive, but still has a corresponding dtAgent, destroy the dtAgent

      dtCrowd* pDtCrowd = m_CrowdPerNavMesh.GetValue(pAgent->m_uiCrowdId - 1);
      dtCrowdAgent* pDtAgent = TryGetValidDtAgent(pDtCrowd, pAgent);

      if (pDtAgent)
      {
        pDtCrowd->removeAgent(static_cast<int>(pAgent->m_uiAgentId - 1));
      }

      pAgent->m_uiAgentId = 0;
    }
  }

  // Update each dtCrowd
  const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
  for (auto pair : m_CrowdPerNavMesh)
  {
    pair.value->update(fDeltaTime, nullptr);
  }
}

void ezDetourCrowdAgentComponentManager::SyncTransforms(const UpdateContext& ctx)
{
  const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  // Sync each agent's position with corresponding dtAgent
  for (auto it = this->m_ComponentStorage.GetIterator(ctx.m_uiFirstComponentIndex, ctx.m_uiComponentCount); it.IsValid(); ++it)
  {
    ezDetourCrowdAgentComponent* pAgent = it;

    if (pAgent->IsActiveAndSimulating() && pAgent->m_uiCrowdId)
    {
      dtCrowd* pDtCrowd = m_CrowdPerNavMesh.GetValue(pAgent->m_uiCrowdId - 1);
      dtCrowdAgent* pDtAgent = TryGetValidDtAgent(pDtCrowd, pAgent);

      if (pDtAgent)
      {
        const ezVec3 vPosition = ezRcPos(pDtAgent->npos);
        const ezVec3 vVelocity = ezRcPos(pDtAgent->vel);

        ezVec3 vLookDir = pAgent->GetOwner()->GetGlobalDirForwards();
        vLookDir.z = 0;
        vLookDir.Normalize();

        ezVec3 vTargetDir = vVelocity;
        vTargetDir.z = 0;
        vTargetDir.NormalizeIfNotZero(vLookDir).IgnoreResult();

        if (pAgent->m_RotationMode == ezDetourCrowdAgentRotationMode::LookAtNextPathCorner && pDtAgent->ncorners > 0)
        {
          ezVec3 vNextCorner = ezRcPos(pDtAgent->cornerVerts);
          ezVec3 vDiff = vNextCorner - vPosition;
          vDiff.z = 0;

          if (vDiff.GetLengthSquared() > 0.001f)
          {
            vTargetDir = vDiff.GetNormalized();
          }
          else if (pDtAgent->ncorners > 1)
          {
            vNextCorner = ezRcPos(pDtAgent->cornerVerts + 3);
            vDiff = vNextCorner - vPosition;
            vDiff.z = 0;
            if (vDiff.GetLengthSquared() > 0.001f)
            {
              vTargetDir = vDiff.GetNormalized();
            }
          }
        }

        const float maxTurnAngle = fDeltaTime * pAgent->m_MaxAngularSpeed.GetRadian();
        float turnAngle = vLookDir.GetAngleBetween(vTargetDir, ezVec3::MakeAxisZ()).GetRadian();
        turnAngle = ezMath::Sign(turnAngle) * ezMath::Min(ezMath::Abs(turnAngle), maxTurnAngle);

        const ezQuat qRot = ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisZ(), ezAngle::MakeFromRadian(turnAngle));
        const ezVec3 vNewLookDir = qRot * pAgent->GetOwner()->GetGlobalDirForwards();

        ezTransform transform = pAgent->GetOwner()->GetGlobalTransform();
        transform.m_vPosition = vPosition;
        transform.m_qRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), vNewLookDir);
        pAgent->GetOwner()->SetGlobalTransform(transform);

        pAgent->m_vVelocity = vVelocity;
      }

      // Draw destination
      if (cvar_DetourCrowdVisDestination && pAgent->HasDestination())
      {
        ezDebugRendererLine line{};
        line.m_start = pAgent->GetOwner()->GetGlobalPosition();
        line.m_end = pAgent->m_vActualDestination;

        ezDebugRenderer::DrawLines(GetWorld(), ezArrayPtr(&line, 1), ezColor::DarkSalmon, ezTransform::Make(ezVec3(0.0f, 0.0f, 0.1f)));

        ezDebugRenderer::DrawLineSphere(GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 0.2f), ezColor::DarkSalmon, ezTransform::Make(pAgent->m_vActualDestination));
      }
    }
  }

  // Debug visualization
  if (cvar_DetourCrowdVisAgents)
  {
    for (auto pair : m_CrowdPerNavMesh)
    {
      const ezInt32 iNumAgents = pair.value->getAgentCount();
      for (int i = 0; i < iNumAgents; ++i)
      {
        const dtCrowdAgent* pDtAgent = pair.value->getAgent(i);
        if (pDtAgent->active)
        {
          const float fHeight = pDtAgent->params.height;
          const float fRadius = pDtAgent->params.radius;

          ezTransform transform(ezRcPos(pDtAgent->npos));
          transform.m_vPosition.z += fHeight * 0.5f;

          // Draw agent cylinder
          ezDebugRenderer::DrawLineCylinderZ(GetWorld(), fHeight, fRadius, ezColor::BlueViolet, transform);

          // Draw velocity arrow
          ezVec3 vVelocity = ezRcPos(pDtAgent->vel);
          vVelocity.z = 0;
          if (!vVelocity.IsZero())
          {
            vVelocity.Normalize();
            transform.m_qRotation = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vVelocity);
            ezDebugRenderer::DrawArrow(GetWorld(), 1.0f, ezColor::BlueViolet, transform);
          }

          // Draw path corners
          if (cvar_DetourCrowdVisCorners.GetValue() && pDtAgent->ncorners > 0)
          {
            ezDebugRendererLine lines[DT_CROWDAGENT_MAX_CORNERS];

            lines[0].m_start = ezRcPos(pDtAgent->npos);
            lines[0].m_end = ezRcPos(pDtAgent->cornerVerts);

            for (int i = 0; i < pDtAgent->ncorners - 1; ++i)
            {
              lines[i].m_start = ezRcPos(pDtAgent->cornerVerts + 3 * i);
              lines[i].m_end = ezRcPos(pDtAgent->cornerVerts + 3 * i + 3);
            }

            ezDebugRenderer::DrawLines(GetWorld(), ezArrayPtr(lines, pDtAgent->ncorners), ezColor::Cyan, ezTransform::Make(ezVec3(0.0f, 0.0f, 0.1f)));
          }
        }
      }
    }
  }
}
