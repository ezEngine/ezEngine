#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/SimdMath/SimdConversion.h>

namespace
{
  enum
  {
    MAX_CELL_INDEX = (1 << 20) - 1,
    CELL_INDEX_MASK = (1 << 21) - 1
  };

  EZ_ALWAYS_INLINE ezSimdVec4f ToVec3(const ezSimdVec4i& v) { return v.ToFloat(); }

  EZ_ALWAYS_INLINE ezSimdVec4i ToVec3I32(const ezSimdVec4f& v)
  {
    ezSimdVec4f vf = v.Floor();
    return ezSimdVec4i::Truncate(vf);
  }

  EZ_ALWAYS_INLINE ezUInt64 GetCellKey(ezInt32 x, ezInt32 y, ezInt32 z)
  {
    ezUInt64 sx = (x + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    ezUInt64 sy = (y + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    ezUInt64 sz = (z + MAX_CELL_INDEX) & CELL_INDEX_MASK;

    return (sx << 42) | (sy << 21) | sz;
  }

  EZ_ALWAYS_INLINE ezSimdBBox ComputeCellBoundingBox(const ezSimdVec4i& cellIndex, const ezSimdVec4i& iCellSize)
  {
    ezSimdVec4i overlapSize = iCellSize >> 2;
    ezSimdVec4i minPos = cellIndex.CompMul(iCellSize);

    ezSimdVec4f bmin = ToVec3(minPos - overlapSize);
    ezSimdVec4f bmax = ToVec3(minPos + overlapSize + iCellSize);

    return ezSimdBBox(bmin, bmax);
  }

  EZ_ALWAYS_INLINE bool FilterByCategory(ezUInt32 uiCategoryBitmask, ezUInt32 uiQueryBitmask)
  {
    return (uiCategoryBitmask & uiQueryBitmask) == 0;
  }

  EZ_ALWAYS_INLINE bool FilterByTags(const ezTagSet& tags, const ezTagSet& includeTags, const ezTagSet& excludeTags)
  {
    if (!excludeTags.IsEmpty() && excludeTags.IsAnySet(tags))
      return true;

    if (!includeTags.IsEmpty() && !includeTags.IsAnySet(tags))
      return true;

    return false;
  }

  struct PlaneData
  {
    ezSimdVec4f m_x0x1x2x3;
    ezSimdVec4f m_y0y1y2y3;
    ezSimdVec4f m_z0z1z2z3;
    ezSimdVec4f m_w0w1w2w3;

    ezSimdVec4f m_x4x5x4x5;
    ezSimdVec4f m_y4y5y4y5;
    ezSimdVec4f m_z4z5z4z5;
    ezSimdVec4f m_w4w5w4w5;
  };

  EZ_FORCE_INLINE bool SphereFrustumIntersect(const ezSimdBSphere& sphere, const PlaneData& planeData)
  {
    ezSimdVec4f pos_xxxx(sphere.m_CenterAndRadius.x());
    ezSimdVec4f pos_yyyy(sphere.m_CenterAndRadius.y());
    ezSimdVec4f pos_zzzz(sphere.m_CenterAndRadius.z());
    ezSimdVec4f pos_rrrr(sphere.m_CenterAndRadius.w());

    ezSimdVec4f dot_0123;
    dot_0123 = ezSimdVec4f::MulAdd(pos_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dot_0123 = ezSimdVec4f::MulAdd(pos_yyyy, planeData.m_y0y1y2y3, dot_0123);
    dot_0123 = ezSimdVec4f::MulAdd(pos_zzzz, planeData.m_z0z1z2z3, dot_0123);

    ezSimdVec4f dot_4545;
    dot_4545 = ezSimdVec4f::MulAdd(pos_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_4545 = ezSimdVec4f::MulAdd(pos_yyyy, planeData.m_y4y5y4y5, dot_4545);
    dot_4545 = ezSimdVec4f::MulAdd(pos_zzzz, planeData.m_z4z5z4z5, dot_4545);

    ezSimdVec4b cmp_0123 = dot_0123 > pos_rrrr;
    ezSimdVec4b cmp_4545 = dot_4545 > pos_rrrr;
    return (cmp_0123 || cmp_4545).NoneSet<4>();
  }

  EZ_FORCE_INLINE ezUInt32 SphereFrustumIntersect(const ezSimdBSphere& sphereA, const ezSimdBSphere& sphereB, const PlaneData& planeData)
  {
    ezSimdVec4f posA_xxxx(sphereA.m_CenterAndRadius.x());
    ezSimdVec4f posA_yyyy(sphereA.m_CenterAndRadius.y());
    ezSimdVec4f posA_zzzz(sphereA.m_CenterAndRadius.z());
    ezSimdVec4f posA_rrrr(sphereA.m_CenterAndRadius.w());

    ezSimdVec4f dotA_0123;
    dotA_0123 = ezSimdVec4f::MulAdd(posA_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotA_0123 = ezSimdVec4f::MulAdd(posA_yyyy, planeData.m_y0y1y2y3, dotA_0123);
    dotA_0123 = ezSimdVec4f::MulAdd(posA_zzzz, planeData.m_z0z1z2z3, dotA_0123);

    ezSimdVec4f posB_xxxx(sphereB.m_CenterAndRadius.x());
    ezSimdVec4f posB_yyyy(sphereB.m_CenterAndRadius.y());
    ezSimdVec4f posB_zzzz(sphereB.m_CenterAndRadius.z());
    ezSimdVec4f posB_rrrr(sphereB.m_CenterAndRadius.w());

    ezSimdVec4f dotB_0123;
    dotB_0123 = ezSimdVec4f::MulAdd(posB_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotB_0123 = ezSimdVec4f::MulAdd(posB_yyyy, planeData.m_y0y1y2y3, dotB_0123);
    dotB_0123 = ezSimdVec4f::MulAdd(posB_zzzz, planeData.m_z0z1z2z3, dotB_0123);

    ezSimdVec4f posAB_xxxx = posA_xxxx.GetCombined<ezSwizzle::XXXX>(posB_xxxx);
    ezSimdVec4f posAB_yyyy = posA_yyyy.GetCombined<ezSwizzle::XXXX>(posB_yyyy);
    ezSimdVec4f posAB_zzzz = posA_zzzz.GetCombined<ezSwizzle::XXXX>(posB_zzzz);
    ezSimdVec4f posAB_rrrr = posA_rrrr.GetCombined<ezSwizzle::XXXX>(posB_rrrr);

    ezSimdVec4f dot_A45B45;
    dot_A45B45 = ezSimdVec4f::MulAdd(posAB_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_A45B45 = ezSimdVec4f::MulAdd(posAB_yyyy, planeData.m_y4y5y4y5, dot_A45B45);
    dot_A45B45 = ezSimdVec4f::MulAdd(posAB_zzzz, planeData.m_z4z5z4z5, dot_A45B45);

    ezSimdVec4b cmp_A0123 = dotA_0123 > posA_rrrr;
    ezSimdVec4b cmp_B0123 = dotB_0123 > posB_rrrr;
    ezSimdVec4b cmp_A45B45 = dot_A45B45 > posAB_rrrr;

    ezSimdVec4b cmp_A45 = cmp_A45B45.Get<ezSwizzle::XYXY>();
    ezSimdVec4b cmp_B45 = cmp_A45B45.Get<ezSwizzle::ZWZW>();

    ezUInt32 result = (cmp_A0123 || cmp_A45).NoneSet<4>() ? 1 : 0;
    result |= (cmp_B0123 || cmp_B45).NoneSet<4>() ? 2 : 0;

    return result;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

struct CellDataMapping
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiCellIndex = ezInvalidIndex;
  ezUInt32 m_uiCellDataIndex = ezInvalidIndex;
};

struct Cell
{
  Cell(ezAllocatorBase* pAlignedAlloctor, ezAllocatorBase* pAllocator)
    : m_BoundingSpheres(pAlignedAlloctor)
    , m_CategoryBitmasks(pAllocator)
    , m_TagSets(pAllocator)
    , m_ObjectPointers(pAllocator)
    , m_DataIndices(pAllocator)
  {
  }

  EZ_FORCE_INLINE ezUInt32 AddData(const ezSimdBBoxSphere& bounds, ezUInt32 uiCategoryBitmask, const ezTagSet& tags, ezGameObject* pObject, ezUInt64 uiLastVisibleFrame, ezUInt32 uiDataIndex)
  {
    m_BoundingSpheres.PushBack(bounds.GetSphere());
    m_CategoryBitmasks.PushBack(uiCategoryBitmask);
    m_TagSets.PushBack(tags);
    m_ObjectPointers.PushBack({pObject, uiLastVisibleFrame});
    m_DataIndices.PushBack(uiDataIndex);

    return m_BoundingSpheres.GetCount() - 1;
  }

  // Returns the data index of the moved data
  EZ_FORCE_INLINE ezUInt32 RemoveData(ezUInt32 uiCellDataIndex)
  {
    ezUInt32 uiMovedDataIndex = m_DataIndices.PeekBack();

    m_BoundingSpheres.RemoveAtAndSwap(uiCellDataIndex);
    m_CategoryBitmasks.RemoveAtAndSwap(uiCellDataIndex);
    m_TagSets.RemoveAtAndSwap(uiCellDataIndex);
    m_ObjectPointers.RemoveAtAndSwap(uiCellDataIndex);
    m_DataIndices.RemoveAtAndSwap(uiCellDataIndex);

    return uiMovedDataIndex;
  }

  EZ_ALWAYS_INLINE ezBoundingBox GetBoundingBox() const { return ezSimdConversion::ToBBoxSphere(m_Bounds).GetBox(); }

  ezSimdBBoxSphere m_Bounds;

  struct ObjectPointerAndFrame
  {
    EZ_DECLARE_POD_TYPE();

    ezGameObject* m_pObject;
    ezUInt64 m_uiLastVisibleFrame;
  };

  ezDynamicArray<ezSimdBSphere> m_BoundingSpheres;
  ezDynamicArray<ezUInt32> m_CategoryBitmasks;
  ezDynamicArray<ezTagSet> m_TagSets;
  ezDynamicArray<ObjectPointerAndFrame> m_ObjectPointers;
  ezDynamicArray<ezUInt32> m_DataIndices;
};

//////////////////////////////////////////////////////////////////////////

struct CellKeyHashHelper
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    // return ezUInt32(value * 2654435761U);
    return ezHashHelper<ezUInt64>::Hash(value);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezUInt64 a, ezUInt64 b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

struct ezSpatialSystem_RegularGrid::Grid
{
  Grid(ezSpatialSystem_RegularGrid& system)
    : m_System(system)
    , m_Cells(&system.m_Allocator)
    , m_CellKeyToCellIndex(&system.m_Allocator)
  {
    ezSimdBBox overflowBox;
    overflowBox.SetCenterAndHalfExtents(ezSimdVec4f::ZeroVector(), ezSimdVec4f((float)(system.m_iCellSize.x() * MAX_CELL_INDEX)));

    auto pOverflowCell = EZ_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
    pOverflowCell->m_Bounds = overflowBox;

    m_Cells.PushBack(pOverflowCell);
  }

  ezUInt32 GetOrCreateCell(const ezSimdBBoxSphere& bounds)
  {
    ezSimdVec4i cellIndex = ToVec3I32(bounds.m_CenterAndRadius * m_System.m_fInvCellSize);
    ezSimdBBox cellBox = ComputeCellBoundingBox(cellIndex, m_System.m_iCellSize);

    if (cellBox.Contains(bounds.GetBox()))
    {
      ezUInt64 cellKey = GetCellKey(cellIndex.x(), cellIndex.y(), cellIndex.z());

      ezUInt32 uiCellIndex = 0;
      if (m_CellKeyToCellIndex.TryGetValue(cellKey, uiCellIndex))
      {
        return uiCellIndex;
      }

      uiCellIndex = m_Cells.GetCount();
      m_CellKeyToCellIndex.Insert(cellKey, uiCellIndex);

      auto pNewCell = EZ_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
      pNewCell->m_Bounds = cellBox;

      m_Cells.PushBack(pNewCell);

      return uiCellIndex;
    }
    else
    {
      return m_uiOverflowCellIndex;
    }
  }

  void AddSpatialData(const ezSimdBBoxSphere& bounds, ezUInt32 uiCategoryBitmask, const ezTagSet& tags, ezGameObject* pObject, ezUInt64 uiLastVisibleFrame, const ezSpatialDataHandle& hData)
  {
    ezUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;
    
    ezUInt32 uiCellIndex = GetOrCreateCell(bounds);
    ezUInt32 uiCellDataIndex = m_Cells[uiCellIndex]->AddData(bounds, uiCategoryBitmask, tags, pObject, uiLastVisibleFrame, uiDataIndex);

    m_CellDataMappings.EnsureCount(uiDataIndex + 1);
    m_CellDataMappings[uiDataIndex] = {uiCellIndex, uiCellDataIndex};
  }

  void RemoveSpatialData(const ezSpatialDataHandle& hData)
  {
    ezUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    auto& mapping = m_CellDataMappings[uiDataIndex];
    ezUInt32 uiMovedDataIndex = m_Cells[mapping.m_uiCellIndex]->RemoveData(mapping.m_uiCellDataIndex);
    if (uiMovedDataIndex != uiDataIndex)
    {
      m_CellDataMappings[uiMovedDataIndex].m_uiCellDataIndex = mapping.m_uiCellDataIndex;
    }

    mapping = {};
  }

  template <typename Functor>
  EZ_FORCE_INLINE void ForEachCellInBox(const ezSimdBBox& box, Functor func) const
  {
    ezSimdVec4i minIndex = ToVec3I32((box.m_Min - m_System.m_fOverlapSize) * m_System.m_fInvCellSize);
    ezSimdVec4i maxIndex = ToVec3I32((box.m_Max + m_System.m_fOverlapSize) * m_System.m_fInvCellSize);

    EZ_ASSERT_DEBUG((minIndex.Abs() < ezSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");
    EZ_ASSERT_DEBUG((maxIndex.Abs() < ezSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");

    const ezInt32 iMinX = minIndex.x();
    const ezInt32 iMinY = minIndex.y();
    const ezInt32 iMinZ = minIndex.z();

    const ezSimdVec4i diff = maxIndex - minIndex + ezSimdVec4i(1);
    const ezInt32 iDiffX = diff.x();
    const ezInt32 iDiffY = diff.y();
    const ezInt32 iDiffZ = diff.z();
    const ezInt32 iNumIterations = iDiffX * iDiffY * iDiffZ;

    for (ezInt32 i = 0; i < iNumIterations; ++i)
    {
      ezInt32 index = i;
      ezInt32 z = i / (iDiffX * iDiffY);
      index -= z * iDiffX * iDiffY;
      ezInt32 y = index / iDiffX;
      ezInt32 x = index - (y * iDiffX);

      x += iMinX;
      y += iMinY;
      z += iMinZ;

      ezUInt64 cellKey = GetCellKey(x, y, z);
      ezUInt32 cellIndex = 0;
      if (m_CellKeyToCellIndex.TryGetValue(cellKey, cellIndex))
      {
        const Cell& constCell = *m_Cells[cellIndex];
        if (func(constCell) == ezVisitorExecution::Stop)
          return;
      }
    }

    const Cell& overflowCell = *m_Cells[m_uiOverflowCellIndex];
    func(overflowCell);
  }

  ezUInt32 m_uiCategoryBitmask = 0;
  ezTagSet m_IncludeTags;
  ezTagSet m_ExcludeTags;

  ezSpatialSystem_RegularGrid& m_System;
  ezDynamicArray<ezUniquePtr<Cell>> m_Cells;

  ezHashTable<ezUInt64, ezUInt32, CellKeyHashHelper> m_CellKeyToCellIndex;
  static constexpr ezUInt32 m_uiOverflowCellIndex = 0;

  ezDynamicArray<CellDataMapping> m_CellDataMappings;
};

//////////////////////////////////////////////////////////////////////////

constexpr ezUInt32 g_AlwaysVisibleGridIndex = 0;
constexpr ezUInt32 g_DefaultGridIndex = 1;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem_RegularGrid, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSpatialSystem_RegularGrid::ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize /*= 128*/)
  : m_AlignedAllocator("Spatial System Aligned", ezFoundation::GetAlignedAllocator())
  , m_DataTable(&m_Allocator)
  , m_iCellSize(uiCellSize)
  , m_fOverlapSize(uiCellSize / 4.0f)
  , m_fInvCellSize(1.0f / uiCellSize)
{
  // Always visible and default grid
  for (ezUInt32 i = 0; i < 2; ++i)
  {
    auto pGrid = EZ_NEW(&m_Allocator, Grid, *this);
    pGrid->m_uiCategoryBitmask = ezInvalidIndex;

    m_Grids.PushBack(pGrid);
  }
}

ezSpatialSystem_RegularGrid::~ezSpatialSystem_RegularGrid() = default;

ezSpatialDataHandle ezSpatialSystem_RegularGrid::CreateSpatialData(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return ezSpatialDataHandle();

  Data data = {EZ_BIT(g_DefaultGridIndex)};
  auto hData = ezSpatialDataHandle(m_DataTable.Insert(data));

  m_Grids[g_DefaultGridIndex]->AddSpatialData(bounds, uiCategoryBitmask, tags, pObject, m_uiFrameCounter, hData);

  return hData;
}

ezSpatialDataHandle ezSpatialSystem_RegularGrid::CreateSpatialDataAlwaysVisible(ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return ezSpatialDataHandle();

  Data data = {EZ_BIT(g_AlwaysVisibleGridIndex)};
  auto hData = ezSpatialDataHandle(m_DataTable.Insert(data));

  ezSimdBBox hugeBox;
  hugeBox.SetCenterAndHalfExtents(ezSimdVec4f::ZeroVector(), ezSimdVec4f((float)(m_iCellSize.x() * MAX_CELL_INDEX)));

  m_Grids[g_AlwaysVisibleGridIndex]->AddSpatialData(hugeBox, uiCategoryBitmask, tags, pObject, m_uiFrameCounter, hData);

  return hData;
}

void ezSpatialSystem_RegularGrid::DeleteSpatialData(const ezSpatialDataHandle& hData)
{
  Data oldData;
  if (!m_DataTable.Remove(hData.GetInternalID(), &oldData))
    return;

  ForEachGrid(oldData, hData,
    [&](Grid& grid, const CellDataMapping& mapping) {
      grid.RemoveSpatialData(hData);
    });
}

void ezSpatialSystem_RegularGrid::UpdateSpatialDataBounds(const ezSpatialDataHandle& hData, const ezSimdBBoxSphere& bounds)
{
  Data* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return;

  // No need to update bounds for always visible data
  if (IsAlwaysVisibleData(*pData))
    return;

  ForEachGrid(*pData, hData,
    [&](Grid& grid, const CellDataMapping& mapping) {
      auto& pOldCell = grid.m_Cells[mapping.m_uiCellIndex];

      if (pOldCell->m_Bounds.GetBox().Contains(bounds.GetBox()))
      {
        pOldCell->m_BoundingSpheres[mapping.m_uiCellDataIndex] = bounds.GetSphere();
      }
      else
      {
        ezUInt32 uiNewCellIndex = grid.GetOrCreateCell(bounds);
        if (mapping.m_uiCellIndex == uiNewCellIndex)
        {
          EZ_ASSERT_DEBUG(false, "should never happen?");
          pOldCell->m_BoundingSpheres[mapping.m_uiCellDataIndex] = bounds.GetSphere();
        }
        else
        {
          // Migrate all data to new cell
          ezUInt32 uiCategoryBitmask = pOldCell->m_CategoryBitmasks[mapping.m_uiCellDataIndex];
          const ezTagSet& tags = pOldCell->m_TagSets[mapping.m_uiCellDataIndex];
          auto& objectPointer = pOldCell->m_ObjectPointers[mapping.m_uiCellDataIndex];
          grid.AddSpatialData(bounds, uiCategoryBitmask, tags, objectPointer.m_pObject, objectPointer.m_uiLastVisibleFrame, hData);

          // Remove data from old cell
          grid.RemoveSpatialData(hData);
        }
      }
    });
}

void ezSpatialSystem_RegularGrid::UpdateSpatialDataObject(const ezSpatialDataHandle& hData, ezGameObject* pObject)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const
{
  EZ_PROFILE_SCOPE("FindObjectsInSphere");

  ezSimdBSphere simdSphere(ezSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);
  ezSimdBBox simdBox;
  simdBox.SetCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<ezSwizzle::WWWW>());

  m_Grids[0]->ForEachCellInBox(
    simdBox, [&](const Cell& cell) {
      ezSimdBBox cellBox = cell.m_Bounds.GetBox();
      if (!cellBox.Overlaps(simdSphere))
        return ezVisitorExecution::Continue;

      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto categoryBitmasks = cell.m_CategoryBitmasks.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();

      const ezUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (queryParams.m_pStats != nullptr)
      {
        queryParams.m_pStats->m_uiNumObjectsTested += numSpheres;
      }
#endif

      for (ezUInt32 i = 0; i < numSpheres; ++i)
      {
        if (FilterByCategory(categoryBitmasks[i], queryParams.m_uiCategoryBitmask))
          continue;

        if (!simdSphere.Overlaps(boundingSpheres[i]))
          continue;

        if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
          continue;

        if (callback(objectPointers[i].m_pObject) == ezVisitorExecution::Stop)
          return ezVisitorExecution::Stop;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
        if (queryParams.m_pStats != nullptr)
        {
          queryParams.m_pStats->m_uiNumObjectsPassed++;
        }
#endif
      }

      return ezVisitorExecution::Continue;
    });

  /*for (auto& alwaysVisibleData : m_AlwaysVisibleData)
  {
    if (FilterByCategory(alwaysVisibleData.m_uiCategoryBitmask, queryParams.m_uiCategoryBitmask))
      continue;

    if (FilterByTags(alwaysVisibleData.m_Tags, queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
      continue;

    callback(alwaysVisibleData.m_pObject);
  }*/
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const
{
  EZ_PROFILE_SCOPE("FindObjectsInBox");

  ezSimdBBox simdBox(ezSimdConversion::ToVec3(box.m_vMin), ezSimdConversion::ToVec3(box.m_vMax));

  m_Grids[0]->ForEachCellInBox(
    simdBox, [&](const Cell& cell) {
      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto categoryBitmasks = cell.m_CategoryBitmasks.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();

      const ezUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (queryParams.m_pStats != nullptr)
      {
        queryParams.m_pStats->m_uiNumObjectsTested += numSpheres;
      }
#endif

      for (ezUInt32 i = 0; i < numSpheres; ++i)
      {
        if (FilterByCategory(categoryBitmasks[i], queryParams.m_uiCategoryBitmask))
          continue;

        if (!simdBox.Overlaps(boundingSpheres[i]))
          continue;

        if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
          continue;

        if (callback(objectPointers[i].m_pObject) == ezVisitorExecution::Stop)
          return ezVisitorExecution::Stop;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
        if (queryParams.m_pStats != nullptr)
        {
          queryParams.m_pStats->m_uiNumObjectsPassed++;
        }
#endif
      }

      return ezVisitorExecution::Continue;
    });

  /*for (auto& alwaysVisibleData : m_AlwaysVisibleData)
  {
    if (FilterByCategory(alwaysVisibleData.m_uiCategoryBitmask, queryParams.m_uiCategoryBitmask))
      continue;

    if (FilterByTags(alwaysVisibleData.m_Tags, queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
      continue;

    callback(alwaysVisibleData.m_pObject);
  }*/
}

void ezSpatialSystem_RegularGrid::FindVisibleObjects(const ezFrustum& frustum, const QueryParams& queryParams, ezDynamicArray<const ezGameObject*>& out_Objects) const
{
  EZ_PROFILE_SCOPE("FindVisibleObjects");

  /*for (auto& alwaysVisibleData : m_AlwaysVisibleData)
  {
    if (FilterByCategory(alwaysVisibleData.m_uiCategoryBitmask, queryParams.m_uiCategoryBitmask))
      continue;

    if (FilterByTags(alwaysVisibleData.m_Tags, queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
      continue;

    out_Objects.PushBack(alwaysVisibleData.m_pObject);
  }*/
}

ezUInt64 ezSpatialSystem_RegularGrid::GetNumFramesSinceVisible(const ezSpatialDataHandle& hData) const
{
  Data* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return 0;

  if (IsAlwaysVisibleData(*pData))
    return 0;

  ezUInt64 uiLastFrameVisible = 0;
  ForEachGrid(*pData, hData,
    [&](const Grid& grid, const CellDataMapping& mapping) {
      auto& pCell = grid.m_Cells[mapping.m_uiCellIndex];
      uiLastFrameVisible = ezMath::Max(uiLastFrameVisible, pCell->m_ObjectPointers[mapping.m_uiCellDataIndex].m_uiLastVisibleFrame);
    });

  return (m_uiFrameCounter > uiLastFrameVisible) ? m_uiFrameCounter - uiLastFrameVisible : 0;
}

EZ_ALWAYS_INLINE bool ezSpatialSystem_RegularGrid::IsAlwaysVisibleData(const Data& data) const
{
  return (data.m_uiGridBitmask & EZ_BIT(g_AlwaysVisibleGridIndex)) == EZ_BIT(g_AlwaysVisibleGridIndex);
}

template <typename Functor>
void ezSpatialSystem_RegularGrid::ForEachGrid(const Data& data, const ezSpatialDataHandle& hData, Functor func) const
{
  ezUInt64 uiGridBitmask = data.m_uiGridBitmask;
  ezUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

  while (uiGridBitmask > 0)
  {
    ezUInt32 uiGridIndex = ezMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& grid = *m_Grids[uiGridIndex];
    auto& mapping = grid.m_CellDataMappings[uiDataIndex];
    func(grid, mapping);
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem_RegularGrid);
