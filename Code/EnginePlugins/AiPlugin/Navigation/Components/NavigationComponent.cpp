#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezAiNavigationDebugFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezAiNavigationDebugFlags::PrintState, ezAiNavigationDebugFlags::VisPathCorridor, ezAiNavigationDebugFlags::VisPathLine, ezAiNavigationDebugFlags::VisTarget)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAiNavigationComponentState, 1)
  EZ_ENUM_CONSTANTS(ezAiNavigationComponentState::Idle, ezAiNavigationComponentState::Moving, ezAiNavigationComponentState::Falling, ezAiNavigationComponentState::Fallen, ezAiNavigationComponentState::Failed)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezAiNavigationComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new ezDynamicStringEnumAttribute("AiNavmeshConfig")),
    EZ_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new ezDynamicStringEnumAttribute("AiPathSearchConfig")),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    EZ_MEMBER_PROPERTY("Deceleration", m_fDecceleration)->AddAttributes(new ezDefaultValueAttribute(8.0f)),
    EZ_MEMBER_PROPERTY("FootRadius", m_fFootRadius)->AddAttributes(new ezDefaultValueAttribute(0.15f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("ReachedDistance", m_fReachedDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("FallHeight", m_fFallHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_BITFLAGS_MEMBER_PROPERTY("DebugFlags", ezAiNavigationDebugFlags , m_DebugFlags),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Navigation"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetDestination, In, "Destination", In, "AllowPartialPaths"),
    EZ_SCRIPT_FUNCTION_PROPERTY(CancelNavigation),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetState),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAiNavigationComponent::ezAiNavigationComponent() = default;
ezAiNavigationComponent::~ezAiNavigationComponent() = default;

void ezAiNavigationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_uiSkipNextFrames = 3; // 2 are needed to have colliders set up at the start of the scene simulation, 3 just to be save
  m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
}

void ezAiNavigationComponent::SetDestination(const ezVec3& vGlobalPos, bool bAllowPartialPath)
{
  m_bAllowPartialPath = bAllowPartialPath;
  m_Navigation.SetTargetPosition(vGlobalPos);
  m_State = ezAiNavigationComponentState::Moving;
}

void ezAiNavigationComponent::CancelNavigation()
{
  m_Navigation.CancelNavigation();

  if (m_State != ezAiNavigationComponentState::Falling)
  {
    // if it is still falling, don't reset the state
    m_State = ezAiNavigationComponentState::Idle;
  }
}

void ezAiNavigationComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_sPathSearchConfig;
  s << m_sNavmeshConfig;
  s << m_fReachedDistance;
  s << m_fSpeed;
  s << m_fAcceleration;
  s << m_fDecceleration;
  s << m_fFootRadius;
  s << m_uiCollisionLayer;
  s << m_fFallHeight;
  s << m_DebugFlags;
}

void ezAiNavigationComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  ezStreamReader& s = inout_stream.GetStream();
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_sPathSearchConfig;
  s >> m_sNavmeshConfig;
  s >> m_fReachedDistance;
  s >> m_fSpeed;
  s >> m_fAcceleration;
  s >> m_fDecceleration;
  s >> m_fFootRadius;
  s >> m_uiCollisionLayer;
  s >> m_fFallHeight;
  s >> m_DebugFlags;
}

