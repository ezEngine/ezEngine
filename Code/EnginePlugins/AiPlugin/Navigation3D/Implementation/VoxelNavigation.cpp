#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <AiPlugin/Navigation3D/VoxelNavigation.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezAiVoxelNavigation::ezAiVoxelNavigation() = default;
ezAiVoxelNavigation::~ezAiVoxelNavigation() = default;

// How many voxels FindPathToTarget()/FindPathToExit() search around a blocked start coordinate
// before giving up. 2 voxels covers the common case of the immediate neighbor also being blocked
// (e.g. a tight corner), without letting recovery silently teleport the object very far.
static const ezUInt32 s_uiStartRecoveryRadiusVoxels = 2;

bool ezAiVoxelNavigation::FindNearbyValidCoord(const ezVoxelGrid& grid, const ezVec3I32& vCoord, ezUInt32 uiMaxRadius, ezVec3I32& out_vValidCoord)
{
  // Exhaustive search of the (2*uiMaxRadius+1)^3 cube around vCoord, keeping the closest valid,
  // non-solid coordinate found (squared voxel-offset distance is monotonic with physical distance
  // since a grid's voxels are uniform in size, so this doesn't need to convert to world space).
  // uiMaxRadius is expected to stay small (a handful of voxels) - this is meant for local recovery,
  // not long-range searches.
  const ezInt32 iMaxRadius = (ezInt32)uiMaxRadius;
  ezInt32 iBestDistSqr = ezMath::MaxValue<ezInt32>();
  bool bFound = false;

  for (ezInt32 dz = -iMaxRadius; dz <= iMaxRadius; ++dz)
  {
    for (ezInt32 dy = -iMaxRadius; dy <= iMaxRadius; ++dy)
    {
      for (ezInt32 dx = -iMaxRadius; dx <= iMaxRadius; ++dx)
      {
        if (dx == 0 && dy == 0 && dz == 0)
          continue;

        const ezInt32 iDistSqr = dx * dx + dy * dy + dz * dz;
        if (iDistSqr >= iBestDistSqr)
          continue;

        const ezVec3I32 vNeighbor = vCoord + ezVec3I32(dx, dy, dz);
        if (!grid.IsCoordValid(vNeighbor) || grid.IsVoxelSet(vNeighbor))
          continue;

        iBestDistSqr = iDistSqr;
        out_vValidCoord = vNeighbor;
        bFound = true;
      }
    }
  }

  return bFound;
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

  EZ_FORCE_INLINE bool IsBoundaryCoord(const ezVec3I32& vCoord, const ezVec3U32& vDims)
  {
    return vCoord.x == 0 || vCoord.y == 0 || vCoord.z == 0 ||
           (ezUInt32)vCoord.x == vDims.x - 1 || (ezUInt32)vCoord.y == vDims.y - 1 || (ezUInt32)vCoord.z == vDims.z - 1;
  }

  /// Returns the (unnormalized) outward-facing normal of the grid boundary at vCoord, i.e. the sum
  /// of the outward unit vectors of every face vCoord touches (nonzero only where IsBoundaryCoord is true).
  EZ_FORCE_INLINE ezVec3 GetBoundaryOutwardNormal(const ezVec3I32& vCoord, const ezVec3U32& vDims)
  {
    ezVec3 vNormal = ezVec3::MakeZero();

    if (vCoord.x == 0)
      vNormal.x -= 1.0f;
    if ((ezUInt32)vCoord.x == vDims.x - 1)
      vNormal.x += 1.0f;

    if (vCoord.y == 0)
      vNormal.y -= 1.0f;
    if ((ezUInt32)vCoord.y == vDims.y - 1)
      vNormal.y += 1.0f;

    if (vCoord.z == 0)
      vNormal.z -= 1.0f;
    if ((ezUInt32)vCoord.z == vDims.z - 1)
      vNormal.z += 1.0f;

    return vNormal;
  }

  /// Whether exiting the grid at vCoord (which must satisfy IsBoundaryCoord) actually leads away
  /// from the grid, towards vTargetCoord - i.e. leaving through this particular voxel doesn't just
  /// walk straight back into the same grid. vTargetCoord may lie outside the grid.
  bool IsUsableExit(const ezVec3I32& vCoord, const ezVec3U32& vDims, const ezVec3I32& vTargetCoord)
  {
    const ezVec3 vNormal = GetBoundaryOutwardNormal(vCoord, vDims);

    const ezVec3 vToTarget(
      (float)(vTargetCoord.x - vCoord.x),
      (float)(vTargetCoord.y - vCoord.y),
      (float)(vTargetCoord.z - vCoord.z));

    if (vToTarget.IsZero(0.001f))
      return true; // no clear direction to check against, don't reject

    return vNormal.Dot(vToTarget) > 0.0f;
  }

  /// Grids are not supposed to overlap, but in case they do, reject exit voxels that fall inside a
  /// solid voxel of another grid - that world position wouldn't actually be reachable.
  bool IsCoordFreeInOtherGrids(const ezVoxelGrid& grid, const ezVec3I32& vCoord, ezArrayPtr<const ezVoxelGrid* const> otherGrids)
  {
    if (otherGrids.IsEmpty())
      return true;

    const ezVec3 vWorldPos = grid.CoordToWorld(vCoord);

    for (const ezVoxelGrid* pOther : otherGrids)
    {
      if (pOther == &grid)
        continue;

      if (!pOther->GetAABB().Contains(vWorldPos))
        continue;

      const ezVec3I32 vOtherCoord = pOther->WorldToCoord(vWorldPos);
      if (pOther->IsCoordValid(vOtherCoord) && pOther->IsVoxelSet(vOtherCoord))
        return false;
    }

    return true;
  }

  // Admissible heuristic for 6-connected (face-neighbor only) movement: the minimum number of
  // axis-aligned unit steps needed, which is exactly the Manhattan distance.
  float ManhattanHeuristic(const ezVec3I32& vA, const ezVec3I32& vB)
  {
    return (float)(ezMath::Abs(vA.x - vB.x) + ezMath::Abs(vA.y - vB.y) + ezMath::Abs(vA.z - vB.z));
  }

  // Minimal binary min-heap for A* open set
  struct OpenSetEntry
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 uiIndex;
    float fFCost;
  };

  void HeapPush(ezDynamicArray<OpenSetEntry>& ref_heap, ezUInt32 uiIndex, float fFCost)
  {
    OpenSetEntry entry;
    entry.uiIndex = uiIndex;
    entry.fFCost = fFCost;
    ref_heap.PushBack(entry);

    // Sift up
    ezUInt32 i = ref_heap.GetCount() - 1;
    while (i > 0)
    {
      const ezUInt32 uiParent = (i - 1) / 2;
      if (ref_heap[uiParent].fFCost > ref_heap[i].fFCost)
      {
        ezMath::Swap(ref_heap[uiParent], ref_heap[i]);
        i = uiParent;
      }
      else
      {
        break;
      }
    }
  }

  OpenSetEntry HeapPop(ezDynamicArray<OpenSetEntry>& ref_heap)
  {
    OpenSetEntry top = ref_heap[0];
    ref_heap[0] = ref_heap[ref_heap.GetCount() - 1];
    ref_heap.PopBack();

    // Sift down
    ezUInt32 i = 0;
    const ezUInt32 uiCount = ref_heap.GetCount();
    while (true)
    {
      ezUInt32 uiSmallest = i;
      const ezUInt32 uiLeft = 2 * i + 1;
      const ezUInt32 uiRight = 2 * i + 2;

      if (uiLeft < uiCount && ref_heap[uiLeft].fFCost < ref_heap[uiSmallest].fFCost)
        uiSmallest = uiLeft;
      if (uiRight < uiCount && ref_heap[uiRight].fFCost < ref_heap[uiSmallest].fFCost)
        uiSmallest = uiRight;

      if (uiSmallest != i)
      {
        ezMath::Swap(ref_heap[i], ref_heap[uiSmallest]);
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

// Shared A* core, used both to search for an exact target coordinate inside a grid, and to search
// for a way out of a grid (any voxel on its boundary), biased towards vTargetCoord.
//
// vStartCoord must be a valid, non-solid coordinate in grid. If bExitSearch is false, vTargetCoord
// must also be a valid, non-solid coordinate in grid and is searched for exactly. If bExitSearch is
// true, vTargetCoord is only used to bias the search heuristic and may lie outside grid; the search
// stops at the first voxel reached that lies on the boundary of grid.
static ezAiVoxelNavigation::State RunGridAStar(const ezVoxelGrid& grid, const ezVec3I32& vStartCoord, const ezVec3I32& vTargetCoord,
  bool bExitSearch, ezArrayPtr<const ezVoxelGrid* const> otherGrids, ezUInt32 uiMaxIterations, ezDynamicArray<ezVec3>& out_waypoints)
{
  using State = ezAiVoxelNavigation::State;

  const ezUInt32 uiDimX = grid.GetDimensions().x;
  const ezUInt32 uiDimY = grid.GetDimensions().y;
  const ezVec3U32 vDims = grid.GetDimensions();

  const ezUInt32 uiStartPacked = PackCoord(vStartCoord, uiDimX, uiDimY);
  const ezUInt32 uiTargetPacked = PackCoord(vTargetCoord, uiDimX, uiDimY);

  ezHashTable<ezUInt32, AStarNode> nodes;
  ezDynamicArray<OpenSetEntry> openSet;

  // Initialize start node
  {
    AStarNode startNode;
    startNode.fGCost = 0.0f;
    startNode.fFCost = ManhattanHeuristic(vStartCoord, vTargetCoord);
    startNode.uiParent = ezInvalidIndex;
    nodes.Insert(uiStartPacked, startNode);
    HeapPush(openSet, uiStartPacked, startNode.fFCost);
  }

  // 6-connected (face) neighbor offsets. Diagonal (26-connected) movement was removed: it lets a
  // path cut across a corner between two solid voxels that only touch edge-to-edge or corner-to-
  // corner, squeezing through a gap that isn't actually open.
  struct Neighbor
  {
    ezInt32 dx, dy, dz;
  };

  static const Neighbor neighbors[6] = {
    {-1, 0, 0},
    {1, 0, 0},
    {0, -1, 0},
    {0, 1, 0},
    {0, 0, -1},
    {0, 0, 1},
  };
  const ezUInt32 uiNeighborCount = 6;

  ezUInt32 uiIterations = 0;
  ezUInt32 uiGoalPacked = ezInvalidIndex;

  while (!openSet.IsEmpty() && uiIterations < uiMaxIterations)
  {
    ++uiIterations;

    const OpenSetEntry current = HeapPop(openSet);

    AStarNode* pCurrentNode = nullptr;
    nodes.TryGetValue(current.uiIndex, pCurrentNode);

    if (pCurrentNode == nullptr || pCurrentNode->bClosed)
      continue;

    pCurrentNode->bClosed = true;

    const ezVec3I32 vCurrentCoord = UnpackCoord(current.uiIndex, uiDimX, uiDimY);

    const bool bIsGoal = bExitSearch
                           ? (IsBoundaryCoord(vCurrentCoord, vDims) && IsUsableExit(vCurrentCoord, vDims, vTargetCoord) &&
                               IsCoordFreeInOtherGrids(grid, vCurrentCoord, otherGrids))
                           : current.uiIndex == uiTargetPacked;

    if (bIsGoal)
    {
      uiGoalPacked = current.uiIndex;
      break;
    }

    const float fCurrentG = pCurrentNode->fGCost;

    for (ezUInt32 n = 0; n < uiNeighborCount; ++n)
    {
      const ezVec3I32 vNeighborCoord(
        vCurrentCoord.x + neighbors[n].dx,
        vCurrentCoord.y + neighbors[n].dy,
        vCurrentCoord.z + neighbors[n].dz);

      if (!grid.IsCoordValid(vNeighborCoord))
        continue;

      if (grid.IsVoxelSet(vNeighborCoord))
        continue;

      const ezUInt32 uiNeighborPacked = PackCoord(vNeighborCoord, uiDimX, uiDimY);
      const float fTentativeG = fCurrentG + 1.0f;

      AStarNode* pNeighborNode = nullptr;
      if (!nodes.TryGetValue(uiNeighborPacked, pNeighborNode))
      {
        AStarNode newNode;
        newNode.fGCost = fTentativeG;
        newNode.fFCost = fTentativeG + ManhattanHeuristic(vNeighborCoord, vTargetCoord);
        newNode.uiParent = current.uiIndex;
        nodes.Insert(uiNeighborPacked, newNode);
        HeapPush(openSet, uiNeighborPacked, newNode.fFCost);
      }
      else if (!pNeighborNode->bClosed && fTentativeG < pNeighborNode->fGCost)
      {
        pNeighborNode->fGCost = fTentativeG;
        pNeighborNode->fFCost = fTentativeG + ManhattanHeuristic(vNeighborCoord, vTargetCoord);
        pNeighborNode->uiParent = current.uiIndex;
        // Re-insert into open set (lazy deletion handles stale entries)
        HeapPush(openSet, uiNeighborPacked, pNeighborNode->fFCost);
      }
    }
  }

  if (uiGoalPacked == ezInvalidIndex)
    return State::NoPathFound;

  // Back-trace path
  ezDynamicArray<ezVec3> rawPath;
  ezUInt32 uiCurrent = uiGoalPacked;

  while (uiCurrent != ezInvalidIndex)
  {
    const ezVec3I32 vCoord = UnpackCoord(uiCurrent, uiDimX, uiDimY);
    rawPath.PushBack(grid.CoordToWorld(vCoord));

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

  out_waypoints = std::move(rawPath);
  return State::PathFound;
}

ezAiVoxelNavigation::State ezAiVoxelNavigation::FindPathToTarget(const ezVoxelGrid& grid, const ezVec3& vStart, const ezVec3& vTarget,
  ezUInt32 uiMaxIterations, ezDynamicArray<ezVec3>& out_waypoints) const
{
  out_waypoints.Clear();

  ezVec3I32 vStartCoord = grid.WorldToCoord(vStart);
  const ezVec3I32 vTargetCoord = grid.WorldToCoord(vTarget);

  if (!grid.IsCoordValid(vStartCoord) || grid.IsVoxelSet(vStartCoord))
  {
    // The exact start voxel is blocked or out of bounds - before giving up, try to recover by
    // stepping into a free voxel immediately next to it (e.g. the object got pushed into a voxel
    // that turned solid after it last pathed).
    ezVec3I32 vRecoveredCoord;
    if (!FindNearbyValidCoord(grid, vStartCoord, s_uiStartRecoveryRadiusVoxels, vRecoveredCoord))
      return State::InvalidStartPosition;

    vStartCoord = vRecoveredCoord;
  }

  if (!grid.IsCoordValid(vTargetCoord) || grid.IsVoxelSet(vTargetCoord))
    return State::InvalidTargetPosition;

  const State result = RunGridAStar(grid, vStartCoord, vTargetCoord, false, ezArrayPtr<const ezVoxelGrid* const>(), uiMaxIterations, out_waypoints);

  if (result == State::PathFound)
  {
    SmoothPath(grid, out_waypoints);
  }

  return result;
}

ezAiVoxelNavigation::State ezAiVoxelNavigation::FindPathToExit(const ezVoxelGrid& grid, const ezVec3& vStart, const ezVec3& vRealTarget,
  ezArrayPtr<const ezVoxelGrid* const> otherGrids, ezUInt32 uiMaxIterations, ezDynamicArray<ezVec3>& out_waypoints) const
{
  out_waypoints.Clear();

  ezVec3I32 vStartCoord = grid.WorldToCoord(vStart);
  const ezVec3I32 vTargetCoord = grid.WorldToCoord(vRealTarget); // may lie outside grid, only used for the heuristic

  if (!grid.IsCoordValid(vStartCoord) || grid.IsVoxelSet(vStartCoord))
  {
    ezVec3I32 vRecoveredCoord;
    if (!FindNearbyValidCoord(grid, vStartCoord, s_uiStartRecoveryRadiusVoxels, vRecoveredCoord))
      return State::InvalidStartPosition;

    vStartCoord = vRecoveredCoord;
  }

  const State result = RunGridAStar(grid, vStartCoord, vTargetCoord, true, otherGrids, uiMaxIterations, out_waypoints);

  if (result == State::PathFound)
  {
    SmoothPath(grid, out_waypoints);
  }

  return result;
}

ezAiVoxelNavigation::State ezAiVoxelNavigation::FindPath(const ezVec3& vStart, const ezVec3& vTarget, const ezAiVoxelGridFinder& gridFinder,
  float fSearchMargin, ezUInt32 uiMaxIterationsPerHop, ezUInt32 uiMaxHops)
{
  m_Waypoints.Clear();
  m_SegmentInsideGrid.Clear();
  m_uiCurrentWaypoint = 0;
  m_Waypoints.PushBack(vStart);

  ezVec3 vCurrent = vStart;

  for (ezUInt32 uiHop = 0; uiHop < uiMaxHops; ++uiHop)
  {
    if ((vTarget - vCurrent).GetLengthSquared() < 0.0001f)
    {
      m_State = State::PathFound;
      return m_State;
    }

    ezBoundingBox searchBox = ezBoundingBox::MakeInvalid();
    searchBox.ExpandToInclude(vCurrent);
    searchBox.ExpandToInclude(vTarget);
    searchBox.Grow(ezVec3(fSearchMargin));

    ezDynamicArray<const ezVoxelGrid*> grids;
    gridFinder(searchBox, grids);

    const ezVoxelGrid* pGrid = nullptr;
    ezVec3 vEntry = vCurrent;
    float fBestEnter = ezMath::HighValue<float>();

    for (const ezVoxelGrid* pCandidate : grids)
    {
      const ezBoundingBox box = pCandidate->GetAABB();

      if (box.Contains(vCurrent))
      {
        pGrid = pCandidate;
        vEntry = vCurrent;
        break;
      }

      float fEnter;
      if (box.GetLineSegmentIntersection(vCurrent, vTarget, &fEnter) && fEnter > 0.0001f && fEnter < fBestEnter)
      {
        fBestEnter = fEnter;
        pGrid = pCandidate;

        // The intersection point lies exactly on the grid's boundary surface. Nudge it a tiny bit
        // further along the segment, into the grid's interior: a point exactly on the max-corner
        // face would otherwise floor to a voxel coordinate one past the last valid index (an
        // off-by-one at the boundary), making the grid falsely look unreachable.
        const ezVec3 vTravelDir = (vTarget - vCurrent).GetNormalized();
        vEntry = ezMath::Lerp(vCurrent, vTarget, fEnter) + vTravelDir * (pCandidate->GetVoxelSize() * 0.1f);
      }
    }

    if (pGrid == nullptr)
    {
      // Rest of the way is free space (not covered by any grid)
      m_Waypoints.PushBack(vTarget);
      m_SegmentInsideGrid.PushBack(false);
      m_State = State::PathFound;
      return m_State;
    }

    if (!vEntry.IsEqual(vCurrent, 0.0001f))
    {
      m_Waypoints.PushBack(vEntry);
      m_SegmentInsideGrid.PushBack(false);
    }

    const bool bTargetInside = pGrid->GetAABB().Contains(vTarget);

    ezDynamicArray<ezVec3> hopWaypoints;
    const State hopState = bTargetInside
                             ? FindPathToTarget(*pGrid, vEntry, vTarget, uiMaxIterationsPerHop, hopWaypoints)
                             : FindPathToExit(*pGrid, vEntry, vTarget, grids, uiMaxIterationsPerHop, hopWaypoints);

    if (hopState != State::PathFound)
    {
      m_State = hopState;
      return m_State;
    }

    for (ezUInt32 i = 0; i < hopWaypoints.GetCount(); ++i)
    {
      if (i == 0 && !m_Waypoints.IsEmpty() && hopWaypoints[i].IsEqual(m_Waypoints.PeekBack(), 0.0001f))
        continue;

      m_Waypoints.PushBack(hopWaypoints[i]);
      m_SegmentInsideGrid.PushBack(true);
    }

    vCurrent = m_Waypoints.PeekBack();

    if (bTargetInside)
    {
      m_State = State::PathFound;
      return m_State;
    }

    // vCurrent is the center of the boundary voxel the exit search stopped at, which is only up to
    // half a voxel away from the grid's true edge - not actually outside the grid yet. Push it out
    // along the boundary face normal of that voxel (not the direction from the grid's center through
    // the exit point - for non-cubic grids that vector is dominated by whichever axis the exit point
    // is farthest from center on, which usually isn't the axis of the face that was actually crossed).
    // A full voxel step guarantees clearing the boundary even when exiting through a corner or edge,
    // where the face normal is not axis-aligned.
    const ezVec3I32 vExitCoord = pGrid->WorldToCoord(vCurrent);
    const ezVec3 vOutward = GetBoundaryOutwardNormal(vExitCoord, pGrid->GetDimensions());
    if (!vOutward.IsZero(0.0001f))
    {
      vCurrent += vOutward.GetNormalized() * pGrid->GetVoxelSize();
    }
  }

  m_State = State::NoPathFound;
  return m_State;
}

void ezAiVoxelNavigation::SmoothPath(const ezVoxelGrid& grid, ezDynamicArray<ezVec3>& inout_waypoints) const
{
  if (inout_waypoints.GetCount() <= 2)
    return;

  ezDynamicArray<ezVec3> smoothed;
  smoothed.PushBack(inout_waypoints[0]);

  ezUInt32 uiCurrent = 0;

  while (uiCurrent < inout_waypoints.GetCount() - 1)
  {
    ezUInt32 uiFarthestVisible = uiCurrent + 1;

    for (ezUInt32 i = inout_waypoints.GetCount() - 1; i > uiCurrent + 1; --i)
    {
      ezVec3I32 vCoordA = grid.WorldToCoord(inout_waypoints[uiCurrent]);
      ezVec3I32 vCoordB = grid.WorldToCoord(inout_waypoints[i]);

      if (grid.IsCoordValid(vCoordA) &&
          grid.IsCoordValid(vCoordB) &&
          grid.CheckLineOfSight(vCoordA, vCoordB))
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

ezVec3 ezAiVoxelNavigation::WalkPathForward(const ezVec3& vCurrentPos, float fDistance, ezUInt32& inout_uiIndex) const
{
  ezVec3 vFrom = vCurrentPos;
  ezVec3 vTo = m_Waypoints[inout_uiIndex];
  float fRemaining = ezMath::Max(fDistance, 0.0f);

  while (true)
  {
    const ezVec3 vSeg = vTo - vFrom;
    const float fSegLen = vSeg.GetLength();

    if (fSegLen > 0.0001f)
    {
      if (fSegLen >= fRemaining)
        return vFrom + vSeg * (fRemaining / fSegLen);

      fRemaining -= fSegLen;
    }

    if (inout_uiIndex + 1 >= m_Waypoints.GetCount())
      return vTo;

    vFrom = vTo;
    ++inout_uiIndex;
    vTo = m_Waypoints[inout_uiIndex];
  }
}

ezVec3 ezAiVoxelNavigation::GetLookAheadPoint(const ezVec3& vCurrentPos, float fLookAheadDistance) const
{
  if (IsPathComplete())
    return vCurrentPos;

  ezUInt32 uiIndex = m_uiCurrentWaypoint; // local copy - peek only, does not advance the path
  return WalkPathForward(vCurrentPos, fLookAheadDistance, uiIndex);
}

ezVec3 ezAiVoxelNavigation::AdvanceAlongPath(const ezVec3& vCurrentPos, float fDistance)
{
  if (IsPathComplete())
    return vCurrentPos;

  return WalkPathForward(vCurrentPos, fDistance, m_uiCurrentWaypoint); // mutates the current waypoint index directly
}

void ezAiVoxelNavigation::SetDirectPath(const ezVec3& vStart, const ezVec3& vTarget)
{
  m_Waypoints.Clear();
  m_SegmentInsideGrid.Clear();
  m_uiCurrentWaypoint = 0;

  m_Waypoints.PushBack(vStart);
  m_Waypoints.PushBack(vTarget);
  m_SegmentInsideGrid.PushBack(false);

  m_State = State::PathFound;
}

void ezAiVoxelNavigation::CancelNavigation()
{
  m_Waypoints.Clear();
  m_SegmentInsideGrid.Clear();
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

void ezAiVoxelNavigation::DebugDrawPathSegments(const ezDebugRendererContext& context, const ezColor& insideGridColor,
  const ezColor& freeSpaceColor) const
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

    const ezColor& color = m_SegmentInsideGrid[i] ? insideGridColor : freeSpaceColor;
    line.m_startColor = color;
    line.m_endColor = color;
  }

  ezDebugRenderer::DrawLines(context, lines, ezColor::White);
}
