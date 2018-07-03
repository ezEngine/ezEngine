#include <PCH.h>
#include <RecastPlugin/Components/RecastAgentComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <ThirdParty/Recast/DetourCrowd.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/Components/CharacterControllerComponent.h>
#include <RecastPlugin/Utils/RcMath.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezRcAgentComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WalkSpeed",m_fWalkSpeed)->AddAttributes(new ezDefaultValueAttribute(4.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE

ezRcAgentComponent::ezRcAgentComponent() { }
ezRcAgentComponent::~ezRcAgentComponent() { }

void ezRcAgentComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_fWalkSpeed;
}

void ezRcAgentComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fWalkSpeed;
}

ezResult ezRcAgentComponent::InitializeRecast()
{
  if (m_bRecastInitialized)
    return EZ_SUCCESS;

  if (!GetWorld()->GetOrCreateModule<ezRecastWorldModule>()->IsInitialized())
    return EZ_FAILURE;

  dtNavMesh* pNavMesh = GetWorld()->GetOrCreateModule<ezRecastWorldModule>()->GetNavMesh();
  if (pNavMesh == nullptr)
    return EZ_FAILURE;

  m_bRecastInitialized = true;

  m_pQuery = EZ_DEFAULT_NEW(dtNavMeshQuery);
  m_pCorridor = EZ_DEFAULT_NEW(dtPathCorridor);

  /// \todo Hard-coded limits
  m_pQuery->init(pNavMesh, 512);
  m_pCorridor->init(256);

  return EZ_SUCCESS;
}

void ezRcAgentComponent::ClearTargetPosition()
{
  m_iNumNextSteps = 0;
  m_iFirstNextStep = 0;
  m_PathCorridor.Clear();
  m_vCurrentSteeringDirection.SetZero();

  if (m_PathToTargetState != ezAgentPathFindingState::HasNoTarget)
  {
    m_PathToTargetState = ezAgentPathFindingState::HasNoTarget;

    ezAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = ezAgentSteeringEvent::TargetCleared;

    m_SteeringEvents.Broadcast(e, 1);
  }
}

ezAgentPathFindingState::Enum ezRcAgentComponent::GetPathToTargetState() const
{
  return m_PathToTargetState;
}

void ezRcAgentComponent::SetTargetPosition(const ezVec3& vPos)
{
  ClearTargetPosition();

  m_vTargetPosition = vPos;
  m_PathToTargetState = ezAgentPathFindingState::HasTargetWaitingForPath;
}

ezVec3 ezRcAgentComponent::GetTargetPosition() const
{
  return m_vTargetPosition;
}

ezResult ezRcAgentComponent::FindNavMeshPolyAt(const ezVec3& vPosition, dtPolyRef& out_PolyRef, ezVec3* out_vAdjustedPosition /*= nullptr*/, float fPlaneEpsilon /*= 0.01f*/, float fHeightEpsilon /*= 1.0f*/) const
{
  ezRcPos rcPos = vPosition;
  ezVec3 vSize(fPlaneEpsilon, fHeightEpsilon, fPlaneEpsilon);

  ezRcPos resultPos;
  dtQueryFilter filter; /// \todo Hard-coded filter
  if (dtStatusFailed(m_pQuery->findNearestPoly(rcPos, &vSize.x, &m_QueryFilter, &out_PolyRef, resultPos)))
    return EZ_FAILURE;

  if (!ezMath::IsEqual(vPosition.x, resultPos.m_Pos[0], fPlaneEpsilon) ||
      !ezMath::IsEqual(vPosition.y, resultPos.m_Pos[2], fPlaneEpsilon) ||
      !ezMath::IsEqual(vPosition.z, resultPos.m_Pos[1], fHeightEpsilon))
    return EZ_FAILURE;

  if (out_vAdjustedPosition != nullptr)
  {
    *out_vAdjustedPosition = resultPos;
  }

  return EZ_SUCCESS;
}

