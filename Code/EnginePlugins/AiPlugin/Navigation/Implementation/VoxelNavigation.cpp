#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/VoxelGrid.h>
#include <AiPlugin/Navigation/VoxelNavigation.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezAiVoxelNavigation::ezAiVoxelNavigation() = default;
ezAiVoxelNavigation::~ezAiVoxelNavigation() = default;

void ezAiVoxelNavigation::SetVoxelGrid(const ezVoxelGrid* pGrid)
{
  m_pGrid = pGrid;
}

namespace
{
  struct AStarNode
  {
    EZ_DECLARE_POD_TYPE();

    float fGCost = ezMath::MaxValue<float>();
    float fFCost = ezMath::MaxValue<float>();
    ezUInt32 uiParent = ezInvalidIndex;
    bool bClosed = false;
  };

  EZ_FORCE_INLINE ezUInt32 PackCoord(const ezVec3I32& vCoord, ezUInt32 uiDimX, ezUInt32 uiDimY)
  {
    return (ezUInt32)vCoord.z * uiDimX * uiDimY + (ezUInt32)vCoord.y * uiDimX + (ezUInt32)vCoord.x;
  }

  EZ_FORCE_INLINE ezVec3I32 UnpackCoord(ezUInt32 uiIndex, ezUInt32 uiDimX, ezUInt32 uiDimY)
  {
    const ezInt32 z = (ezInt32)(uiIndex / (uiDimX * uiDimY));
    const ezUInt32 uiRemaining = uiIndex - (ezUInt32)z * uiDimX * uiDimY;
    const ezInt32 y = (ezInt32)(uiRemaining / uiDimX);
    const ezInt32 x = (ezInt32)(uiRemaining % uiDimX);
    return ezVec3I32(x, y, z);
  }

  float OctileHeuristic(const ezVec3I32& vA, const ezVec3I32& vB)
  {
    const float dx = (float)ezMath::Abs(vA.x - vB.x);
    const float dy = (float)ezMath::Abs(vA.y - vB.y);
    const float dz = (float)ezMath::Abs(vA.z - vB.z);

    // Sort so that d1 <= d2 <= d3
    float d1 = ezMath::Min(dx, ezMath::Min(dy, dz));
    float d3 = ezMath::Max(dx, ezMath::Max(dy, dz));
    float d2 = dx + dy + dz - d1 - d3;

    // d1 space-diagonal steps (cost sqrt(3)), then (d2 - d1) face-diagonal steps (sqrt(2)), then (d3 - d2) axis steps (1)
    static const float fSqrt2 = ezMath::Sqrt(2.0f);
    static const float fSqrt3 = ezMath::Sqrt(3.0f);

    return d1 * fSqrt3 + (d2 - d1) * fSqrt2 + (d3 - d2);
  }

  // Minimal binary min-heap for A* open set
  struct OpenSetEntry
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 uiIndex;
    float fFCost;
  };

  void HeapPush(ezDynamicArray<OpenSetEntry>& heap, ezUInt32 uiIndex, float fFCost)
  {
    OpenSetEntry entry;
    entry.uiIndex = uiIndex;
    entry.fFCost = fFCost;
    heap.PushBack(entry);

    // Sift up
    ezUInt32 i = heap.GetCount() - 1;
    while (i > 0)
    {
      const ezUInt32 uiParent = (i - 1) / 2;
      if (heap[uiParent].fFCost > heap[i].fFCost)
      {
        ezMath::Swap(heap[uiParent], heap[i]);
        i = uiParent;
      }
      else
      {
        break;
      }
    }
  }

  OpenSetEntry HeapPop(ezDynamicArray<OpenSetEntry>& heap)
  {
    OpenSetEntry top = heap[0];
    heap[0] = heap[heap.GetCount() - 1];
    heap.PopBack();

    // Sift down
    ezUInt32 i = 0;
    const ezUInt32 uiCount = heap.GetCount();
    while (true)
    {
      ezUInt32 uiSmallest = i;
      const ezUInt32 uiLeft = 2 * i + 1;
      const ezUInt32 uiRight = 2 * i + 2;

      if (uiLeft < uiCount && heap[uiLeft].fFCost < heap[uiSmallest].fFCost)
        uiSmallest = uiLeft;
      if (uiRight < uiCount && heap[uiRight].fFCost < heap[uiSmallest].fFCost)
        uiSmallest = uiRight;

      if (uiSmallest != i)
      {
        ezMath::Swap(heap[i], heap[uiSmallest]);
        i = uiSmallest;
      }
      else
      {
        break;
      }
    }

    return top;
  }
} // namespace

