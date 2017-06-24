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

struct ezRcPos
{
  float m_Pos[3];

  ezRcPos() {}
  ezRcPos(const ezVec3& v)
  {
    *this = v;
  }

  void operator=(const ezVec3& v)
  {
    m_Pos[0] = v.x;
    m_Pos[1] = v.z;
    m_Pos[2] = v.y;
  }

  void operator=(const float* pos)
  {
    m_Pos[0] = pos[0];
    m_Pos[1] = pos[1];
    m_Pos[2] = pos[2];
  }

  operator const float*() const
  {
    return &m_Pos[0];
  }

  operator ezVec3() const
  {
    return ezVec3(m_Pos[0], m_Pos[2], m_Pos[1]);
  }
};

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
  m_tLastUpdate = ezTime::Seconds(-100);

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

  RenderPathCorridorPosition();
  RenderPathCorridor();

  //if (GetWorld()->GetClock().GetAccumulatedTime() - m_tLastUpdate > ezTime::Seconds(7.0f))
  if (!m_bHasPath)
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


  if (HasReachedGoal(1.0f))
  {
    m_bHasPath = false;
    return;
  }

  VisualizeCurrentPath();
  VisualizeTargetPosition();

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

      if (m_bHasValidCorridor)
      {
        ezRcPos rcPosNew = vNewPos;

        dtQueryFilter filter; /// \todo
        if (!m_pCorridor->movePosition(rcPosNew, m_pQuery.Borrow(), &filter))
        {
          int i = 0;
          ++i;
        }

        {
          rcPosNew = m_pCorridor->getPos();
          vNewPos = rcPosNew;
        }
      }

      GetOwner()->SetGlobalPosition(vNewPos);
    }
  }
}

void ezRcAgentComponent::VisualizeCurrentPath()
{
  ezHybridArray<ezDebugRenderer::Line, 16> lines;
  lines.Reserve(m_iNumNextSteps);

  ezHybridArray<ezDebugRenderer::Line, 16> steps;
  steps.Reserve(m_iNumNextSteps);

  ezVec3 vPrev = GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 0.5f);
  for (ezInt32 i = m_iFirstNextStep; i < m_iNumNextSteps; ++i)
  {
    auto& line = lines.ExpandAndGetRef();
    line.m_start = vPrev;
    line.m_end = m_vNextSteps[i] + ezVec3(0, 0, 0.5f);
    vPrev = line.m_end;

    auto& step = steps.ExpandAndGetRef();
    step.m_start = m_vNextSteps[i];
    step.m_end = m_vNextSteps[i] + ezVec3(0, 0, 1.0f);
  }

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::DarkViolet);
  ezDebugRenderer::DrawLines(GetWorld(), steps, ezColor::LightYellow);
}


void ezRcAgentComponent::VisualizeTargetPosition()
{
  ezHybridArray<ezDebugRenderer::Line, 16> lines;
  auto& line = lines.ExpandAndGetRef();

  line.m_start = m_vEndPosition - ezVec3(0, 0, 0.5f);
  line.m_end = m_vEndPosition + ezVec3(0, 0, 1.5f);

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::HotPink);
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

  m_bHasPath = true;
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

  if (m_PathCorridor[m_iPathCorridorLength - 1] != m_endPoly)
  {
    ezLog::Warning("Target Position cannot be reached");
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
  m_iNumNextSteps = m_pCorridor->findCorners(&m_vNextSteps[0].x, m_uiStepFlags, m_StepPolys, 4, m_pQuery.Borrow(), &filter);

  for (ezInt32 i = 0; i < m_iNumNextSteps; ++i)
  {
    ezMath::Swap(m_vNextSteps[i].y, m_vNextSteps[i].z);
  }

  /// \todo Failure ?
  return EZ_SUCCESS;
}

