#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/VoxelGridSettingsComponent.h>
#include <AiPlugin/Navigation/VoxelWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>

ezCVarBool cvar_VoxelGridVisualize("AI.VoxelGrid.Visualize", false, ezCVarFlags::None, "Visualize the voxel grid.");

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

  m_bNeedsVoxelization = true;
}

void ezAiVoxelWorldModule::Deinitialize()
{
  SUPER::Deinitialize();
}

void ezAiVoxelWorldModule::Update(const UpdateContext& ctxt)
{
  if (m_uiUpdateDelay > 0)
  {
    --m_uiUpdateDelay;
    return;
  }

  if (m_bNeedsVoxelization)
  {
    m_bNeedsVoxelization = false;

    // Read settings from the settings component if one exists in the scene
    if (auto* pSettingsManager = GetWorld()->GetComponentManager<ezAiVoxelGridSettingsComponentManager>())
    {
      if (auto* pSettings = pSettingsManager->GetSingletonComponent())
      {
        m_uiResolutionX = pSettings->GetResolutionX();
        m_uiResolutionY = pSettings->GetResolutionY();
        m_uiResolutionZ = pSettings->GetResolutionZ();
        m_fVoxelSize = pSettings->GetVoxelSize();
        m_uiCollisionLayer = pSettings->GetCollisionLayer();
        m_vGridCenter = pSettings->GetOwner()->GetGlobalPosition();
      }
    }

    m_VoxelGrid.Init(m_uiResolutionX, m_uiResolutionY, m_uiResolutionZ);
    m_VoxelGrid.SetWorldParameters(m_vGridCenter, m_fVoxelSize);
    VoxelizeWorld(m_uiCollisionLayer);
    m_bIsReady = true;
  }

  if (cvar_VoxelGridVisualize)
  {
    m_VoxelGrid.DebugDraw(GetWorld(), ezColor::LimeGreen.WithAlpha(0.1f));
  }
}