ezResult ezRcAgentComponent::ComputePathCorridor(dtPolyRef startPoly, dtPolyRef endPoly, bool& bFoundPartialPath)
{
  bFoundPartialPath = false;

  ezRcPos rcStart = m_vCurrentPositionOnNavmesh;
  ezRcPos rcEnd = m_vTargetPosition;

  ezInt32 iPathCorridorLength = 0;

  // make enough room
  m_PathCorridor.SetCountUninitialized(256);
  if (dtStatusFailed(m_pQuery->findPath(startPoly, endPoly, rcStart, rcEnd, &m_QueryFilter, m_PathCorridor.GetData(), &iPathCorridorLength, (int)m_PathCorridor.GetCount())) || iPathCorridorLength <= 0)
  {
    m_PathCorridor.Clear();
    return EZ_FAILURE;
  }

  // reduce to actual length
  m_PathCorridor.SetCountUninitialized(iPathCorridorLength);

  if (m_PathCorridor[iPathCorridorLength - 1] != endPoly)
  {
    // if this is the case, the target position cannot be reached, but we can walk close to it
    bFoundPartialPath = true;
  }

  m_pCorridor->reset(startPoly, rcStart);
  m_pCorridor->setCorridor(rcEnd, m_PathCorridor.GetData(), iPathCorridorLength);

  return EZ_SUCCESS;
}

ezResult ezRcAgentComponent::ComputePathToTarget()
{
  const ezVec3 vStartPos = GetOwner()->GetGlobalPosition();

  dtPolyRef startPoly;
  if (FindNavMeshPolyAt(vStartPos, startPoly, &m_vCurrentPositionOnNavmesh).Failed())
  {
    m_PathToTargetState = ezAgentPathFindingState::HasTargetPathFindingFailed;

    ezAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = ezAgentSteeringEvent::ErrorOutsideNavArea;
    m_SteeringEvents.Broadcast(e);
    return EZ_FAILURE;
  }

  dtPolyRef endPoly;
  if (FindNavMeshPolyAt(m_vTargetPosition, endPoly).Failed())
  {
    m_PathToTargetState = ezAgentPathFindingState::HasTargetPathFindingFailed;

    ezAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = ezAgentSteeringEvent::ErrorInvalidTargetPosition;
    m_SteeringEvents.Broadcast(e);
    return EZ_FAILURE;
  }

  /// \todo Optimize case when endPoly is same as previously ?

  bool bFoundPartialPath = false;
  if (ComputePathCorridor(startPoly, endPoly, bFoundPartialPath).Failed() || bFoundPartialPath)
  {
    m_PathToTargetState = ezAgentPathFindingState::HasTargetPathFindingFailed;

    /// \todo For now a partial path is considered an error

    ezAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = bFoundPartialPath ? ezAgentSteeringEvent::WarningNoFullPathToTarget : ezAgentSteeringEvent::ErrorNoPathToTarget;
    m_SteeringEvents.Broadcast(e);
    return EZ_FAILURE;
  }

  m_PathToTargetState = ezAgentPathFindingState::HasTargetAndValidPath;

  ezAgentSteeringEvent e;
  e.m_pComponent = this;
  e.m_Type = ezAgentSteeringEvent::PathToTargetFound;
  m_SteeringEvents.Broadcast(e);
  return EZ_SUCCESS;
}

bool ezRcAgentComponent::HasReachedPosition(const ezVec3& pos, float fMaxDistance) const
{
  ezVec3 vTargetPos = pos;
  ezVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  /// \todo The comment below may not always be true
  const float fCellHeight = 1.5f;

  // agent component is assumed to be located on the ground (independent of character height)
  // so max error is dependent on the navmesh resolution mostly (cell height)
  const float fHeightError = fCellHeight;

  if (!ezMath::IsInRange(vTargetPos.z, vOwnPos.z - fHeightError, vOwnPos.z + fHeightError))
    return false;

  vTargetPos.z = 0;
  vOwnPos.z = 0;

  return (vTargetPos - vOwnPos).GetLengthSquared() < ezMath::Square(fMaxDistance);
}

