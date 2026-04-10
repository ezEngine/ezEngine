#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/VoxelObstacleComponent.h>
#include <AiPlugin/Navigation/VoxelWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAiVoxelObstacleComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(UpdateObstacle),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Navigation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAiVoxelObstacleComponent::ezAiVoxelObstacleComponent() = default;
ezAiVoxelObstacleComponent::~ezAiVoxelObstacleComponent() = default;

void ezAiVoxelObstacleComponent::OnActivated()
{
  SUPER::OnActivated();

  if (IsSimulationStarted())
    InjectIntoGrid();
}

void ezAiVoxelObstacleComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  InjectIntoGrid();
}

void ezAiVoxelObstacleComponent::OnDeactivated()
{
  RemoveFromGrid();

  SUPER::OnDeactivated();
}

void ezAiVoxelObstacleComponent::UpdateObstacle()
{
  RemoveFromGrid();
  InjectIntoGrid();
}

void ezAiVoxelObstacleComponent::InjectIntoGrid()
{
  auto* pPhysics = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
    return;

  auto* pVoxelModule = GetWorld()->GetOrCreateModule<ezAiVoxelWorldModule>();
  if (pVoxelModule == nullptr)
    return;

  auto bounds = pPhysics->GetWorldSpaceBounds(GetOwner(), m_uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, true);
  if (!bounds.IsValid())
    return;

  const ezBoundingBox box = bounds.GetBox();
  pVoxelModule->InjectObstacle(box);
  m_LastInjectedBounds = box;
  m_bInjected = true;
}

void ezAiVoxelObstacleComponent::RemoveFromGrid()
{
  if (!m_bInjected)
    return;

  auto* pVoxelModule = GetWorld()->GetModule<ezAiVoxelWorldModule>();
  if (pVoxelModule == nullptr)
    return;

  pVoxelModule->RemoveObstacle(m_LastInjectedBounds);
  m_bInjected = false;
}

void ezAiVoxelObstacleComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
}

void ezAiVoxelObstacleComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
}

EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_VoxelObstacleComponent);