void ezAiVoxelWorldModule::VoxelizeWorld(ezUInt32 uiCollisionLayer)
{
  auto* pPhysics = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
  {
    ezLog::Warning("ezAiVoxelWorldModule: No physics module available for voxelization.");
    return;
  }

  m_VoxelGrid.ClearData();

  const ezPhysicsQueryParameters queryParams(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  const float fVoxelSize = m_VoxelGrid.GetVoxelSize();
  const ezBoundingBox gridAABB = m_VoxelGrid.GetAABB();

  const ezUInt32 uiDimX = m_VoxelGrid.GetDimX();
  const ezUInt32 uiDimY = m_VoxelGrid.GetDimY();
  const ezUInt32 uiDimZ = m_VoxelGrid.GetDimZ();

  // Phase 1: Raycast along all 3 axes to catch thin geometry (walls, floors, ceilings).
  // For each row of voxels along an axis, cast a ray and mark hit voxels as solid.
  {
    ezPhysicsCastResultArray hitResults;
    const float fPadding = fVoxelSize * 0.5f;

    // Rays along +X axis (catches walls perpendicular to X)
    const float fRayLenX = gridAABB.m_vMax.x - gridAABB.m_vMin.x + fPadding * 2.0f;
    for (ezUInt32 z = 0; z < uiDimZ; ++z)
    {
      for (ezUInt32 y = 0; y < uiDimY; ++y)
      {
        const ezVec3 vRayStart(
          gridAABB.m_vMin.x - fPadding,
          gridAABB.m_vMin.y + (y + 0.5f) * fVoxelSize,
          gridAABB.m_vMin.z + (z + 0.5f) * fVoxelSize);

        hitResults.m_Results.Clear();
        if (pPhysics->RaycastAll(hitResults, vRayStart, ezVec3(1, 0, 0), fRayLenX, queryParams))
        {
          for (const auto& hit : hitResults.m_Results)
          {
            ezVec3I32 vCoord;
            if (m_VoxelGrid.WorldToCoord(hit.m_vPosition, vCoord))
            {
              m_VoxelGrid.SetVoxel(vCoord, true);
            }
          }
        }
      }
    }

    // Rays along +Y axis (catches walls perpendicular to Y)
    const float fRayLenY = gridAABB.m_vMax.y - gridAABB.m_vMin.y + fPadding * 2.0f;
    for (ezUInt32 z = 0; z < uiDimZ; ++z)
    {
      for (ezUInt32 x = 0; x < uiDimX; ++x)
      {
        const ezVec3 vRayStart(
          gridAABB.m_vMin.x + (x + 0.5f) * fVoxelSize,
          gridAABB.m_vMin.y - fPadding,
          gridAABB.m_vMin.z + (z + 0.5f) * fVoxelSize);

        hitResults.m_Results.Clear();
        if (pPhysics->RaycastAll(hitResults, vRayStart, ezVec3(0, 1, 0), fRayLenY, queryParams))
        {
          for (const auto& hit : hitResults.m_Results)
          {
            ezVec3I32 vCoord;
            if (m_VoxelGrid.WorldToCoord(hit.m_vPosition, vCoord))
            {
              m_VoxelGrid.SetVoxel(vCoord, true);
            }
          }
        }
      }
    }

    // Rays along +Z axis (catches floors and ceilings)
    const float fRayLenZ = gridAABB.m_vMax.z - gridAABB.m_vMin.z + fPadding * 2.0f;
    for (ezUInt32 y = 0; y < uiDimY; ++y)
    {
      for (ezUInt32 x = 0; x < uiDimX; ++x)
      {
        const ezVec3 vRayStart(
          gridAABB.m_vMin.x + (x + 0.5f) * fVoxelSize,
          gridAABB.m_vMin.y + (y + 0.5f) * fVoxelSize,
          gridAABB.m_vMin.z - fPadding);

        hitResults.m_Results.Clear();
        if (pPhysics->RaycastAll(hitResults, vRayStart, ezVec3(0, 0, 1), fRayLenZ, queryParams))
        {
          for (const auto& hit : hitResults.m_Results)
          {
            ezVec3I32 vCoord;
            if (m_VoxelGrid.WorldToCoord(hit.m_vPosition, vCoord))
            {
              m_VoxelGrid.SetVoxel(vCoord, true);
            }
          }
        }
      }
    }
  }

  // Phase 2: Overlap test to fill in volumetric geometry that rays might pass through.
  // Use a slightly expanded box to catch edges.
  {
    const float fOverlapExtent = fVoxelSize * 1.1f;
    const ezVec3 vBoxExtents(fOverlapExtent, fOverlapExtent, fOverlapExtent);

    for (ezUInt32 z = 0; z < uiDimZ; ++z)
    {
      for (ezUInt32 y = 0; y < uiDimY; ++y)
      {
        for (ezUInt32 x = 0; x < uiDimX; ++x)
        {
          const ezVec3I32 vCoord((ezInt32)x, (ezInt32)y, (ezInt32)z);

          // Skip already-marked voxels from raycast phase
          if (m_VoxelGrid.CheckVoxel(vCoord))
            continue;

          const ezVec3 vWorldPos = m_VoxelGrid.CoordToWorld(vCoord);

          ezTransform xform = ezTransform::MakeIdentity();
          xform.m_vPosition = vWorldPos;

          if (pPhysics->OverlapTestBox(vBoxExtents, vWorldPos, xform, queryParams))
          {
            m_VoxelGrid.SetVoxel(vCoord, true);
          }
        }
      }
    }
  }

  const ezBoundingBox finalAABB = m_VoxelGrid.GetAABB();
  ezLog::Info("ezAiVoxelWorldModule: Voxelized world ({}x{}x{}, voxel size {}). Grid bounds: ({}, {}, {}) to ({}, {}, {}).",
    uiDimX, uiDimY, uiDimZ, m_VoxelGrid.GetVoxelSize(),
    finalAABB.m_vMin.x, finalAABB.m_vMin.y, finalAABB.m_vMin.z,
    finalAABB.m_vMax.x, finalAABB.m_vMax.y, finalAABB.m_vMax.z);
}

void ezAiVoxelWorldModule::InjectObstacle(const ezBoundingBox& box)
{
  m_VoxelGrid.InjectBox(box);
}

void ezAiVoxelWorldModule::RemoveObstacle(const ezBoundingBox& box)
{
  m_VoxelGrid.SubtractBox(box);
}

EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Implementation_VoxelWorldModule);