bool ezRcAgentComponent::HasReachedGoal(float fMaxDistance) const
{
  if (GetPathToTargetState() == ezAgentPathFindingState::HasNoTarget)
    return true;

  return HasReachedPosition(m_vTargetPosition, fMaxDistance);
}

void ezRcAgentComponent::PlanNextSteps()
{
  if (m_PathCorridor.IsEmpty())
    return;

  ezUInt8 stepFlags[16];
  dtPolyRef stepPolys[16];

  m_iFirstNextStep = 0;
  m_iNumNextSteps = m_pCorridor->findCorners(&m_vNextSteps[0].x, stepFlags, stepPolys, 4, m_pQuery.Borrow(), &m_QueryFilter);

  // convert from Recast convention (Y up) to ez (Z up)
  for (ezInt32 i = 0; i < m_iNumNextSteps; ++i)
  {
    ezMath::Swap(m_vNextSteps[i].y, m_vNextSteps[i].z);
  }
}

bool ezRcAgentComponent::IsPositionVisible(const ezVec3& pos) const
{
  ezRcPos endPos = pos;

  dtRaycastHit hit;
  if (dtStatusFailed(m_pQuery->raycast(m_pCorridor->getFirstPoly(), m_pCorridor->getPos(), endPos, &m_QueryFilter, 0, &hit)))
    return false;

  // 'visible' if no hit was detected
  return (hit.t > 100000.0f);
}

void ezRcAgentComponent::OnSimulationStarted()
{
  ClearTargetPosition();

  m_bRecastInitialized = false;

  ezCharacterControllerComponent* pCC = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<ezCharacterControllerComponent>(pCC))
  {
    m_hCharacterController = pCC->GetHandle();
  }
}

void ezRcAgentComponent::ApplySteering(const ezVec3& vDirection, float fSpeed)
{
  // compute new rotation
  {
    ezQuat qDesiredNewRotation;
    qDesiredNewRotation.SetShortestRotation(ezVec3(1, 0, 0), vDirection);

    /// \todo Pass through character controller
    GetOwner()->SetGlobalRotation(qDesiredNewRotation);
  }

  if (!m_hCharacterController.IsInvalidated())
  {
    ezCharacterControllerComponent* pCharacter = nullptr;
    if (GetWorld()->TryGetComponent(m_hCharacterController, pCharacter))
    {
      // the character controller already applies time scaling
      const ezVec3 vRelativeSpeed = (-GetOwner()->GetGlobalRotation() * vDirection) * fSpeed;

      ezMsgMoveCharacterController msg;
      msg.m_fMoveForwards = ezMath::Max(0.0f, vRelativeSpeed.x);
      msg.m_fMoveBackwards = ezMath::Max(0.0f, -vRelativeSpeed.x);
      msg.m_fStrafeLeft = ezMath::Max(0.0f, -vRelativeSpeed.y);
      msg.m_fStrafeRight = ezMath::Max(0.0f, vRelativeSpeed.y);

      pCharacter->MoveCharacter(msg);
    }
  }
  else
  {
    const float fTimeDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();
    const ezVec3 vOwnerPos = GetOwner()->GetGlobalPosition();
    const ezVec3 vDesiredNewPosition = vOwnerPos + vDirection * fSpeed * fTimeDiff;

    GetOwner()->SetGlobalPosition(vDesiredNewPosition);
  }
}

void ezRcAgentComponent::SyncSteeringWithReality()
{
  const ezRcPos rcCurrentAgentPosition = GetOwner()->GetGlobalPosition();

  if (!m_pCorridor->movePosition(rcCurrentAgentPosition, m_pQuery.Borrow(), &m_QueryFilter))
  {
    ezAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = ezAgentSteeringEvent::ErrorSteeringFailed;
    m_SteeringEvents.Broadcast(e);
    ClearTargetPosition();
    return;
  }

  const ezRcPos rcPosOnNavmesh = m_pCorridor->getPos();
  m_vCurrentPositionOnNavmesh = rcPosOnNavmesh;

  /// \todo Check when these values diverge
}

void ezRcAgentComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  // this can happen the first few frames
  if (InitializeRecast().Failed())
    return;

  // visualize various things
  {
    VisualizePathCorridorPosition();
    VisualizePathCorridor();
    VisualizeCurrentPath();
    VisualizeTargetPosition();
  }

  // target is set, but no path is computed yet
  if (GetPathToTargetState() == ezAgentPathFindingState::HasTargetWaitingForPath)
  {
    if (ComputePathToTarget().Failed())
      return;

    PlanNextSteps();
  }

  // from here on down, everything has to do with following a valid path

  if (GetPathToTargetState() != ezAgentPathFindingState::HasTargetAndValidPath)
    return;

  if (HasReachedGoal(1.0f))
  {
    ezAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = ezAgentSteeringEvent::TargetReached;
    m_SteeringEvents.Broadcast(e);

    ClearTargetPosition();
    return;
  }

  ComputeSteeringDirection(1.0f);

  if (m_vCurrentSteeringDirection.IsZero())
  {
    /// \todo This would be some sort of error
    ezLog::Error("Steering Direction is zero.");
    ClearTargetPosition();
    return;
  }

  ApplySteering(m_vCurrentSteeringDirection, m_fWalkSpeed);

  SyncSteeringWithReality();
}

void ezRcAgentComponent::ComputeSteeringDirection(float fMaxDistance)
{
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
    //ezLog::Error("Next step not visible");
    iNextStep = m_iFirstNextStep;
  }

  if (iNextStep == m_iNumNextSteps - 1)
  {
    if (HasReachedPosition(m_vNextSteps[iNextStep], 1.0f))
    {
      PlanNextSteps();
      return; // reuse last steering direction
    }
  }

  m_iFirstNextStep = iNextStep;

  if (m_iFirstNextStep >= m_iNumNextSteps)
  {
    PlanNextSteps();
    return; // reuse last steering direction
  }


  ezVec3 vDirection = m_vNextSteps[m_iFirstNextStep] - vCurPos;
  vDirection.z = 0;
  vDirection.NormalizeIfNotZero(ezVec3::ZeroVector());

  m_vCurrentSteeringDirection = vDirection;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezRcAgentComponent::VisualizePathCorridorPosition()
{
  if (GetPathToTargetState() != ezAgentPathFindingState::HasTargetAndValidPath)
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

void ezRcAgentComponent::VisualizePathCorridor()
{
  if (GetPathToTargetState() != ezAgentPathFindingState::HasTargetAndValidPath)
    return;

  for (ezUInt32 c = 0; c < m_PathCorridor.GetCount(); ++c)
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

void ezRcAgentComponent::VisualizeTargetPosition()
{
  if (GetPathToTargetState() == ezAgentPathFindingState::HasNoTarget)
    return;

  ezHybridArray<ezDebugRenderer::Line, 16> lines;
  auto& line = lines.ExpandAndGetRef();

  line.m_start = m_vTargetPosition - ezVec3(0, 0, 0.5f);
  line.m_end = m_vTargetPosition + ezVec3(0, 0, 1.5f);

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::HotPink);
}

void ezRcAgentComponent::VisualizeCurrentPath()
{
  if (GetPathToTargetState() != ezAgentPathFindingState::HasTargetAndValidPath)
    return;

  ezHybridArray<ezDebugRenderer::Line, 16> lines;
  lines.Reserve(m_iNumNextSteps);

  ezHybridArray<ezDebugRenderer::Line, 16> steps;
  steps.Reserve(m_iNumNextSteps);

  /// \todo Hard-coded height offset
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


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezRcAgentComponentManager::ezRcAgentComponentManager(ezWorld* pWorld) : SUPER(pWorld) { }
ezRcAgentComponentManager::~ezRcAgentComponentManager() { }

void ezRcAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();

  m_pPhysicsInterface = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRcAgentComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezRcAgentComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
      it->Update();
  }
}
