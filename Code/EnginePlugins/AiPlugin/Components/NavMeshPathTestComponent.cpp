#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Components/NavMeshPathTestComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAiNavMeshPathTestComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("VisualizePathCorridor", m_bVisualizePathCorridor)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("VisualizePathLine", m_bVisualizePathLine)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("VisualizePathState", m_bVisualizePathState)->AddAttributes(new ezDefaultValueAttribute(true)),

    EZ_ACCESSOR_PROPERTY("PathEnd", DummyGetter, SetPathEndReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new ezDynamicStringEnumAttribute("AiNavmeshConfig")),
    EZ_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new ezDynamicStringEnumAttribute("AiPathSearchConfig")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Navigation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAiNavMeshPathTestComponent::ezAiNavMeshPathTestComponent() = default;
ezAiNavMeshPathTestComponent::~ezAiNavMeshPathTestComponent() = default;

void ezAiNavMeshPathTestComponent::SetPathEndReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetPathEnd(resolver(szReference, GetHandle(), "PathEnd"));
}

void ezAiNavMeshPathTestComponent::SetPathEnd(ezGameObjectHandle hObject)
{
  m_hPathEnd = hObject;
}

void ezAiNavMeshPathTestComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hPathEnd);
  s << m_sPathSearchConfig;
  s << m_sNavmeshConfig;
  s << m_bVisualizePathCorridor;
  s << m_bVisualizePathLine;
  s << m_bVisualizePathState;
}

void ezAiNavMeshPathTestComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  ezStreamReader& s = inout_stream.GetStream();
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  m_hPathEnd = inout_stream.ReadGameObjectHandle();
  s >> m_sPathSearchConfig;
  s >> m_sNavmeshConfig;
  s >> m_bVisualizePathCorridor;
  s >> m_bVisualizePathLine;
  s >> m_bVisualizePathState;
}

void ezAiNavMeshPathTestComponent::Update()
{
  if (m_hPathEnd.IsInvalidated())
    return;

  ezGameObject* pEnd = nullptr;
  if (!GetWorld()->TryGetObject(m_hPathEnd, pEnd))
    return;

  m_Navigation.SetCurrentPosition(GetOwner()->GetGlobalPosition());
  m_Navigation.SetTargetPosition(pEnd->GetGlobalPosition());

  if (ezAiNavMeshWorldModule* pNavMeshModule = GetWorld()->GetOrCreateModule<ezAiNavMeshWorldModule>())
  {
    m_Navigation.SetNavmesh(*pNavMeshModule->GetNavMesh(m_sNavmeshConfig));
    m_Navigation.SetQueryFilter(pNavMeshModule->GetPathSearchFilter(m_sPathSearchConfig));
  }

  m_Navigation.Update();

  if (m_bVisualizePathCorridor || m_bVisualizePathLine)
  {
    m_Navigation.DebugDraw(GetWorld(), m_bVisualizePathCorridor ? ezColor::Aquamarine.WithAlpha(0.2f) : ezColor::MakeZero(), m_bVisualizePathLine ? ezColor::Lime : ezColor::MakeZero());
  }

  if (m_bVisualizePathState)
  {
    const ezAiNavigation::State state = m_Navigation.GetState();

    switch (state)
    {
      case ezAiNavigation::State::Idle:
        ezDebugRenderer::Draw3DText(GetWorld(), "Idle", GetOwner()->GetGlobalPosition(), ezColor::Grey);
        break;
      case ezAiNavigation::State::StartNewSearch:
        ezDebugRenderer::Draw3DText(GetWorld(), "Starting...", GetOwner()->GetGlobalPosition(), ezColor::Yellow);
        break;
      case ezAiNavigation::State::InvalidCurrentPosition:
        ezDebugRenderer::Draw3DText(GetWorld(), "Invalid Start Position", GetOwner()->GetGlobalPosition(), ezColor::Black);
        break;
      case ezAiNavigation::State::InvalidTargetPosition:
        ezDebugRenderer::Draw3DText(GetWorld(), "Invalid Target Position", GetOwner()->GetGlobalPosition(), ezColor::IndianRed);
        break;
      case ezAiNavigation::State::NoPathFound:
        ezDebugRenderer::Draw3DText(GetWorld(), "No Path Found", GetOwner()->GetGlobalPosition(), ezColor::White);
        break;
      case ezAiNavigation::State::PartialPathFound:
        ezDebugRenderer::Draw3DText(GetWorld(), "Partial Path Found", GetOwner()->GetGlobalPosition(), ezColor::Turquoise);
        break;
      case ezAiNavigation::State::FullPathFound:
        ezDebugRenderer::Draw3DText(GetWorld(), "Full Path Found", GetOwner()->GetGlobalPosition(), ezColor::LawnGreen);
        break;
      case ezAiNavigation::State::Searching:
        ezDebugRenderer::Draw3DText(GetWorld(), "Searching...", GetOwner()->GetGlobalPosition(), ezColor::Yellow);
        break;
    }
  }

  // if (m_fSpeed <= 0)
  //   return;

  // ezVec2 vForwardDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  // vForwardDir.NormalizeIfNotZero(ezVec2(1, 0)).IgnoreResult();

  // m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  // m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
  // m_Steering.m_vVelocity = GetOwner()->GetLinearVelocity();
  // m_Steering.m_fMaxSpeed = m_fSpeed;
  // m_Steering.m_MinTurnSpeed = m_TurnSpeed;
  // m_Steering.m_fAcceleration = m_fAcceleration;
  // m_Steering.m_fDecceleration = m_fDecceleration;

  // const float fBrakingDistance = 1.2f * (ezMath::Square(m_Steering.m_fMaxSpeed) / (2.0f * m_Steering.m_fDecceleration));

  // m_Navigation.ComputeSteeringInfo(m_Steering.m_Info, vForwardDir, fBrakingDistance);
  // m_Steering.Calculate(GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds(), GetWorld());

  // m_Steering.m_vPosition.z = m_Navigation.GetCurrentElevation();

  // GetOwner()->SetGlobalPosition(m_Steering.m_vPosition);
  // GetOwner()->SetGlobalRotation(m_Steering.m_qRotation);
}
