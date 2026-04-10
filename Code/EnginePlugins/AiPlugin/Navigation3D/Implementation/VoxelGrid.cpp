#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <Foundation/Math/Math.h>
#include <RendererCore/Debug/DebugRenderer.h>

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
    if (ezMath::Min(a.x, ezMath::Min(b.x, c.x)) > h || ezMath::Max(a.x, ezMath::Max(b.x, c.x)) < -h)
      return false;
    if (ezMath::Min(a.y, ezMath::Min(b.y, c.y)) > h || ezMath::Max(a.y, ezMath::Max(b.y, c.y)) < -h)
      return false;
    if (ezMath::Min(a.z, ezMath::Min(b.z, c.z)) > h || ezMath::Max(a.z, ezMath::Max(b.z, c.z)) < -h)
      return false;

    // Triangle edges
    const ezVec3 f0 = b - a;
    const ezVec3 f1 = c - b;
    const ezVec3 f2 = a - c;

    // 9 cross product axes: box_axis_i x triangle_edge_j
    // For each axis, project all 3 vertices and check against box projection radius.
    float p0, p1, p2, r;

    // (1,0,0) x f0 = (0, -f0.z, f0.y)
    p0 = -a.y * f0.z + a.z * f0.y;
    p1 = -b.y * f0.z + b.z * f0.y;
    p2 = -c.y * f0.z + c.z * f0.y;
    r = h * (ezMath::Abs(f0.z) + ezMath::Abs(f0.y));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (1,0,0) x f1 = (0, -f1.z, f1.y)
    p0 = -a.y * f1.z + a.z * f1.y;
    p1 = -b.y * f1.z + b.z * f1.y;
    p2 = -c.y * f1.z + c.z * f1.y;
    r = h * (ezMath::Abs(f1.z) + ezMath::Abs(f1.y));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (1,0,0) x f2 = (0, -f2.z, f2.y)
    p0 = -a.y * f2.z + a.z * f2.y;
    p1 = -b.y * f2.z + b.z * f2.y;
    p2 = -c.y * f2.z + c.z * f2.y;
    r = h * (ezMath::Abs(f2.z) + ezMath::Abs(f2.y));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (0,1,0) x f0 = (f0.z, 0, -f0.x)
    p0 = a.x * f0.z - a.z * f0.x;
    p1 = b.x * f0.z - b.z * f0.x;
    p2 = c.x * f0.z - c.z * f0.x;
    r = h * (ezMath::Abs(f0.z) + ezMath::Abs(f0.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (0,1,0) x f1 = (f1.z, 0, -f1.x)
    p0 = a.x * f1.z - a.z * f1.x;
    p1 = b.x * f1.z - b.z * f1.x;
    p2 = c.x * f1.z - c.z * f1.x;
    r = h * (ezMath::Abs(f1.z) + ezMath::Abs(f1.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (0,1,0) x f2 = (f2.z, 0, -f2.x)
    p0 = a.x * f2.z - a.z * f2.x;
    p1 = b.x * f2.z - b.z * f2.x;
    p2 = c.x * f2.z - c.z * f2.x;
    r = h * (ezMath::Abs(f2.z) + ezMath::Abs(f2.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (0,0,1) x f0 = (-f0.y, f0.x, 0)
    p0 = -a.x * f0.y + a.y * f0.x;
    p1 = -b.x * f0.y + b.y * f0.x;
    p2 = -c.x * f0.y + c.y * f0.x;
    r = h * (ezMath::Abs(f0.y) + ezMath::Abs(f0.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (0,0,1) x f1 = (-f1.y, f1.x, 0)
    p0 = -a.x * f1.y + a.y * f1.x;
    p1 = -b.x * f1.y + b.y * f1.x;
    p2 = -c.x * f1.y + c.y * f1.x;
    r = h * (ezMath::Abs(f1.y) + ezMath::Abs(f1.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // (0,0,1) x f2 = (-f2.y, f2.x, 0)
    p0 = -a.x * f2.y + a.y * f2.x;
    p1 = -b.x * f2.y + b.y * f2.x;
    p2 = -c.x * f2.y + c.y * f2.x;
    r = h * (ezMath::Abs(f2.y) + ezMath::Abs(f2.x));
    if (ezMath::Min(p0, ezMath::Min(p1, p2)) > r || ezMath::Max(p0, ezMath::Max(p1, p2)) < -r)
      return false;

    // Triangle normal
    const ezVec3 normal = f0.CrossRH(f1);
    const float d = normal.Dot(a);
    r = h * (ezMath::Abs(normal.x) + ezMath::Abs(normal.y) + ezMath::Abs(normal.z));
    if (ezMath::Abs(d) > r)
      return false;

    return true;
  }
} // namespace

ezVoxelGrid::ezVoxelGrid() = default;
ezVoxelGrid::~ezVoxelGrid() = default;

void ezVoxelGrid::Initialize(const ezVec3U32& vDimensions, const ezVec3& vCenter, float fVoxelSize)
{
  EZ_ASSERT_DEV(fVoxelSize > 0.0f, "Voxel size must be positive");

  m_vDimensions.x = ezMath::Max(4u, vDimensions.x);
  m_vDimensions.y = ezMath::Max(4u, vDimensions.y);
  m_vDimensions.z = ezMath::Max(4u, vDimensions.z);

  m_vNumBlocks.x = (m_vDimensions.x + 3u) / 4u;
  m_vNumBlocks.y = (m_vDimensions.y + 3u) / 4u;
  m_vNumBlocks.z = (m_vDimensions.z + 3u) / 4u;

  // Round dims up to block boundaries
  m_vDimensions.x = m_vNumBlocks.x * 4u;
  m_vDimensions.y = m_vNumBlocks.y * 4u;
  m_vDimensions.z = m_vNumBlocks.z * 4u;

  m_Blocks.Clear();
  m_Blocks.SetCount(m_vNumBlocks.x * m_vNumBlocks.y * m_vNumBlocks.z);

  m_vCenter = vCenter;
  m_fVoxelSize = fVoxelSize;
  m_fInvVoxelSize = 1.0f / fVoxelSize;

  const ezVec3 vHalfExtents(
    m_vDimensions.x * 0.5f * m_fVoxelSize,
    m_vDimensions.y * 0.5f * m_fVoxelSize,
    m_vDimensions.z * 0.5f * m_fVoxelSize);

  m_vOrigin = m_vCenter - vHalfExtents;
}

void ezVoxelGrid::ClearData()
{
  ezMemoryUtils::ZeroFill(m_Blocks.GetData(), m_Blocks.GetCount());
}

void ezVoxelGrid::SetVoxel(const ezVec3I32& vCoord, bool bSolid)
{
  EZ_ASSERT_DEBUG(IsCoordValid(vCoord), "Invalid coordinate");

  const ezUInt32 uiX = (ezUInt32)vCoord.x;
  const ezUInt32 uiY = (ezUInt32)vCoord.y;
  const ezUInt32 uiZ = (ezUInt32)vCoord.z;

  const ezUInt32 uiBlockIdx = GetBlockIndex(uiX / 4u, uiY / 4u, uiZ / 4u);
  const ezUInt32 uiBit = GetBitIndex(uiX % 4u, uiY % 4u, uiZ % 4u);
  const ezUInt64 uiMask = 1ull << uiBit;

  if (bSolid)
  {
    m_Blocks[uiBlockIdx] |= uiMask;
  }
  else
  {
    m_Blocks[uiBlockIdx] &= ~uiMask;
  }
}

bool ezVoxelGrid::IsVoxelSet(const ezVec3I32& vCoord) const
{
  EZ_ASSERT_DEBUG(IsCoordValid(vCoord), "Invalid coordinate");

  const ezUInt32 uiX = (ezUInt32)vCoord.x;
  const ezUInt32 uiY = (ezUInt32)vCoord.y;
  const ezUInt32 uiZ = (ezUInt32)vCoord.z;

  const ezUInt32 uiBlockIdx = GetBlockIndex(uiX / 4u, uiY / 4u, uiZ / 4u);
  const ezUInt64 uiBlock = m_Blocks[uiBlockIdx];

  if (uiBlock == 0ull)
    return false;

  const ezUInt32 uiBit = GetBitIndex(uiX % 4u, uiY % 4u, uiZ % 4u);
  const ezUInt64 uiMask = 1ull << uiBit;

  return (uiBlock & uiMask) != 0ull;
}

void ezVoxelGrid::SetVoxelsOnTriangle(const ezVec3& v0, const ezVec3& v1, const ezVec3& v2, bool bSet /*= true*/)
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
  ezVec3I32 coordMin = WorldToCoord(triMin);
  ezVec3I32 coordMax = WorldToCoord(triMax);

  coordMin.x = ezMath::Max(coordMin.x, 0);
  coordMin.y = ezMath::Max(coordMin.y, 0);
  coordMin.z = ezMath::Max(coordMin.z, 0);
  coordMax.x = ezMath::Min(coordMax.x, (ezInt32)GetDimensions().x - 1);
  coordMax.y = ezMath::Min(coordMax.y, (ezInt32)GetDimensions().y - 1);
  coordMax.z = ezMath::Min(coordMax.z, (ezInt32)GetDimensions().z - 1);

  if (coordMin.x > coordMax.x || coordMin.y > coordMax.y || coordMin.z > coordMax.z)
    return;

  const float fHalfSize = GetVoxelSize() * 0.5f;

  for (ezInt32 z = coordMin.z; z <= coordMax.z; ++z)
  {
    for (ezInt32 y = coordMin.y; y <= coordMax.y; ++y)
    {
      for (ezInt32 x = coordMin.x; x <= coordMax.x; ++x)
      {
        const ezVec3I32 vCoord(x, y, z);

        if (IsVoxelSet(vCoord) == bSet)
          continue;

        const ezVec3 voxelCenter = CoordToWorld(vCoord);

        if (TriangleBoxOverlap(v0, v1, v2, voxelCenter, fHalfSize))
        {
          SetVoxel(vCoord, bSet);
        }
      }
    }
  }
}

bool ezVoxelGrid::CheckLineOfSight(const ezVec3I32& vStart, const ezVec3I32& vGoal) const
{
  const ezInt32 dx = vGoal.x - vStart.x;
  const ezInt32 dy = vGoal.y - vStart.y;
  const ezInt32 dz = vGoal.z - vStart.z;

  const ezInt32 iSteps = ezMath::Max(ezMath::Abs(dx), ezMath::Max(ezMath::Abs(dy), ezMath::Abs(dz)));

  if (iSteps == 0)
    return true;

  const float fInvSteps = 1.0f / (float)iSteps;
  const float fXIncr = (float)dx * fInvSteps;
  const float fYIncr = (float)dy * fInvSteps;
  const float fZIncr = (float)dz * fInvSteps;

  float fX = (float)vStart.x;
  float fY = (float)vStart.y;
  float fZ = (float)vStart.z;

  for (ezInt32 i = 0; i < iSteps; ++i)
  {
    const ezVec3I32 vCoord(
      (ezInt32)ezMath::Round(fX),
      (ezInt32)ezMath::Round(fY),
      (ezInt32)ezMath::Round(fZ));

    if (vCoord.x == vGoal.x && vCoord.y == vGoal.y && vCoord.z == vGoal.z)
      return true;

    if (IsVoxelSet(vCoord))
      return false;

    fX += fXIncr;
    fY += fYIncr;
    fZ += fZIncr;
  }

  return true;
}

ezBoundingBox ezVoxelGrid::GetAABB() const
{
  const ezVec3 vHalfExtents(
    m_vDimensions.x * 0.5f * m_fVoxelSize,
    m_vDimensions.y * 0.5f * m_fVoxelSize,
    m_vDimensions.z * 0.5f * m_fVoxelSize);

  return ezBoundingBox::MakeFromCenterAndHalfExtents(m_vCenter, vHalfExtents);
}

ezUInt64 ezVoxelGrid::GetHeapMemoryUsage() const
{
  return m_Blocks.GetHeapMemoryUsage();
}

void ezVoxelGrid::DebugDraw(const ezDebugRendererContext& context, const ezColor& color) const
{
  if (m_Blocks.IsEmpty())
    return;

  // Shrink boxes slightly so individual voxels are visually distinguishable
  const float fHalf = m_fVoxelSize * 0.48f;
  const ezVec3 vHalfExtents(fHalf, fHalf, fHalf);

  for (ezUInt32 bz = 0; bz < m_vNumBlocks.z; ++bz)
  {
    for (ezUInt32 by = 0; by < m_vNumBlocks.y; ++by)
    {
      for (ezUInt32 bx = 0; bx < m_vNumBlocks.x; ++bx)
      {
        const ezUInt32 uiBlockIdx = GetBlockIndex(bx, by, bz);
        ezUInt64 uiBlock = m_Blocks[uiBlockIdx];

        if (uiBlock == 0ull)
          continue;

        while (uiBlock != 0ull)
        {
          const ezUInt32 uiBit = ezMath::FirstBitLow(uiBlock);
          uiBlock ^= (1ull << uiBit);

          const ezUInt32 lx = uiBit % 4u;
          const ezUInt32 ly = (uiBit / 4u) % 4u;
          const ezUInt32 lz = uiBit / 16u;

          const ezVec3I32 vGlobalCoord(
            (ezInt32)(bx * 4u + lx),
            (ezInt32)(by * 4u + ly),
            (ezInt32)(bz * 4u + lz));

          // Only draw surface voxels (at least one free neighbor)
          bool bIsSurface = false;
          const ezVec3I32 vNeighbors[] = {
            {vGlobalCoord.x - 1, vGlobalCoord.y, vGlobalCoord.z},
            {vGlobalCoord.x + 1, vGlobalCoord.y, vGlobalCoord.z},
            {vGlobalCoord.x, vGlobalCoord.y - 1, vGlobalCoord.z},
            {vGlobalCoord.x, vGlobalCoord.y + 1, vGlobalCoord.z},
            {vGlobalCoord.x, vGlobalCoord.y, vGlobalCoord.z - 1},
            {vGlobalCoord.x, vGlobalCoord.y, vGlobalCoord.z + 1},
          };

          for (const auto& vNeighbor : vNeighbors)
          {
            if (!IsCoordValid(vNeighbor) || !IsVoxelSet(vNeighbor))
            {
              bIsSurface = true;
              break;
            }
          }

          if (bIsSurface)
          {
            const ezVec3 vWorldPos = CoordToWorld(vGlobalCoord);
            const ezBoundingBox voxelBox = ezBoundingBox::MakeFromCenterAndHalfExtents(vWorldPos, vHalfExtents);
            ezDebugRenderer::DrawLineBox(context, voxelBox, color);
          }
        }
      }
    }
  }
}
