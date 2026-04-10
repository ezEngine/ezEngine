#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/VoxelGrid.h>
#include <Foundation/Math/Math.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezVoxelGrid::ezVoxelGrid() = default;
ezVoxelGrid::~ezVoxelGrid() = default;

void ezVoxelGrid::Init(ezUInt32 uiDimX, ezUInt32 uiDimY, ezUInt32 uiDimZ)
{
  m_uiDimX = ezMath::Max(4u, uiDimX);
  m_uiDimY = ezMath::Max(4u, uiDimY);
  m_uiDimZ = ezMath::Max(4u, uiDimZ);

  m_uiBlocksX = (m_uiDimX + 3u) / 4u;
  m_uiBlocksY = (m_uiDimY + 3u) / 4u;
  m_uiBlocksZ = (m_uiDimZ + 3u) / 4u;

  // Round dims up to block boundaries
  m_uiDimX = m_uiBlocksX * 4u;
  m_uiDimY = m_uiBlocksY * 4u;
  m_uiDimZ = m_uiBlocksZ * 4u;

  m_Blocks.Clear();
  m_Blocks.SetCount(m_uiBlocksX * m_uiBlocksY * m_uiBlocksZ, 0ull);
}

void ezVoxelGrid::ClearData()
{
  for (ezUInt32 i = 0; i < m_Blocks.GetCount(); ++i)
  {
    m_Blocks[i] = 0ull;
  }
}

void ezVoxelGrid::SetWorldParameters(const ezVec3& vCenter, float fVoxelSize)
{
  m_vCenter = vCenter;
  m_fVoxelSize = fVoxelSize;
  m_fInvVoxelSize = 1.0f / fVoxelSize;

  const ezVec3 vHalfExtents(
    m_uiDimX * 0.5f * m_fVoxelSize,
    m_uiDimY * 0.5f * m_fVoxelSize,
    m_uiDimZ * 0.5f * m_fVoxelSize);

  m_vOrigin = m_vCenter - vHalfExtents;
}

bool ezVoxelGrid::WorldToCoord(const ezVec3& vWorldPos, ezVec3I32& out_vCoord) const
{
  const ezVec3 vLocal = (vWorldPos - m_vOrigin) * m_fInvVoxelSize;

  out_vCoord.x = (ezInt32)ezMath::Floor(vLocal.x);
  out_vCoord.y = (ezInt32)ezMath::Floor(vLocal.y);
  out_vCoord.z = (ezInt32)ezMath::Floor(vLocal.z);

  return IsCoordValid(out_vCoord);
}

ezVec3 ezVoxelGrid::CoordToWorld(const ezVec3I32& vCoord) const
{
  return m_vOrigin + ezVec3(
                       (vCoord.x + 0.5f) * m_fVoxelSize,
                       (vCoord.y + 0.5f) * m_fVoxelSize,
                       (vCoord.z + 0.5f) * m_fVoxelSize);
}

bool ezVoxelGrid::IsCoordValid(const ezVec3I32& vCoord) const
{
  return vCoord.x >= 0 && vCoord.y >= 0 && vCoord.z >= 0 &&
         (ezUInt32)vCoord.x < m_uiDimX &&
         (ezUInt32)vCoord.y < m_uiDimY &&
         (ezUInt32)vCoord.z < m_uiDimZ;
}

ezUInt32 ezVoxelGrid::GetBlockIndex(ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 uiBlockZ) const
{
  return uiBlockZ * (m_uiBlocksX * m_uiBlocksY) + uiBlockY * m_uiBlocksX + uiBlockX;
}

ezUInt32 ezVoxelGrid::GetBitIndex(ezUInt32 uiLocalX, ezUInt32 uiLocalY, ezUInt32 uiLocalZ)
{
  return uiLocalZ * 16u + uiLocalY * 4u + uiLocalX;
}

