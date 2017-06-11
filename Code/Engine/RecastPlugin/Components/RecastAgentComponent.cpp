#include <PCH.h>
#include <RecastPlugin/Components/RecastAgentComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <ThirdParty/Recast/DetourCrowd.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezRcAgentComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WalkSpeed",m_fWalkSpeed)->AddAttributes(new ezDefaultValueAttribute(4.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES
}
EZ_END_COMPONENT_TYPE

ezRcAgentComponent::ezRcAgentComponent() { }
ezRcAgentComponent::~ezRcAgentComponent() { }

void ezRcAgentComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_fWalkSpeed;
  s << m_uiCollisionLayer;
}

void ezRcAgentComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fWalkSpeed;
  s >> m_uiCollisionLayer;
}


void ezRcAgentComponent::OnSimulationStarted()
{
  m_tLastUpdate = GetWorld()->GetClock().GetAccumulatedTime();

  m_bInitialized = false;
}

static float frand()
{
  return (float)rand() / (float)RAND_MAX;
}

void ezRcAgentComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (!Init())
    return;


  if (GetWorld()->GetClock().GetAccumulatedTime() - m_tLastUpdate > ezTime::Seconds(3.0f))
  {
    m_tLastUpdate = GetWorld()->GetClock().GetAccumulatedTime();

    dtCrowd* pCrowd = GetManager()->GetRecastWorldModule()->m_pCrowd;

    dtQueryFilter filter;
    dtPolyRef ref;
    float pt[3];
    if (dtStatusFailed(pCrowd->getNavMeshQuery()->findRandomPoint(&filter, frand, &ref, pt)))
    {
      ezLog::Error("Could not find random point");
    }
    else
    {
      SetTargetPosition(ezVec3(pt[0], pt[2], pt[1]));
    }
  }

  ezHybridArray<ezDebugRenderer::Line, 16> lines;
  lines.Reserve(m_iNumNextSteps);

  ezVec3 vPrev = GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 0.5f);
  for (ezInt32 i = m_iFirstNextStep; i < m_iNumNextSteps; ++i)
  {
    auto& line = lines.ExpandAndGetRef();
    line.m_start = vPrev;
    line.m_end = m_vNextSteps[i] + ezVec3(0, 0, 0.5f);
    vPrev = line.m_end;
  }

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::IndianRed);

  {
    const ezVec3 vDir = ComputeSteeringDirection(1.0f);

    const ezVec3 vOwnerPos = GetOwner()->GetGlobalPosition();
    const ezVec3 vTargetPos = vOwnerPos + vDir;

    ezDebugRenderer::Line dir;
    dir.m_start = vOwnerPos + ezVec3(0, 0, 0.5f);
    dir.m_end = vTargetPos + ezVec3(0, 0, 0.5f);
    ezDebugRenderer::DrawLines(GetWorld(), ezArrayPtr<ezDebugRenderer::Line>(&dir, 1), ezColor::OldLace);

    if (!vDir.IsZero())
    {
      const ezVec3 vSpeed = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * m_fWalkSpeed * vDir;
      ezVec3 vNewPos = vOwnerPos + vSpeed;

      ezQuat qRot;
      qRot.SetShortestRotation(ezVec3(1, 0, 0), vDir);

      GetOwner()->SetGlobalRotation(qRot);

      ezPhysicsWorldModuleInterface* pPhysicsInterface = GetManager()->m_pPhysicsInterface;
      if (pPhysicsInterface)
      {
        /// \todo radius, step height, gravity dir

        float fStepHeight = 0.2f;
        ezVec3 vStepHeight = vNewPos + ezVec3(0, 0, fStepHeight);

        ezPhysicsHitResult res;
        if (pPhysicsInterface->SweepTestSphere(0.05f, vStepHeight, ezVec3(0, 0, -1), fStepHeight * 2, m_uiCollisionLayer, res))
        {
          vNewPos = vStepHeight;
          vNewPos.z -= res.m_fDistance;
        }
        else
        {
          // falling

          vNewPos = vStepHeight;
          vNewPos.z -= fStepHeight * 2;
        }
      }

      GetOwner()->SetGlobalPosition(vNewPos);


      ezMath::Swap(vNewPos.y, vNewPos.z);

      if (m_bHasValidCorridor)
      {
        dtQueryFilter filter; /// \todo
        m_pCorridor->movePosition(&vNewPos.x, m_pQuery.Borrow(), &filter);
      }
    }
  }


}

void ezRcAgentComponent::SetTargetPosition(const ezVec3& vPos)
{
  if (!m_bInitialized)
    return;

  ezVec3 vStartPos = GetOwner()->GetGlobalPosition();

  dtPolyRef startPoly;
  if (FindNavMeshPolyAt(vStartPos, startPoly).Failed())
  {
    ezLog::Warning("ezRcAgentComponent::SetTargetPosition: Agent not on NavMesh");
    return;
  }

  ezVec3 vEndPos = vPos;
  dtPolyRef endPoly;
  if (FindNavMeshPolyAt(vEndPos, endPoly).Failed())
  {
    ezLog::Warning("ezRcAgentComponent::SetTargetPosition: Destination not on NavMesh");
    return;
  }

  if (m_startPoly == startPoly && m_endPoly == endPoly)
  {
    //ezLog::Warning("ezRcAgentComponent::SetTargetPosition: Start/End have not changed");
    return;
  }

  m_vStartPosition = GetOwner()->GetGlobalPosition();
  m_vEndPosition = vPos;

  m_startPoly = startPoly;
  m_endPoly = endPoly;

  if (RecomputePathCorridor().Failed())
  {
    ezLog::Warning("ezRcAgentComponent::SetTargetPosition: Failed to compute path corridor");
    return;
  }


}


