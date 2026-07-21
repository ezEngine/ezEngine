#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGridComponent.h>
#include <AiPlugin/Navigation3D/VoxelWorldModule.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezCVarBool cvar_VoxelGridVisualize("AI.VoxelGrid.Visualize", false, ezCVarFlags::None, "Visualize the voxel grid. Use AI.VoxelGrid.Visualize=true in the console to enable.");

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezAiVoxelWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiVoxelWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiVoxelWorldModule::ezAiVoxelWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

ezAiVoxelWorldModule::~ezAiVoxelWorldModule() = default;

void ezAiVoxelWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezAiVoxelWorldModule::Update, this);
    updateDesc.m_Phase = ezWorldUpdatePhase::PostTransform;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }
}

void ezAiVoxelWorldModule::Update(const UpdateContext& ctxt)
{
  if (m_uiUpdateDelay > 0)
  {
    --m_uiUpdateDelay;
    return;
  }

  auto* pGridManager = GetWorld()->GetComponentManager<ezAiVoxelGridComponentManager>();
  if (pGridManager == nullptr)
    return;

  for (auto it = pGridManager->GetComponents(); it.IsValid(); it.Next())
  {
    it->VoxelizeWorld();
  }

  m_bIsReady = true;

  for (auto it = pGridManager->GetComponents(); it.IsValid(); it.Next())
  {
    if (it->m_bVisualize || cvar_VoxelGridVisualize)
    {
      const ezVoxelGrid& grid = it->GetStaticVoxelGrid();
      grid.DebugDraw(GetWorld(), ezColor::LimeGreen.WithAlpha(0.1f));

      const ezVec3U32 dim = grid.GetDimensions();
      const ezUInt64 uiMemUsage = grid.GetHeapMemoryUsage();

      ezDebugRenderer::Draw3DText(GetWorld(), ezFmt("Voxel Grid\nCells: {} x {} x {}\nMemory: {}", dim.x, dim.y, dim.z, ezArgFileSize(uiMemUsage)), grid.GetCenter(), ezColor::White);
    }
  }
}

void ezAiVoxelWorldModule::FindGridsInBox(const ezBoundingBox& box, ezDynamicArray<ezAiVoxelGridComponent*>& out_grids) const
{
  out_grids.Clear();

  const ezSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
  if (pSpatialSystem == nullptr)
    return;

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = ezAiVoxelGridComponent::SpatialDataCategory.GetBitmask();

  ezDynamicArray<ezGameObject*> objects;
  pSpatialSystem->FindObjectsInBox(box, queryParams, objects);

  for (ezGameObject* pObject : objects)
  {
    ezAiVoxelGridComponent* pGridComponent = nullptr;
    if (pObject->TryGetComponentOfBaseType(pGridComponent))
    {
      out_grids.PushBack(pGridComponent);
    }
  }
}


EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation3D_Implementation_VoxelWorldModule);