void ezVoxelGrid::SetVoxel(const ezVec3I32& vCoord, bool bSolid)
{
  if (!IsCoordValid(vCoord))
    return;

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

bool ezVoxelGrid::CheckVoxel(const ezVec3I32& vCoord) const
{
  if (!IsCoordValid(vCoord))
    return false;

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

void ezVoxelGrid::InjectBox(const ezBoundingBox& box)
{
  ezVec3I32 vMin, vMax;
  WorldToCoord(box.m_vMin, vMin);
  WorldToCoord(box.m_vMax, vMax);

  // Clamp to valid range
  vMin.x = ezMath::Max(vMin.x, 0);
  vMin.y = ezMath::Max(vMin.y, 0);
  vMin.z = ezMath::Max(vMin.z, 0);
  vMax.x = ezMath::Min(vMax.x, (ezInt32)m_uiDimX - 1);
  vMax.y = ezMath::Min(vMax.y, (ezInt32)m_uiDimY - 1);
  vMax.z = ezMath::Min(vMax.z, (ezInt32)m_uiDimZ - 1);

  for (ezInt32 z = vMin.z; z <= vMax.z; ++z)
  {
    for (ezInt32 y = vMin.y; y <= vMax.y; ++y)
    {
      for (ezInt32 x = vMin.x; x <= vMax.x; ++x)
      {
        SetVoxel(ezVec3I32(x, y, z), true);
      }
    }
  }
}

void ezVoxelGrid::InjectSphere(const ezVec3& vCenter, float fRadius)
{
  const ezBoundingBox aabb = ezBoundingBox::MakeFromCenterAndHalfExtents(vCenter, ezVec3(fRadius));

  ezVec3I32 vMin, vMax;
  WorldToCoord(aabb.m_vMin, vMin);
  WorldToCoord(aabb.m_vMax, vMax);

  vMin.x = ezMath::Max(vMin.x, 0);
  vMin.y = ezMath::Max(vMin.y, 0);
  vMin.z = ezMath::Max(vMin.z, 0);
  vMax.x = ezMath::Min(vMax.x, (ezInt32)m_uiDimX - 1);
  vMax.y = ezMath::Min(vMax.y, (ezInt32)m_uiDimY - 1);
  vMax.z = ezMath::Min(vMax.z, (ezInt32)m_uiDimZ - 1);

  const float fRadiusSqr = fRadius * fRadius;

  for (ezInt32 z = vMin.z; z <= vMax.z; ++z)
  {
    for (ezInt32 y = vMin.y; y <= vMax.y; ++y)
    {
      for (ezInt32 x = vMin.x; x <= vMax.x; ++x)
      {
        const ezVec3 vVoxelCenter = CoordToWorld(ezVec3I32(x, y, z));
        const float fDistSqr = (vVoxelCenter - vCenter).GetLengthSquared();

        if (fDistSqr <= fRadiusSqr)
        {
          SetVoxel(ezVec3I32(x, y, z), true);
        }
      }
    }
  }
}

void ezVoxelGrid::SubtractBox(const ezBoundingBox& box)
{
  ezVec3I32 vMin, vMax;
  WorldToCoord(box.m_vMin, vMin);
  WorldToCoord(box.m_vMax, vMax);

  vMin.x = ezMath::Max(vMin.x, 0);
  vMin.y = ezMath::Max(vMin.y, 0);
  vMin.z = ezMath::Max(vMin.z, 0);
  vMax.x = ezMath::Min(vMax.x, (ezInt32)m_uiDimX - 1);
  vMax.y = ezMath::Min(vMax.y, (ezInt32)m_uiDimY - 1);
  vMax.z = ezMath::Min(vMax.z, (ezInt32)m_uiDimZ - 1);

  for (ezInt32 z = vMin.z; z <= vMax.z; ++z)
  {
    for (ezInt32 y = vMin.y; y <= vMax.y; ++y)
    {
      for (ezInt32 x = vMin.x; x <= vMax.x; ++x)
      {
        SetVoxel(ezVec3I32(x, y, z), false);
      }
    }
  }
}

bool ezVoxelGrid::IsVisibleCoord(const ezVec3I32& vStart, const ezVec3I32& vGoal) const
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

    if (CheckVoxel(vCoord))
      return false;

    fX += fXIncr;
    fY += fYIncr;
    fZ += fZIncr;
  }

  return true;
}

bool ezVoxelGrid::IsVisible(const ezVec3& vObserver, const ezVec3& vSubject) const
{
  ezVec3I32 vStart, vGoal;

  if (!WorldToCoord(vObserver, vStart))
    return false;

  if (!WorldToCoord(vSubject, vGoal))
    return false;

  return IsVisibleCoord(vStart, vGoal);
}

ezBoundingBox ezVoxelGrid::GetAABB() const
{
  const ezVec3 vHalfExtents(
    m_uiDimX * 0.5f * m_fVoxelSize,
    m_uiDimY * 0.5f * m_fVoxelSize,
    m_uiDimZ * 0.5f * m_fVoxelSize);

  return ezBoundingBox::MakeFromCenterAndHalfExtents(m_vCenter, vHalfExtents);
}

ezUInt64 ezVoxelGrid::GetMemoryUsage() const
{
  return m_Blocks.GetCount() * sizeof(ezUInt64);
}

void ezVoxelGrid::DebugDraw(const ezDebugRendererContext& context, const ezColor& color) const
{
  if (m_Blocks.IsEmpty())
    return;

  // Shrink boxes slightly so individual voxels are visually distinguishable
  const float fHalf = m_fVoxelSize * 0.45f;
  const ezVec3 vHalfExtents(fHalf, fHalf, fHalf);

  for (ezUInt32 bz = 0; bz < m_uiBlocksZ; ++bz)
  {
    for (ezUInt32 by = 0; by < m_uiBlocksY; ++by)
    {
      for (ezUInt32 bx = 0; bx < m_uiBlocksX; ++bx)
      {
        const ezUInt32 uiBlockIdx = GetBlockIndex(bx, by, bz);
        ezUInt64 uiBlock = m_Blocks[uiBlockIdx];

        if (uiBlock == 0ull)
          continue;

        while (uiBlock != 0ull)
        {
          // Find lowest set bit
          ezUInt32 uiBit = 0;
          ezUInt64 uiTemp = uiBlock;
          while ((uiTemp & 1ull) == 0ull)
          {
            uiTemp >>= 1;
            ++uiBit;
          }
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
            if (!IsCoordValid(vNeighbor) || !CheckVoxel(vNeighbor))
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

EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Implementation_VoxelGrid);