void ezAiNavigationComponent::Update()
{
  if (m_uiSkipNextFrames > 0)
  {
    // in the very first frame, physics may not be available yet (colliders are not yet set up)
    // so skip that frame to prevent not finding a ground and entering the 'falling' state
    m_uiSkipNextFrames--;
    return;
  }

  ezTransform transform = GetOwner()->GetGlobalTransform();
  const float tDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  Steer(transform, tDiff);
  PlaceOnGround(transform, tDiff);

  GetOwner()->SetGlobalPosition(transform.m_vPosition);
  GetOwner()->SetGlobalRotation(transform.m_qRotation);

  if (m_DebugFlags.IsAnyFlagSet())
  {
    if (m_DebugFlags.IsSet(ezAiNavigationDebugFlags::VisPathCorridor))
    {
      m_Navigation.DebugDrawPathCorridor(GetWorld(), ezColor::Aquamarine.WithAlpha(0.15f), 0.2f);
    }

    if (m_DebugFlags.IsSet(ezAiNavigationDebugFlags::VisPathLine))
    {
      m_Navigation.DebugDrawPathLine(GetWorld(), ezColor::DeepSkyBlue, 0.3f);
    }

    if (m_DebugFlags.IsSet(ezAiNavigationDebugFlags::PrintState))
    {
      const ezVec3 vPosition = GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 1.5f);

      switch (m_State)
      {
        case ezAiNavigationComponentState::Idle:
          ezDebugRenderer::Draw3DText(GetWorld(), "Idle", vPosition, ezColor::Grey);
          break;
        case ezAiNavigationComponentState::Moving:
          ezDebugRenderer::Draw3DText(GetWorld(), "Moving", vPosition, ezColor::Yellow);
          m_Navigation.DebugDrawState(GetWorld(), vPosition - ezVec3(0, 0, 0.5f));
          break;
        case ezAiNavigationComponentState::Falling:
          ezDebugRenderer::Draw3DText(GetWorld(), "Falling...", vPosition, ezColor::IndianRed);
          break;
        case ezAiNavigationComponentState::Fallen:
          ezDebugRenderer::Draw3DText(GetWorld(), "Fallen", vPosition, ezColor::IndianRed);
          break;
        case ezAiNavigationComponentState::Failed:
          ezDebugRenderer::Draw3DText(GetWorld(), "Failed", vPosition, ezColor::Red);
          m_Navigation.DebugDrawState(GetWorld(), vPosition - ezVec3(0, 0, 0.5f));
          break;
      }
    }

    if (m_DebugFlags.IsSet(ezAiNavigationDebugFlags::VisTarget))
    {
      ezDebugRenderer::DrawArrow(GetWorld(), 1.0f, ezColor::Lime, m_Navigation.GetTargetPosition() + ezVec3(0, 0, 1.5f), -ezVec3::MakeAxisZ());
    }
  }
}

void ezAiNavigationComponent::Steer(ezTransform& transform, float tDiff)
{
  if (m_State != ezAiNavigationComponentState::Moving)
    return;

  if (ezAiNavMeshWorldModule* pNavMeshModule = GetWorld()->GetOrCreateModule<ezAiNavMeshWorldModule>())
  {
    m_Navigation.SetNavmesh(pNavMeshModule->GetNavMesh(m_sNavmeshConfig));
    m_Navigation.SetQueryFilter(pNavMeshModule->GetPathSearchFilter(m_sPathSearchConfig));
  }

  m_Navigation.SetCurrentPosition(GetOwner()->GetGlobalPosition());

  m_Navigation.Update();

  switch (m_Navigation.GetState())
  {
    case ezAiNavigation::State::Idle:
      m_State = ezAiNavigationComponentState::Idle;
      return;

    case ezAiNavigation::State::InvalidCurrentPosition:
    case ezAiNavigation::State::InvalidTargetPosition:
    case ezAiNavigation::State::NoPathFound:
      m_State = ezAiNavigationComponentState::Failed;
      return;

    case ezAiNavigation::State::StartNewSearch:
    case ezAiNavigation::State::Searching:
      return;

    case ezAiNavigation::State::FullPathFound:
      break;

    case ezAiNavigation::State::PartialPathFound:
      if (m_bAllowPartialPath)
        break;

      m_State = ezAiNavigationComponentState::Failed;
      return;
  }

  if (m_fSpeed <= 0)
    return;

  ezVec2 vForwardDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  vForwardDir.NormalizeIfNotZero(ezVec2(1, 0)).IgnoreResult();

  m_Steering.m_fMaxSpeed = m_fSpeed;
  m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
  m_Steering.m_vVelocity = GetOwner()->GetLinearVelocity();
  m_Steering.m_fAcceleration = m_fAcceleration;
  m_Steering.m_fDecceleration = m_fDecceleration;

  // TODO: hard-coded values
  m_Steering.m_MinTurnSpeed = ezAngle::MakeFromDegree(180);

  const float fBrakingDistance = 1.2f * (ezMath::Square(m_Steering.m_fMaxSpeed) / (2.0f * m_Steering.m_fDecceleration));

  m_Navigation.ComputeSteeringInfo(m_Steering.m_Info, vForwardDir, fBrakingDistance);
  m_Steering.Calculate(tDiff, GetWorld());

  const ezVec2 vMove = m_Steering.m_vPosition.GetAsVec2() - transform.m_vPosition.GetAsVec2();
  const float fSpeed = vMove.GetLength() / tDiff;

  transform.m_vPosition = m_Steering.m_vPosition;
  transform.m_qRotation = m_Steering.m_qRotation;

  if (fSpeed < 0.2f && (m_Navigation.GetTargetPosition().GetAsVec2() - m_Steering.m_vPosition.GetAsVec2()).GetLengthSquared() < ezMath::Square(m_fReachedDistance))
  {
    // reached the goal
    CancelNavigation();
    m_State = ezAiNavigationComponentState::Idle;
    return;
  }
}

