#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <DetourCrowd.h>
#include <Foundation/Configuration/CVar.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/Utils/RcMath.h>
#include <RecastPlugin/WorldModule/DetourCrowdWorldModule.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezCVarBool cvar_DetourCrowdVisAgents("Recast.Crowd.VisAgents", false, ezCVarFlags::Default, "Draws DetourCrowd agents, if any");
ezCVarBool cvar_DetourCrowdVisCorners("Recast.Crowd.VisCorners", false, ezCVarFlags::Default, "Draws next few path corners of the DetourCrowd agents");
ezCVarInt cvar_DetourCrowdMaxAgents("Recast.Crowd.MaxAgents", 128, ezCVarFlags::Save, "Determines how many DetourCrowd agents can be created");
ezCVarFloat cvar_DetourCrowdMaxRadius("Recast.Crowd.MaxRadius", 2.0f, ezCVarFlags::Save, "Determines the maximum allowed radius of a DetourCrowd agent");

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezDetourCrowdWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDetourCrowdWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


ezDetourCrowdWorldModule::ezDetourCrowdWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

ezDetourCrowdWorldModule::~ezDetourCrowdWorldModule() = default;

void ezDetourCrowdWorldModule::Initialize()
{
  SUPER::Initialize();

  m_pRecastModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezDetourCrowdWorldModule::UpdateNavMesh, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezDetourCrowdWorldModule::UpdateCrowd, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezDetourCrowdWorldModule::VisualizeCrowd, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;

    RegisterUpdateFunction(desc);
  }
}

void ezDetourCrowdWorldModule::Deinitialize()
{
  if (m_pDtCrowd != nullptr)
  {
    dtFreeCrowd(m_pDtCrowd);
    m_pDtCrowd = nullptr;
  }

  SUPER::Deinitialize();
}

bool ezDetourCrowdWorldModule::IsInitializedAndReady() const
{
  if (m_pDtCrowd == nullptr)
    return false;

  if (m_pRecastModule == nullptr)
    return false;

  const dtNavMesh* pNavMesh = m_pRecastModule->GetDetourNavMesh();

  return m_pDtCrowd->getNavMeshQuery()->getAttachedNavMesh() == pNavMesh;
}

const dtCrowdAgent* ezDetourCrowdWorldModule::GetAgentById(ezInt32 iAgentId) const
{
  if (!IsInitializedAndReady())
    return nullptr;

  return m_pDtCrowd->getAgent(iAgentId);
}

void ezDetourCrowdWorldModule::FillDtCrowdAgentParams(const ezDetourCrowdAgentParams& params, struct dtCrowdAgentParams& out_params) const
{
  out_params.radius = ezMath::Clamp(params.m_fRadius, 0.0f, m_fMaxAgentRadius);
  out_params.height = params.m_fHeight;
  out_params.maxAcceleration = params.m_fMaxAcceleration;
  out_params.maxSpeed = params.m_fMaxSpeed;
  out_params.collisionQueryRange = ezMath::Max(1.2f, 12.0f * params.m_fRadius);
  out_params.pathOptimizationRange = ezMath::Max(3.0f, 30.0f * params.m_fRadius);
  out_params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
  out_params.obstacleAvoidanceType = 3;
  out_params.separationWeight = params.m_fSeparationWeight;
  out_params.userData = params.m_pUserData;
}

ezInt32 ezDetourCrowdWorldModule::CreateAgent(const ezVec3& vPos, const ezDetourCrowdAgentParams& params)
{
  if (!IsInitializedAndReady())
    return -1;

  dtCrowdAgentParams dtParams{};
  FillDtCrowdAgentParams(params, dtParams);

  ezInt32 iAgentId = m_pDtCrowd->addAgent(ezRcPos(vPos), &dtParams);

  return iAgentId;
}

void ezDetourCrowdWorldModule::DestroyAgent(ezInt32 iAgentId)
{
  if (!IsInitializedAndReady())
    return;

  m_pDtCrowd->removeAgent(iAgentId);
}

bool ezDetourCrowdWorldModule::SetAgentTargetPosition(ezInt32 iAgentId, const ezVec3& vPos, const ezVec3& vQueryHalfExtents)
{
  if (!IsInitializedAndReady())
    return false;

  float vNavPos[3];
  dtPolyRef navPolyRef;
  m_pDtCrowd->getNavMeshQuery()->findNearestPoly(ezRcPos(vPos), ezRcPos(vQueryHalfExtents), m_pDtCrowd->getFilter(0), &navPolyRef, vNavPos);
  if (navPolyRef != 0)
  {
    m_pDtCrowd->requestMoveTarget(iAgentId, navPolyRef, vNavPos);
    return true;
  }
  return false;
}