ezResult ezRcAgentComponent::FindNavMeshPolyAt(ezVec3& inout_vPosition, dtPolyRef& out_PolyRef) const
{
  ezVec3 vPos = inout_vPosition;
  ezMath::Swap(vPos.y, vPos.z);
  ezVec3 vSize(0.5f, 1.0f, 0.5f); /// \todo Hard-coded offset

  ezVec3 resultPos;
  dtQueryFilter filter; /// \todo Hard-coded filter
  if (dtStatusFailed(m_pQuery->findNearestPoly(&vPos.x, &vSize.x, &filter, &out_PolyRef, &resultPos.x)))
    return EZ_FAILURE;

  ezMath::Swap(resultPos.y, resultPos.z);

  if (!ezMath::IsEqual(inout_vPosition.x, resultPos.x, 0.01f) ||
      !ezMath::IsEqual(inout_vPosition.y, resultPos.y, 0.01f))
    return EZ_FAILURE;

  inout_vPosition = resultPos;
  return EZ_SUCCESS;
}

bool ezRcAgentComponent::Init()
{
  if (m_bInitialized)
    return true;

  if (!GetManager()->GetRecastWorldModule()->IsInitialized())
    return false;

  dtNavMesh* pNavMesh = GetManager()->GetRecastWorldModule()->GetNavMesh();
  if (pNavMesh == nullptr)
    return false;

  m_bInitialized = true;

  m_pQuery = EZ_DEFAULT_NEW(dtNavMeshQuery);
  m_pCorridor = EZ_DEFAULT_NEW(dtPathCorridor);
  m_bHasValidCorridor = false;

  /// \todo Hard-coded limits
  m_pQuery->init(pNavMesh, 512);
  m_pCorridor->init(256);

  m_PathCorridor.SetCount(256);

  return true;
}


ezResult ezRcAgentComponent::RecomputePathCorridor()
{
  m_bHasValidCorridor = false;

  ezVec3 vStart = m_vStartPosition;
  ezVec3 vEnd = m_vEndPosition;

  ezMath::Swap(vStart.y, vStart.z);
  ezMath::Swap(vEnd.y, vEnd.z);

  dtQueryFilter filter; /// \todo Hard-coded filter

  m_iPathCorridorLength = 0;

  if (dtStatusFailed(m_pQuery->findPath(m_startPoly, m_endPoly, &vStart.x, &vEnd.x, &filter, m_PathCorridor.GetData(), &m_iPathCorridorLength, (int)m_PathCorridor.GetCount())))
  {
    return EZ_FAILURE;
  }

  m_pCorridor->reset(m_startPoly, &vStart.x);
  m_pCorridor->setCorridor(&vEnd.x, m_PathCorridor.GetData(), m_iPathCorridorLength);
  m_bHasValidCorridor = m_iPathCorridorLength > 0;

  return PlanNextSteps();
}


ezResult ezRcAgentComponent::PlanNextSteps()
{
  if (!m_bHasValidCorridor)
    return EZ_FAILURE;

  dtQueryFilter filter; /// \todo Hard-coded filter
  m_iFirstNextStep = 0;
  m_iNumNextSteps = m_pCorridor->findCorners(&m_vNextSteps[0].x, m_uiStepFlags, m_StepPolys, 16, m_pQuery.Borrow(), &filter);

  for (ezInt32 i = 0; i < m_iNumNextSteps; ++i)
  {
    ezMath::Swap(m_vNextSteps[i].y, m_vNextSteps[i].z);
  }

  /// \todo Failure ?
  return EZ_SUCCESS;
}

ezVec3 ezRcAgentComponent::ComputeSteeringDirection(float fMaxDistance)
{
  ezVec3 vCurPos = GetOwner()->GetGlobalPosition();
  vCurPos.z = 0;

  for (ezInt32 i = m_iFirstNextStep; i < m_iNumNextSteps; ++i)
  {
    ezVec3 step = m_vNextSteps[i];
    step.z = 0;

    if ((step - vCurPos).GetLength() < fMaxDistance)
    {
      ++m_iFirstNextStep;
    }
  }

  if (m_iFirstNextStep >= m_iNumNextSteps)
    return ezVec3::ZeroVector();


  ezVec3 vDirection = m_vNextSteps[m_iFirstNextStep] - vCurPos;
  vDirection.z = 0;
  vDirection.NormalizeIfNotZero(ezVec3::ZeroVector());

  return vDirection;
}

//////////////////////////////////////////////////////////////////////////

ezRcAgentComponentManager::ezRcAgentComponentManager(ezWorld* pWorld) : SUPER(pWorld) { }
ezRcAgentComponentManager::~ezRcAgentComponentManager() { }

void ezRcAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRcAgentComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezRcAgentComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  if (m_pPhysicsInterface == nullptr)
  {
    m_pPhysicsInterface = GetWorld()->GetModuleOfBaseType<ezPhysicsWorldModuleInterface>();
  }

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
      it->Update();
  }
}