ezAiVoxelNavigation::State ezAiVoxelNavigation::FindPath(const ezVec3& vStart, const ezVec3& vTarget, ezUInt32 uiMaxIterations)
{
  m_Waypoints.Clear();
  m_uiCurrentWaypoint = 0;

  if (m_pGrid == nullptr || !m_pGrid->IsInitialized())
  {
    m_State = State::NoPathFound;
    return m_State;
  }

  ezVec3I32 vStartCoord, vTargetCoord;

  if (!m_pGrid->WorldToCoord(vStart, vStartCoord) || m_pGrid->CheckVoxel(vStartCoord))
  {
    m_State = State::InvalidStartPosition;
    return m_State;
  }

  if (!m_pGrid->WorldToCoord(vTarget, vTargetCoord) || m_pGrid->CheckVoxel(vTargetCoord))
  {
    m_State = State::InvalidTargetPosition;
    return m_State;
  }

  const ezUInt32 uiDimX = m_pGrid->GetDimX();
  const ezUInt32 uiDimY = m_pGrid->GetDimY();

  const ezUInt32 uiStartPacked = PackCoord(vStartCoord, uiDimX, uiDimY);
  const ezUInt32 uiTargetPacked = PackCoord(vTargetCoord, uiDimX, uiDimY);

  ezHashTable<ezUInt32, AStarNode> nodes;
  ezDynamicArray<OpenSetEntry> openSet;

  // Initialize start node
  {
    AStarNode startNode;
    startNode.fGCost = 0.0f;
    startNode.fFCost = OctileHeuristic(vStartCoord, vTargetCoord);
    startNode.uiParent = ezInvalidIndex;
    nodes.Insert(uiStartPacked, startNode);
    HeapPush(openSet, uiStartPacked, startNode.fFCost);
  }

  static const float fSqrt2 = ezMath::Sqrt(2.0f);
  static const float fSqrt3 = ezMath::Sqrt(3.0f);

  // 26-connected neighbor offsets and costs
  struct Neighbor
  {
    ezInt32 dx, dy, dz;
    float fCost;
  };

  Neighbor neighbors[26];
  ezUInt32 uiNeighborCount = 0;
  for (ezInt32 dz = -1; dz <= 1; ++dz)
  {
    for (ezInt32 dy = -1; dy <= 1; ++dy)
    {
      for (ezInt32 dx = -1; dx <= 1; ++dx)
      {
        if (dx == 0 && dy == 0 && dz == 0)
          continue;

        const ezInt32 iManhattan = ezMath::Abs(dx) + ezMath::Abs(dy) + ezMath::Abs(dz);
        float fCost;
        if (iManhattan == 1)
          fCost = 1.0f;
        else if (iManhattan == 2)
          fCost = fSqrt2;
        else
          fCost = fSqrt3;

        Neighbor& n = neighbors[uiNeighborCount++];
        n.dx = dx;
        n.dy = dy;
        n.dz = dz;
        n.fCost = fCost;
      }
    }
  }

  ezUInt32 uiIterations = 0;
  bool bFound = false;

  while (!openSet.IsEmpty() && uiIterations < uiMaxIterations)
  {
    ++uiIterations;

    const OpenSetEntry current = HeapPop(openSet);

    AStarNode* pCurrentNode = nullptr;
    nodes.TryGetValue(current.uiIndex, pCurrentNode);

    if (pCurrentNode == nullptr || pCurrentNode->bClosed)
      continue;

    pCurrentNode->bClosed = true;

    if (current.uiIndex == uiTargetPacked)
    {
      bFound = true;
      break;
    }

    const ezVec3I32 vCurrentCoord = UnpackCoord(current.uiIndex, uiDimX, uiDimY);
    const float fCurrentG = pCurrentNode->fGCost;

    for (ezUInt32 n = 0; n < uiNeighborCount; ++n)
    {
      const ezVec3I32 vNeighborCoord(
        vCurrentCoord.x + neighbors[n].dx,
        vCurrentCoord.y + neighbors[n].dy,
        vCurrentCoord.z + neighbors[n].dz);

      if (!m_pGrid->IsCoordValid(vNeighborCoord))
        continue;

      if (m_pGrid->CheckVoxel(vNeighborCoord))
        continue;

      const ezUInt32 uiNeighborPacked = PackCoord(vNeighborCoord, uiDimX, uiDimY);
      const float fTentativeG = fCurrentG + neighbors[n].fCost;

      AStarNode* pNeighborNode = nullptr;
      if (!nodes.TryGetValue(uiNeighborPacked, pNeighborNode))
      {
        AStarNode newNode;
        newNode.fGCost = fTentativeG;
        newNode.fFCost = fTentativeG + OctileHeuristic(vNeighborCoord, vTargetCoord);
        newNode.uiParent = current.uiIndex;
        nodes.Insert(uiNeighborPacked, newNode);
        HeapPush(openSet, uiNeighborPacked, newNode.fFCost);
      }
      else if (!pNeighborNode->bClosed && fTentativeG < pNeighborNode->fGCost)
      {
        pNeighborNode->fGCost = fTentativeG;
        pNeighborNode->fFCost = fTentativeG + OctileHeuristic(vNeighborCoord, vTargetCoord);
        pNeighborNode->uiParent = current.uiIndex;
        // Re-insert into open set (lazy deletion handles stale entries)
        HeapPush(openSet, uiNeighborPacked, pNeighborNode->fFCost);
      }
    }
  }

  if (!bFound)
  {
    m_State = State::NoPathFound;
    return m_State;
  }

  // Back-trace path
  ezDynamicArray<ezVec3> rawPath;
  ezUInt32 uiCurrent = uiTargetPacked;

  while (uiCurrent != ezInvalidIndex)
  {
    const ezVec3I32 vCoord = UnpackCoord(uiCurrent, uiDimX, uiDimY);
    rawPath.PushBack(m_pGrid->CoordToWorld(vCoord));

    AStarNode* pNode = nullptr;
    if (nodes.TryGetValue(uiCurrent, pNode))
    {
      uiCurrent = pNode->uiParent;
    }
    else
    {
      break;
    }
  }

  // Reverse to get start-to-target order
  for (ezUInt32 i = 0; i < rawPath.GetCount() / 2; ++i)
  {
    ezMath::Swap(rawPath[i], rawPath[rawPath.GetCount() - 1 - i]);
  }

  // Smooth the path using line-of-sight string-pulling
  SmoothPath(rawPath);

  m_Waypoints = std::move(rawPath);
  m_uiCurrentWaypoint = 0;
  m_State = State::PathFound;
  return m_State;
}