void ezAiNavigationComponent::PlaceOnGround(ezTransform& transform, float tDiff)
{
  if (m_fFootRadius <= 0.0f)
    return;

  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  const ezVec3 vDown = -ezVec3::MakeAxisZ();
  const float fDistUp = 1.0f;
  const float fDistDown = m_fFallHeight;
  const ezVec3 vStartPos = transform.m_vPosition - fDistUp * vDown;

  float fMoveUp = 0.0f;
  bool bHadCollision = false;

  ezPhysicsCastResult res;
  ezPhysicsQueryParameters params(m_uiCollisionLayer, ezPhysicsShapeType::Static);
  if (pPhysicsInterface->SweepTestSphere(res, m_fFootRadius, vStartPos, vDown, fDistUp + fDistDown, params))
  {
    if (res.m_fDistance == 0.0f)
    {
      // ran into an obstacle
      // this shouldn't happen when walking just on the navmesh, as long as foot radius is smaller than the character radius
      // it can easily happen, once outside forces push the NPC around, but then one should really use a proper
      // physics character controller to avoid geometry
      return;
    }

    // found an intersection within the search radius
    bHadCollision = true;
    const float fFloorHeight = vStartPos.z - res.m_fDistance - m_fFootRadius;
    fMoveUp = fFloorHeight - transform.m_vPosition.z;
  }
  else
  {
    // did not find an intersection -> falling down
    fMoveUp = -fDistDown; // will be clamped by gravity
    CancelNavigation();
    m_State = ezAiNavigationComponentState::Falling;
  }

  if (fMoveUp > 0.0f)
  {
    // if the character is being pushed up

    // TODO: better lerp up
    transform.m_vPosition.z += fMoveUp * ezMath::Min(1.0f, 25.0f * tDiff);

    m_fFallSpeed = 0.0f;
  }
  else
  {
    m_fFallSpeed += pPhysicsInterface->GetGravity().z * tDiff;

    float fFallDist = m_fFallSpeed * tDiff;

    if (fFallDist < fMoveUp)
    {
      // clamp to the maximum speed / or to the floor
      fFallDist = fMoveUp;

      if (bHadCollision)
      {
        m_fFallSpeed = 0.0f;

        if (m_State == ezAiNavigationComponentState::Falling)
        {
          // we just landed from a high fall -> starting to walk again probably makes no sense, since we obviously left the navmesh
          m_State = ezAiNavigationComponentState::Fallen;
        }
      }
    }

    transform.m_vPosition.z += fFallDist;
  }
}


EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_NavigationComponent);

