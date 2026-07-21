#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGridComponent.h>
#include <AiPlugin/Navigation3D/VoxelPathTestComponent.h>
#include <AiPlugin/Navigation3D/VoxelWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAiVoxelPathTestComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("PathEnd", DummyGetter, SetPathEndReference)->AddAttributes(new ezGameObjectReferenceAttribute()),

    EZ_MEMBER_PROPERTY("VisualizeSmoothedPath", m_bVisualizeSmoothedPath)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("VisualizePathState", m_bVisualizePathState)->AddAttributes(new ezDefaultValueAttribute(true)),

    EZ_MEMBER_PROPERTY("SearchMargin", m_fSearchMargin)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
    EZ_MEMBER_PROPERTY("MaxIterationsPerHop", m_uiMaxIterationsPerHop)->AddAttributes(new ezDefaultValueAttribute(10000)),
    EZ_MEMBER_PROPERTY("MaxHops", m_uiMaxHops)->AddAttributes(new ezDefaultValueAttribute(16)),
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

ezAiVoxelPathTestComponent::ezAiVoxelPathTestComponent() = default;
ezAiVoxelPathTestComponent::~ezAiVoxelPathTestComponent() = default;

void ezAiVoxelPathTestComponent::SetPathEndReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetPathEnd(resolver(szReference, GetHandle(), "PathEnd"));
}

void ezAiVoxelPathTestComponent::SetPathEnd(ezGameObjectHandle hObject)
{
  m_hPathEnd = hObject;
}

void ezAiVoxelPathTestComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hPathEnd);
  s << m_bVisualizeSmoothedPath;
  s << m_bVisualizePathState;
  s << m_fSearchMargin;
  s << m_uiMaxIterationsPerHop;
  s << m_uiMaxHops;
}

void ezAiVoxelPathTestComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  ezStreamReader& s = inout_stream.GetStream();
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  m_hPathEnd = inout_stream.ReadGameObjectHandle();
  s >> m_bVisualizeSmoothedPath;
  s >> m_bVisualizePathState;
  s >> m_fSearchMargin;
  s >> m_uiMaxIterationsPerHop;
  s >> m_uiMaxHops;
}

void ezAiVoxelPathTestComponent::Update()
{
  if (m_hPathEnd.IsInvalidated())
    return;

  ezGameObject* pEnd = nullptr;
  if (!GetWorld()->TryGetObject(m_hPathEnd, pEnd))
    return;

  auto* pVoxelModule = GetWorld()->GetOrCreateModule<ezAiVoxelWorldModule>();
  if (pVoxelModule == nullptr || !pVoxelModule->IsReady())
    return;

  const ezVec3 vStart = GetOwner()->GetGlobalPosition();
  const ezVec3 vTarget = pEnd->GetGlobalPosition();

  const ezAiVoxelGridFinder gridFinder = [pVoxelModule](const ezBoundingBox& searchBox, ezDynamicArray<const ezVoxelGrid*>& out_grids)
  {
    ezHybridArray<ezAiVoxelGridComponent*, 4> gridComponents;
    pVoxelModule->FindGridsInBox(searchBox, gridComponents);

    out_grids.Clear();
    for (const ezAiVoxelGridComponent* pGridComponent : gridComponents)
      out_grids.PushBack(&pGridComponent->GetStaticVoxelGrid());
  };

  const auto state = m_Navigation.FindPath(vStart, vTarget, gridFinder, m_fSearchMargin, m_uiMaxIterationsPerHop, m_uiMaxHops);

  if (m_bVisualizeSmoothedPath)
  {
    m_Navigation.DebugDrawPathSegments(GetWorld(), ezColor::DeepSkyBlue, ezColor::Yellow);
  }

  if (m_bVisualizePathState)
  {
    const ezVec3 vTextPos = GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 0.5f);

    switch (state)
    {
      case ezAiVoxelNavigation::State::Idle:
        ezDebugRenderer::Draw3DText(GetWorld(), "Idle", vTextPos, ezColor::Grey);
        break;
      case ezAiVoxelNavigation::State::PathFound:
        ezDebugRenderer::Draw3DText(GetWorld(), "Path Found", vTextPos, ezColor::LawnGreen);
        break;
      case ezAiVoxelNavigation::State::NoPathFound:
        ezDebugRenderer::Draw3DText(GetWorld(), "No Path Found", vTextPos, ezColor::IndianRed);
        break;
      case ezAiVoxelNavigation::State::InvalidStartPosition:
        ezDebugRenderer::Draw3DText(GetWorld(), "Invalid Start Position", vTextPos, ezColor::IndianRed);
        break;
      case ezAiVoxelNavigation::State::InvalidTargetPosition:
        ezDebugRenderer::Draw3DText(GetWorld(), "Invalid Target Position", vTextPos, ezColor::IndianRed);
        break;
    }
  }
}

EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation3D_Implementation_VoxelPathTestComponent);