void ezAiVoxelNavigation::SmoothPath(ezDynamicArray<ezVec3>& inout_waypoints) const
{
  if (inout_waypoints.GetCount() <= 2 || m_pGrid == nullptr)
    return;

  ezDynamicArray<ezVec3> smoothed;
  smoothed.PushBack(inout_waypoints[0]);

  ezUInt32 uiCurrent = 0;

  while (uiCurrent < inout_waypoints.GetCount() - 1)
  {
    ezUInt32 uiFarthestVisible = uiCurrent + 1;

    for (ezUInt32 i = inout_waypoints.GetCount() - 1; i > uiCurrent + 1; --i)
    {
      if (m_pGrid->IsVisible(inout_waypoints[uiCurrent], inout_waypoints[i]))
      {
        uiFarthestVisible = i;
        break;
      }
    }

    smoothed.PushBack(inout_waypoints[uiFarthestVisible]);
    uiCurrent = uiFarthestVisible;
  }

  inout_waypoints = std::move(smoothed);
}

bool ezAiVoxelNavigation::AdvanceWaypoint()
{
  if (m_uiCurrentWaypoint + 1 < m_Waypoints.GetCount())
  {
    ++m_uiCurrentWaypoint;
    return true;
  }
  return false;
}

ezVec3 ezAiVoxelNavigation::GetNextWaypoint() const
{
  if (m_Waypoints.IsEmpty())
    return ezVec3::MakeZero();

  if (m_uiCurrentWaypoint < m_Waypoints.GetCount())
    return m_Waypoints[m_uiCurrentWaypoint];

  return m_Waypoints[m_Waypoints.GetCount() - 1];
}

bool ezAiVoxelNavigation::IsPathComplete() const
{
  return m_Waypoints.IsEmpty() || m_uiCurrentWaypoint >= m_Waypoints.GetCount();
}

void ezAiVoxelNavigation::CancelNavigation()
{
  m_Waypoints.Clear();
  m_uiCurrentWaypoint = 0;
  m_State = State::Idle;
}

void ezAiVoxelNavigation::DebugDrawPath(const ezDebugRendererContext& context, const ezColor& color) const
{
  if (m_Waypoints.GetCount() < 2)
    return;

  ezDynamicArray<ezDebugRendererLine> lines;
  lines.Reserve(m_Waypoints.GetCount() - 1);

  for (ezUInt32 i = 0; i + 1 < m_Waypoints.GetCount(); ++i)
  {
    auto& line = lines.ExpandAndGetRef();
    line.m_start = m_Waypoints[i];
    line.m_end = m_Waypoints[i + 1];

    if (i < m_uiCurrentWaypoint)
    {
      line.m_startColor = ezColor::Grey;
      line.m_endColor = ezColor::Grey;
    }
    else
    {
      line.m_startColor = color;
      line.m_endColor = color;
    }
  }

  ezDebugRenderer::DrawLines(context, lines, color);
}

EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Implementation_VoxelNavigation);
