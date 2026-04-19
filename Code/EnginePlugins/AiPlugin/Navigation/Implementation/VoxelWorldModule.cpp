#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/VoxelGridSettingsComponent.h>
#include <AiPlugin/Navigation/VoxelWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>

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

    m_StaticGrid.Init(m_uiResolutionX, m_uiResolutionY, m_uiResolutionZ);
    m_StaticGrid.SetWorldParameters(m_vGridCenter, m_fVoxelSize);
    VoxelizeWorld(m_uiCollisionLayer);

    // Dynamic grid shares the same dimensions and world parameters as the static grid.
    // The actual dimensions may be rounded up to 4-voxel boundaries by ezVoxelGrid::Init.
    m_DynamicGrid.Init(m_StaticGrid.GetDimX(), m_StaticGrid.GetDimY(), m_StaticGrid.GetDimZ());
    m_DynamicGrid.SetWorldParameters(m_vGridCenter, m_fVoxelSize);

    const ezUInt32 uiTotalVoxels = m_StaticGrid.GetDimX() * m_StaticGrid.GetDimY() * m_StaticGrid.GetDimZ();
    m_ObstacleCounts.SetCount(uiTotalVoxels, 0);

    m_bIsReady = true;
  }

  if (cvar_VoxelGridVisualize)
  {
    m_StaticGrid.DebugDraw(GetWorld(), ezColor::LimeGreen.WithAlpha(0.1f));
    m_DynamicGrid.DebugDraw(GetWorld(), ezColor::OrangeRed.WithAlpha(0.3f));
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

  m_StaticGrid.ClearData();

  const ezPhysicsQueryParameters queryParams(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  const float fVoxelSize = m_StaticGrid.GetVoxelSize();
  const ezBoundingBox gridAABB = m_StaticGrid.GetAABB();

  const ezUInt32 uiDimX = m_StaticGrid.GetDimX();
  const ezUInt32 uiDimY = m_StaticGrid.GetDimY();
  const ezUInt32 uiDimZ = m_StaticGrid.GetDimZ();

  // Cast rays along all 3 axes to mark surface voxels as solid.
  // Three sets of parallel rays (one set per axis) ensure every surface is hit regardless of orientation.
  // Interior voxels deep inside solid objects need not be marked — no navigation agent can reach them
  // from outside without crossing the already-blocked surface voxels.
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
            if (m_StaticGrid.WorldToCoord(hit.m_vPosition, vCoord))
            {
              m_StaticGrid.SetVoxel(vCoord, true);
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
            if (m_StaticGrid.WorldToCoord(hit.m_vPosition, vCoord))
            {
              m_StaticGrid.SetVoxel(vCoord, true);
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
            if (m_StaticGrid.WorldToCoord(hit.m_vPosition, vCoord))
            {
              m_StaticGrid.SetVoxel(vCoord, true);
            }
          }
        }
      }
    }
  }

  const ezBoundingBox finalAABB = m_StaticGrid.GetAABB();
  ezLog::Info("ezAiVoxelWorldModule: Voxelized world ({}x{}x{}, voxel size {}). Grid bounds: ({}, {}, {}) to ({}, {}, {}).",
    uiDimX, uiDimY, uiDimZ, m_StaticGrid.GetVoxelSize(),
    finalAABB.m_vMin.x, finalAABB.m_vMin.y, finalAABB.m_vMin.z,
    finalAABB.m_vMax.x, finalAABB.m_vMax.y, finalAABB.m_vMax.z);
}

void ezAiVoxelWorldModule::InjectObstacle(const ezBoundingBox& box)
{
  if (!m_StaticGrid.IsInitialized())
    return;

  ezVec3I32 vMin, vMax;
  m_StaticGrid.WorldToCoord(box.m_vMin, vMin);
  m_StaticGrid.WorldToCoord(box.m_vMax, vMax);

  vMin.x = ezMath::Max(vMin.x, 0);
  vMin.y = ezMath::Max(vMin.y, 0);
  vMin.z = ezMath::Max(vMin.z, 0);
  vMax.x = ezMath::Min(vMax.x, (ezInt32)m_StaticGrid.GetDimX() - 1);
  vMax.y = ezMath::Min(vMax.y, (ezInt32)m_StaticGrid.GetDimY() - 1);
  vMax.z = ezMath::Min(vMax.z, (ezInt32)m_StaticGrid.GetDimZ() - 1);

  if (vMin.x > vMax.x || vMin.y > vMax.y || vMin.z > vMax.z)
    return;

  const ezUInt32 uiDimX = m_StaticGrid.GetDimX();
  const ezUInt32 uiDimY = m_StaticGrid.GetDimY();

  for (ezInt32 z = vMin.z; z <= vMax.z; ++z)
  {
    for (ezInt32 y = vMin.y; y <= vMax.y; ++y)
    {
      for (ezInt32 x = vMin.x; x <= vMax.x; ++x)
      {
        const ezUInt32 uiIndex = (ezUInt32)z * uiDimX * uiDimY + (ezUInt32)y * uiDimX + (ezUInt32)x;
        const ezUInt8 uiPrevCount = m_ObstacleCounts[uiIndex];
        if (uiPrevCount < 255u)
          m_ObstacleCounts[uiIndex] = uiPrevCount + 1;
        if (uiPrevCount == 0)
          m_DynamicGrid.SetVoxel(ezVec3I32(x, y, z), true);
      }
    }
  }
}

void ezAiVoxelWorldModule::RemoveObstacle(const ezBoundingBox& box)
{
  if (!m_StaticGrid.IsInitialized())
    return;

  ezVec3I32 vMin, vMax;
  m_StaticGrid.WorldToCoord(box.m_vMin, vMin);
  m_StaticGrid.WorldToCoord(box.m_vMax, vMax);

  vMin.x = ezMath::Max(vMin.x, 0);
  vMin.y = ezMath::Max(vMin.y, 0);
  vMin.z = ezMath::Max(vMin.z, 0);
  vMax.x = ezMath::Min(vMax.x, (ezInt32)m_StaticGrid.GetDimX() - 1);
  vMax.y = ezMath::Min(vMax.y, (ezInt32)m_StaticGrid.GetDimY() - 1);
  vMax.z = ezMath::Min(vMax.z, (ezInt32)m_StaticGrid.GetDimZ() - 1);

  if (vMin.x > vMax.x || vMin.y > vMax.y || vMin.z > vMax.z)
    return;

  const ezUInt32 uiDimX = m_StaticGrid.GetDimX();
  const ezUInt32 uiDimY = m_StaticGrid.GetDimY();

  for (ezInt32 z = vMin.z; z <= vMax.z; ++z)
  {
    for (ezInt32 y = vMin.y; y <= vMax.y; ++y)
    {
      for (ezInt32 x = vMin.x; x <= vMax.x; ++x)
      {
        const ezUInt32 uiIndex = (ezUInt32)z * uiDimX * uiDimY + (ezUInt32)y * uiDimX + (ezUInt32)x;
        if (m_ObstacleCounts[uiIndex] > 0)
        {
          m_ObstacleCounts[uiIndex]--;
          if (m_ObstacleCounts[uiIndex] == 0)
            m_DynamicGrid.SetVoxel(ezVec3I32(x, y, z), false);
        }
      }
    }
  }
}

EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Implementation_VoxelWorldModule);
