#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <DetourCrowd.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RecastPlugin/Utils/RcMath.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Foundation/Configuration/CVar.h>
#include <RecastPlugin/WorldModule/DetourCrowdWorldModule.h>

ezCVarBool cvar_RecastVisDetourCrowd("Recast.VisDetourCrowd", false, ezCVarFlags::Default, "Draws DetourCrowd agents, if any");

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
    //desc.m_DependsOn.PushBack(ezMakeHashedString("ezRecastWorldModule::UpdateNavMesh"));

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
  out_params.collisionQueryRange = 12.0f * params.m_fRadius;
  out_params.pathOptimizationRange = 30.0f * params.m_fRadius;
  out_params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO 
    | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
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

void ezDetourCrowdWorldModule::SetAgentTargetPosition(ezInt32 iAgentId, const ezVec3& vPos, const ezVec3& vQueryHalfExtents)
{
  if (!IsInitializedAndReady())
    return;

  float vNavPos[3];
  dtPolyRef navPolyRef;
  m_pDtCrowd->getNavMeshQuery()->findNearestPoly(ezRcPos(vPos), ezRcPos(vQueryHalfExtents), m_pDtCrowd->getFilter(0), &navPolyRef, vNavPos);
  m_pDtCrowd->requestMoveTarget(iAgentId, navPolyRef, vNavPos);
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

  if (pNavMesh != nullptr && (m_pDtCrowd == nullptr || m_pDtCrowd->getNavMeshQuery()->getAttachedNavMesh() != pNavMesh))
  {
    if (m_pDtCrowd == nullptr)
      m_pDtCrowd = dtAllocCrowd();
    m_pDtCrowd->init(m_iMaxAgents, m_fMaxAgentRadius, const_cast<dtNavMesh*>(pNavMesh));
    // \todo recreate agents when crowd is recreated?
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
  if (!IsInitializedAndReady() || !cvar_RecastVisDetourCrowd)
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

      ezDebugRenderer::DrawLineCylinderZ(GetWorld(), fHeight, fRadius, ezColor::BlueViolet, xform);

      ezVec3 vVelocity = ezRcPos(pAgent->vel);
      vVelocity.z = 0;
      if (!vVelocity.IsZero())
      {
        vVelocity.Normalize();
        xform.m_qRotation = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vVelocity);
        ezDebugRenderer::DrawArrow(GetWorld(), 1.0f, ezColor::BlueViolet, xform);
      }
    }
  }
}
