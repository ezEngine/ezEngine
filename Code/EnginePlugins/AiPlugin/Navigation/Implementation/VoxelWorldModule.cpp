#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/VoxelGridSettingsComponent.h>
#include <AiPlugin/Navigation/VoxelWorldModule.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>

ezCVarBool cvar_VoxelGridVisualize("AI.VoxelGrid.Visualize", false, ezCVarFlags::None, "Visualize the voxel grid. Use AI.VoxelGrid.Visualize=true in the console to enable.");

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezAiVoxelWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiVoxelWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// SAT-based triangle-AABB overlap test (Tomas Akenine-Moller algorithm).
  ///
  /// Tests 13 potential separating axes: 3 box face normals, 1 triangle normal,
  /// and 9 cross products of box edge directions with triangle edge directions.
  bool TriangleBoxOverlap(const ezVec3& v0, const ezVec3& v1, const ezVec3& v2,
    const ezVec3& vBoxCenter, float fBoxHalf)
  {
    // Translate triangle so box center is at origin
    const ezVec3 a = v0 - vBoxCenter;
    const ezVec3 b = v1 - vBoxCenter;
    const ezVec3 c = v2 - vBoxCenter;
    const float h = fBoxHalf;

    // Test 3 AABB face normals
    if (ezMath::Min(a.x, ezMath::Min(b.x, c.x)) > h || ezMath::Max(a.x, ezMath::Max(b.x, c.x)) < -h) return false;
    if (ezMath::Min(a.y, ezMath::Min(b.y, c.y)) > h || ezMath::Max(a.y, ezMath::Max(b.y, c.y)) < -h) return false;
    if (ezMath::Min(a.z, ezMath::Min(b.z, c.z)) > h || ezMath::Max(a.z, ezMath::Max(b.z, c.z)) < -h) return false;

    // Triangle edges
    const ezVec3 f0 = b - a;
    const ezVec3 f1 = c - b;
    const ezVec3 f2 = a - c;

    // 9 cross product axes: box_axis_i x triangle_edge_j
    // For each axis, project all 3 vertices and check against box projection radius.
    float p0, p1, p2, r;

    // (1,0,0) x f0 = (0, -f0.z, f0.y)
    p0 = -a.y * f0.z + a.z * f0.y; p1 = -b.y * f0.z + b.z * f0.y; p2 = -c.y * f0.z + c.z * f0.y;
    r = h * (ezMath::Abs(f0.z) + ezMath::Abs(f0.y));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (1,0,0) x f1 = (0, -f1.z, f1.y)
    p0 = -a.y * f1.z + a.z * f1.y; p1 = -b.y * f1.z + b.z * f1.y; p2 = -c.y * f1.z + c.z * f1.y;
    r = h * (ezMath::Abs(f1.z) + ezMath::Abs(f1.y));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (1,0,0) x f2 = (0, -f2.z, f2.y)
    p0 = -a.y * f2.z + a.z * f2.y; p1 = -b.y * f2.z + b.z * f2.y; p2 = -c.y * f2.z + c.z * f2.y;
    r = h * (ezMath::Abs(f2.z) + ezMath::Abs(f2.y));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (0,1,0) x f0 = (f0.z, 0, -f0.x)
    p0 = a.x * f0.z - a.z * f0.x; p1 = b.x * f0.z - b.z * f0.x; p2 = c.x * f0.z - c.z * f0.x;
    r = h * (ezMath::Abs(f0.z) + ezMath::Abs(f0.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (0,1,0) x f1 = (f1.z, 0, -f1.x)
    p0 = a.x * f1.z - a.z * f1.x; p1 = b.x * f1.z - b.z * f1.x; p2 = c.x * f1.z - c.z * f1.x;
    r = h * (ezMath::Abs(f1.z) + ezMath::Abs(f1.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (0,1,0) x f2 = (f2.z, 0, -f2.x)
    p0 = a.x * f2.z - a.z * f2.x; p1 = b.x * f2.z - b.z * f2.x; p2 = c.x * f2.z - c.z * f2.x;
    r = h * (ezMath::Abs(f2.z) + ezMath::Abs(f2.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (0,0,1) x f0 = (-f0.y, f0.x, 0)
    p0 = -a.x * f0.y + a.y * f0.x; p1 = -b.x * f0.y + b.y * f0.x; p2 = -c.x * f0.y + c.y * f0.x;
    r = h * (ezMath::Abs(f0.y) + ezMath::Abs(f0.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (0,0,1) x f1 = (-f1.y, f1.x, 0)
    p0 = -a.x * f1.y + a.y * f1.x; p1 = -b.x * f1.y + b.y * f1.x; p2 = -c.x * f1.y + c.y * f1.x;
    r = h * (ezMath::Abs(f1.y) + ezMath::Abs(f1.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // (0,0,1) x f2 = (-f2.y, f2.x, 0)
    p0 = -a.x * f2.y + a.y * f2.x; p1 = -b.x * f2.y + b.y * f2.x; p2 = -c.x * f2.y + c.y * f2.x;
    r = h * (ezMath::Abs(f2.y) + ezMath::Abs(f2.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r) return false;

    // Triangle normal
    const ezVec3 normal = f0.CrossRH(f1);
    const float d = normal.Dot(a);
    r = h * (ezMath::Abs(normal.x) + ezMath::Abs(normal.y) + ezMath::Abs(normal.z));
    if (ezMath::Abs(d) > r) return false;

    return true;
  }

  /// Rasterizes a single triangle into the voxel grid by testing each voxel
  /// in the triangle's AABB for overlap using the SAT test.
  void RasterizeTriangle(ezVoxelGrid& grid, const ezVec3& v0, const ezVec3& v1, const ezVec3& v2)
  {
    // Triangle AABB in world space
    ezVec3 triMin;
    triMin.x = ezMath::Min(v0.x, ezMath::Min(v1.x, v2.x));
    triMin.y = ezMath::Min(v0.y, ezMath::Min(v1.y, v2.y));
    triMin.z = ezMath::Min(v0.z, ezMath::Min(v1.z, v2.z));

    ezVec3 triMax;
    triMax.x = ezMath::Max(v0.x, ezMath::Max(v1.x, v2.x));
    triMax.y = ezMath::Max(v0.y, ezMath::Max(v1.y, v2.y));
    triMax.z = ezMath::Max(v0.z, ezMath::Max(v1.z, v2.z));

    // Convert to voxel coordinates and clamp to grid bounds
    ezVec3I32 coordMin, coordMax;
    grid.WorldToCoord(triMin, coordMin);
    grid.WorldToCoord(triMax, coordMax);

    coordMin.x = ezMath::Max(coordMin.x, 0);
    coordMin.y = ezMath::Max(coordMin.y, 0);
    coordMin.z = ezMath::Max(coordMin.z, 0);
    coordMax.x = ezMath::Min(coordMax.x, (ezInt32)grid.GetDimX() - 1);
    coordMax.y = ezMath::Min(coordMax.y, (ezInt32)grid.GetDimY() - 1);
    coordMax.z = ezMath::Min(coordMax.z, (ezInt32)grid.GetDimZ() - 1);

    if (coordMin.x > coordMax.x || coordMin.y > coordMax.y || coordMin.z > coordMax.z)
      return;

    const float fHalfSize = grid.GetVoxelSize() * 0.5f;

    for (ezInt32 z = coordMin.z; z <= coordMax.z; ++z)
    {
      for (ezInt32 y = coordMin.y; y <= coordMax.y; ++y)
      {
        for (ezInt32 x = coordMin.x; x <= coordMax.x; ++x)
        {
          const ezVec3I32 vCoord(x, y, z);

          if (grid.CheckVoxel(vCoord))
            continue;

          const ezVec3 voxelCenter = grid.CoordToWorld(vCoord);

          if (TriangleBoxOverlap(v0, v1, v2, voxelCenter, fHalfSize))
          {
            grid.SetVoxel(vCoord, true);
          }
        }
      }
    }
  }
} // namespace

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
  m_StaticGrid.ClearData();

  const ezBoundingBox gridAABB = m_StaticGrid.GetAABB();
  const ezUInt32 uiDimX = m_StaticGrid.GetDimX();
  const ezUInt32 uiDimY = m_StaticGrid.GetDimY();
  const ezUInt32 uiDimZ = m_StaticGrid.GetDimZ();

  // Preferred path: retrieve actual collision geometry and rasterize triangles directly.
  // This is faster and more reliable than raycasting, as it uses the same geometry data
  // that the physics engine has.
  auto* pGeoModule = GetWorld()->GetOrCreateModule<ezNavmeshGeoWorldModuleInterface>();
  if (pGeoModule != nullptr)
  {
    ezDynamicArray<ezNavmeshTriangle> triangles;
    pGeoModule->RetrieveGeometryInArea(uiCollisionLayer, gridAABB, triangles);

    for (const auto& tri : triangles)
    {
      RasterizeTriangle(m_StaticGrid, tri.m_Vertices[0], tri.m_Vertices[1], tri.m_Vertices[2]);
    }

    const ezBoundingBox finalAABB = m_StaticGrid.GetAABB();
    ezLog::Info("ezAiVoxelWorldModule: Voxelized world from {} triangles ({}x{}x{}, voxel size {}). Grid bounds: ({}, {}, {}) to ({}, {}, {}).",
      triangles.GetCount(), uiDimX, uiDimY, uiDimZ, m_StaticGrid.GetVoxelSize(),
      finalAABB.m_vMin.x, finalAABB.m_vMin.y, finalAABB.m_vMin.z,
      finalAABB.m_vMax.x, finalAABB.m_vMax.y, finalAABB.m_vMax.z);
    return;
  }

  // Fallback: raycast along all 3 axes to mark surface voxels as solid.
  // Used when no navmesh geometry module is available (e.g. no physics engine loaded).
  auto* pPhysics = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
  {
    ezLog::Warning("ezAiVoxelWorldModule: No physics or geometry module available for voxelization.");
    return;
  }

  const ezPhysicsQueryParameters queryParams(uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  const float fVoxelSize = m_StaticGrid.GetVoxelSize();

  {
    ezPhysicsCastResultArray hitResults;
    const float fPadding = fVoxelSize * 0.5f;

    // Rays along +X axis
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

    // Rays along +Y axis
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

    // Rays along +Z axis
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
  ezLog::Info("ezAiVoxelWorldModule: Voxelized world via raycasts ({}x{}x{}, voxel size {}). Grid bounds: ({}, {}, {}) to ({}, {}, {}).",
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