ezVec3 ezRcAgentComponent::ComputeSteeringDirection(float fMaxDistance)
{
  if (!m_bHasValidCorridor)
    return ezVec3::ZeroVector();

  ezVec3 vCurPos = GetOwner()->GetGlobalPosition();
  vCurPos.z = 0;

  ezInt32 iNextStep = 0;
  for (iNextStep = m_iNumNextSteps - 1; iNextStep >= m_iFirstNextStep; --iNextStep)
  {
    ezVec3 step = m_vNextSteps[iNextStep];

    if (IsPositionVisible(step))
      break;
  }

  if (iNextStep < m_iFirstNextStep)
  {
    ezLog::Error("Next step not visible");
    iNextStep = m_iFirstNextStep;
  }

  if (iNextStep == m_iNumNextSteps - 1)
  {
    if (HasReachedPosition(m_vNextSteps[iNextStep], 1.0f))
    {
      PlanNextSteps();
      return ezVec3::ZeroVector();
    }
  }

  m_iFirstNextStep = iNextStep;

  if (m_iFirstNextStep >= m_iNumNextSteps)
  {
    PlanNextSteps();
    return ezVec3::ZeroVector();
  }


  ezVec3 vDirection = m_vNextSteps[m_iFirstNextStep] - vCurPos;
  vDirection.z = 0;
  vDirection.NormalizeIfNotZero(ezVec3::ZeroVector());

  return vDirection;
}


bool ezRcAgentComponent::HasReachedPosition(const ezVec3& pos, float fMaxDistance) const
{
  ezVec3 vTargetPos = pos;
  ezVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  const float fCellHeight = 1.0f;

  // agent component is assumed to be located on the ground (independent of character height)
  // so max error is dependent on the navmesh resolution mostly (cell height)
  const float fHeightError = fCellHeight;
  
  if (!ezMath::IsInRange(vTargetPos.z, vOwnPos.z - fHeightError, vOwnPos.z + fHeightError))
    return false;

  vTargetPos.z = 0;
  vOwnPos.z = 0;

  return (vTargetPos - vOwnPos).GetLength() < fMaxDistance;
}

bool ezRcAgentComponent::HasReachedGoal(float fMaxDistance) const
{
  if (!m_bHasPath)
    return true;

  return HasReachedPosition(m_vEndPosition, fMaxDistance);
}

bool ezRcAgentComponent::IsPositionVisible(const ezVec3& pos) const
{
  ezRcPos endPos = pos;

  /// \todo Hardcoded filter
  dtQueryFilter filter;
  dtRaycastHit hit;
  if (dtStatusFailed(m_pQuery->raycast(m_pCorridor->getFirstPoly(), m_pCorridor->getPos(), endPos, &filter, 0, &hit)))
    return false;

  // 'visible' if no hit was detected
  return (hit.t > 100000.0f);
}

void ezRcAgentComponent::RenderPathCorridorPosition()
{
  if (!m_bHasPath || !m_bHasValidCorridor)
    return;

  const float* pos = m_pCorridor->getPos();
  const ezVec3 vPos(pos[0], pos[2], pos[1]);

  ezBoundingBox box;
  box.SetCenterAndHalfExtents(ezVec3(0, 0, 1.0f), ezVec3(0.3f, 0.3f, 1.0f));

  ezTransform t;
  t.SetIdentity();
  t.m_vPosition = vPos;
  t.m_qRotation = GetOwner()->GetGlobalRotation();

  ezDebugRenderer::DrawLineBox(GetWorld(), box, ezColor::DarkGreen, t);
}

void ezRcAgentComponent::RenderPathCorridor()
{
  if (!m_bHasPath || !m_bHasValidCorridor)
    return;

  for (ezInt32 c = 0; c < m_iPathCorridorLength; ++c)
  {
    dtPolyRef poly = m_PathCorridor[c];

    const dtMeshTile* pTile;
    const dtPoly* pPoly;
    m_pQuery->getAttachedNavMesh()->getTileAndPolyByRef(poly, &pTile, &pPoly);

    ezHybridArray<ezDebugRenderer::Triangle, 32> tris;

    for (ezUInt32 i = 2; i < pPoly->vertCount; ++i)
    {
      ezRcPos rcPos[3];
      rcPos[0] = &(pTile->verts[pPoly->verts[0] * 3]);
      rcPos[1] = &(pTile->verts[pPoly->verts[i - 1] * 3]);
      rcPos[2] = &(pTile->verts[pPoly->verts[i] * 3]);

      auto& tri = tris.ExpandAndGetRef();
      tri.m_p0 = ezVec3(rcPos[0]) + ezVec3(0, 0, 0.1f);
      tri.m_p2 = ezVec3(rcPos[1]) + ezVec3(0, 0, 0.1f);
      tri.m_p1 = ezVec3(rcPos[2]) + ezVec3(0, 0, 0.1f);
    }

    ezDebugRenderer::DrawSolidTriangles(GetWorld(), tris, ezColor::OrangeRed.WithAlpha(0.4f));
  }
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
