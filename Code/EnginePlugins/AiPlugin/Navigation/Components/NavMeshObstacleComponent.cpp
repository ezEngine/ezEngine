#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavMeshObstacleComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezNavMeshObstacleComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(InvalidateSectors),
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

ezNavMeshObstacleComponent::ezNavMeshObstacleComponent() = default;
ezNavMeshObstacleComponent::~ezNavMeshObstacleComponent() = default;

void ezNavMeshObstacleComponent::OnActivated()
{
  SUPER::OnActivated();

  if (IsSimulationStarted())
    InvalidateSectors();
}

void ezNavMeshObstacleComponent::OnSimulationStarted()
{
  ezComponent::OnSimulationStarted();

  InvalidateSectors();
}

void ezNavMeshObstacleComponent::OnDeactivated()
{
  InvalidateSectors();

  SUPER::OnDeactivated();
}

void ezNavMeshObstacleComponent::InvalidateSectors()
{
  // TODO: dynamic obstacles not implemented yet
  if (GetOwner()->IsDynamic())
    return;

  auto* pPhysics = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
    return;

  auto* pNavMeshModule = GetWorld()->GetOrCreateModule<ezAiNavMeshWorldModule>();
  if (pNavMeshModule == nullptr)
    return;

  for (const auto& navConfig : pNavMeshModule->GetConfig().m_NavmeshConfigs)
  {
    ezAiNavMesh* pNavMesh = pNavMeshModule->GetNavMesh(navConfig.m_sName);
    ezUInt8 uiCollisionLayer = navConfig.m_uiCollisionLayer;

    // TODO: change to ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic when dynamic obstacles are supported
    auto bounds = pPhysics->GetWorldSpaceBounds(GetOwner(), uiCollisionLayer, ezPhysicsShapeType::Static, true);
    if (bounds.IsValid())
    {
      pNavMesh->InvalidateSector(bounds.GetBox().GetCenter().GetAsVec2(), bounds.GetBox().GetHalfExtents().GetAsVec2(), false);
    }
  }
}


EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_NavMeshObstacleComponent);