void ezDetourCrowdWorldModule::ClearAgentTargetPosition(ezInt32 iAgentId)
{
  if (!IsInitializedAndReady())
    return;

  m_pDtCrowd->resetMoveTarget(iAgentId);
}

void ezDetourCrowdWorldModule::UpdateAgentParams(ezInt32 iAgentId, const ezDetourCrowdAgentParams& params)
{
  if (!IsInitializedAndReady())
    return;

  dtCrowdAgentParams dtParams{};
  FillDtCrowdAgentParams(params, dtParams);

  m_pDtCrowd->updateAgentParameters(iAgentId, &dtParams);
}

void ezDetourCrowdWorldModule::UpdateNavMesh(const ezWorldModule::UpdateContext& ctx)
{
  const dtNavMesh* pNavMesh = m_pRecastModule->GetDetourNavMesh();

  ezInt32 iDesiredMaxAgents = ezMath::Clamp(cvar_DetourCrowdMaxAgents.GetValue(), 8, 2048);
  float fDesiredMaxRadius = ezMath::Clamp(cvar_DetourCrowdMaxRadius.GetValue(), 0.01f, 5.0f);

  if (pNavMesh != nullptr && (m_pDtCrowd == nullptr || m_pDtCrowd->getNavMeshQuery()->getAttachedNavMesh() != pNavMesh || m_iMaxAgents != iDesiredMaxAgents || m_fMaxAgentRadius != fDesiredMaxRadius))
  {
    if (m_pDtCrowd == nullptr)
      m_pDtCrowd = dtAllocCrowd();
    m_iMaxAgents = iDesiredMaxAgents;
    m_fMaxAgentRadius = fDesiredMaxRadius;
    m_pDtCrowd->init(m_iMaxAgents, m_fMaxAgentRadius, const_cast<dtNavMesh*>(pNavMesh));
  }
}

void ezDetourCrowdWorldModule::UpdateCrowd(const ezWorldModule::UpdateContext& ctx)
{
  if (!IsInitializedAndReady())
    return;

  const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
  m_pDtCrowd->update(fDeltaTime, nullptr);
}

void ezDetourCrowdWorldModule::VisualizeCrowd(const UpdateContext& ctx)
{
  if (!IsInitializedAndReady() || !cvar_DetourCrowdVisAgents)
    return;

  const ezInt32 iNumAgents = m_pDtCrowd->getAgentCount();
  for (int i = 0; i < iNumAgents; ++i)
  {
    const dtCrowdAgent* pAgent = m_pDtCrowd->getAgent(i);
    if (pAgent->active)
    {
      const float fHeight = pAgent->params.height;
      const float fRadius = pAgent->params.radius;

      ezTransform xform(ezRcPos(pAgent->npos));
      xform.m_vPosition.z += fHeight * 0.5f;

      // Draw agent cylinder
      ezDebugRenderer::DrawLineCylinderZ(GetWorld(), fHeight, fRadius, ezColor::BlueViolet, xform);

      // Draw velocity arrow
      ezVec3 vVelocity = ezRcPos(pAgent->vel);
      vVelocity.z = 0;
      if (!vVelocity.IsZero())
      {
        vVelocity.Normalize();
        xform.m_qRotation = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vVelocity);
        ezDebugRenderer::DrawArrow(GetWorld(), 1.0f, ezColor::BlueViolet, xform);
      }

      // Draw corners
      if (cvar_DetourCrowdVisCorners.GetValue() && pAgent->ncorners > 0)
      {
        ezDebugRenderer::Line lines[DT_CROWDAGENT_MAX_CORNERS];

        lines[0].m_start = ezRcPos(pAgent->npos);
        lines[0].m_end = ezRcPos(pAgent->cornerVerts);

        for (int i = 1; i < pAgent->ncorners; ++i)
        {
          lines[i].m_start = ezRcPos(pAgent->cornerVerts + 3 * (i - 1));
          lines[i].m_end = ezRcPos(pAgent->cornerVerts + 3 * i);
        }

        ezDebugRenderer::DrawLines(GetWorld(), ezArrayPtr(lines, pAgent->ncorners), ezColor::Cyan, ezTransform::Make(ezVec3(0.0f, 0.0f, 0.1f)));
      }
    }
  }
}
