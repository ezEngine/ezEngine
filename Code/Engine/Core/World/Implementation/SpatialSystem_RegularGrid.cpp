#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Stopwatch.h>

ezCVarInt cvar_SpatialQueriesCachingThreshold("Spatial.Queries.CachingThreshold", 100, ezCVarFlags::Default, "Number of objects that are tested for a query before it is considered for caching");

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

namespace
{
  enum
  {
    MAX_CELL_INDEX = (1 << 20) - 1,
    CELL_INDEX_MASK = (1 << 21) - 1
  };

  EZ_ALWAYS_INLINE ezSimdVec4f ToVec3(const ezSimdVec4i& v)
  {
    return v.ToFloat();
  }

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

  EZ_ALWAYS_INLINE ezSimdBBox ComputeCellBoundingBox(const ezSimdVec4i& vCellIndex, const ezSimdVec4i& vCellSize)
  {
    ezSimdVec4i overlapSize = vCellSize >> 2;
    ezSimdVec4i minPos = vCellIndex.CompMul(vCellSize);

    ezSimdVec4f bmin = ToVec3(minPos - overlapSize);
    ezSimdVec4f bmax = ToVec3(minPos + overlapSize + vCellSize);

    return ezSimdBBox(bmin, bmax);
  }

  EZ_ALWAYS_INLINE bool AreTagSetsEqual(const ezTagSet& a, const ezTagSet* pB)
  {
    if (pB != nullptr)
    {
      return a == *pB;
    }

    return a.IsEmpty();
  }

  EZ_ALWAYS_INLINE bool FilterByTags(const ezTagSet& tags, const ezTagSet* pIncludeTags, const ezTagSet* pExcludeTags)
  {
    if (pExcludeTags != nullptr && !pExcludeTags->IsEmpty() && pExcludeTags->IsAnySet(tags))
      return true;

    if (pIncludeTags != nullptr && !pIncludeTags->IsEmpty() && !pIncludeTags->IsAnySet(tags))
      return true;

    return false;
  }

  EZ_ALWAYS_INLINE bool CanBeCached(ezSpatialData::Category category)
  {
    return ezSpatialData::GetCategoryFlags(category).IsSet(ezSpatialData::Flags::FrequentChanges) == false;
  }


#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  void TagsToString(const ezTagSet& tags, ezStringBuilder& out_sSb)
  {
    out_sSb.Append("{ ");

    bool first = true;
    for (auto it = tags.GetIterator(); it.IsValid(); ++it)
    {
      if (!first)
      {
        out_sSb.Append(", ");
        first = false;
      }
      out_sSb.Append(it->GetTagString().GetView());
    }

    out_sSb.Append(" }");
  }
#endif

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

struct ezSpatialSystem_RegularGrid::Cell
{
  Cell(ezAllocator* pAlignedAlloctor, ezAllocator* pAllocator)
    : m_BoundingSpheres(pAlignedAlloctor)
    , m_BoundingBoxHalfExtents(pAlignedAlloctor)
    , m_TagSets(pAllocator)
    , m_ObjectPointers(pAllocator)
    , m_DataIndices(pAllocator)
  {
  }

  EZ_FORCE_INLINE ezUInt32 AddData(const ezSimdBBoxSphere& bounds, const ezTagSet& tags, ezGameObject* pObject, ezUInt64 uiLastVisibleFrameIdxAndVisType, ezUInt32 uiDataIndex)
  {
    m_BoundingSpheres.PushBack(bounds.GetSphere());
    m_BoundingBoxHalfExtents.PushBack(bounds.m_BoxHalfExtents);
    m_TagSets.PushBack(tags);
    m_ObjectPointers.PushBack(pObject);
    m_DataIndices.PushBack(uiDataIndex);
    m_LastVisibleFrameIdxAndVisType.PushBack(uiLastVisibleFrameIdxAndVisType);

    return m_BoundingSpheres.GetCount() - 1;
  }

  // Returns the data index of the moved data
  EZ_FORCE_INLINE ezUInt32 RemoveData(ezUInt32 uiCellDataIndex)
  {
    ezUInt32 uiMovedDataIndex = m_DataIndices.PeekBack();

    m_BoundingSpheres.RemoveAtAndSwap(uiCellDataIndex);
    m_BoundingBoxHalfExtents.RemoveAtAndSwap(uiCellDataIndex);
    m_TagSets.RemoveAtAndSwap(uiCellDataIndex);
    m_ObjectPointers.RemoveAtAndSwap(uiCellDataIndex);
    m_DataIndices.RemoveAtAndSwap(uiCellDataIndex);
    m_LastVisibleFrameIdxAndVisType.RemoveAtAndSwap(uiCellDataIndex);

    EZ_ASSERT_DEBUG(m_DataIndices.GetCount() == uiCellDataIndex || m_DataIndices[uiCellDataIndex] == uiMovedDataIndex, "Implementation error");

    return uiMovedDataIndex;
  }

  EZ_ALWAYS_INLINE ezBoundingBox GetBoundingBox() const { return ezSimdConversion::ToBBoxSphere(m_Bounds).GetBox(); }

  ezSimdBBoxSphere m_Bounds;

  ezDynamicArray<ezSimdBSphere> m_BoundingSpheres;
  ezDynamicArray<ezSimdVec4f> m_BoundingBoxHalfExtents;
  ezDynamicArray<ezTagSet> m_TagSets;
  ezDynamicArray<ezGameObject*> m_ObjectPointers;
  mutable ezDynamicArray<ezAtomicInteger64> m_LastVisibleFrameIdxAndVisType;
  ezDynamicArray<ezUInt32> m_DataIndices;
};

//////////////////////////////////////////////////////////////////////////

struct CellKeyHashHelper
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    // manually unrolled MurmurHash32
    const ezUInt32 m = ezInternal::MURMUR_M;
    const ezUInt32 r = ezInternal::MURMUR_R;

    ezUInt32 h = 8;
    {
      ezUInt32 k = ezUInt32(value);

      k *= m;
      k ^= k >> r;
      k *= m;

      h *= m;
      h ^= k;
    }

    {
      ezUInt32 k = ezUInt32(value >> 32);

      k *= m;
      k ^= k >> r;
      k *= m;

      h *= m;
      h ^= k;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }

  EZ_ALWAYS_INLINE static bool Equal(ezUInt64 a, ezUInt64 b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

struct ezSpatialSystem_RegularGrid::Grid
{
  Grid(ezSpatialSystem_RegularGrid& ref_system, ezSpatialData::Category category)
    : m_System(ref_system)
    , m_Cells(&ref_system.m_Allocator)
    , m_CellKeyToCellIndex(&ref_system.m_Allocator)
    , m_Category(category)
    , m_bCanBeCached(CanBeCached(category))
  {
    const ezSimdBBox overflowBox = ezSimdBBox::MakeFromCenterAndHalfExtents(ezSimdVec4f::MakeZero(), ezSimdVec4f((float)(ref_system.m_vCellSize.x() * MAX_CELL_INDEX)));

    auto pOverflowCell = EZ_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
    pOverflowCell->m_Bounds = overflowBox;

    m_Cells.PushBack(pOverflowCell);
  }

  ezUInt32 GetOrCreateCell(const ezSimdBBoxSphere& bounds)
  {
    ezSimdVec4i cellIndex = ToVec3I32(bounds.m_CenterAndRadius * m_System.m_fInvCellSize);
    ezSimdBBox cellBox = ComputeCellBoundingBox(cellIndex, m_System.m_vCellSize);

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

  void AddSpatialData(const ezSimdBBoxSphere& bounds, const ezTagSet& tags, ezGameObject* pObject, ezUInt64 uiLastVisibleFrameIdxAndVisType, const ezSpatialDataHandle& hData)
  {
    ezUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    ezUInt32 uiCellIndex = GetOrCreateCell(bounds);
    ezUInt32 uiCellDataIndex = m_Cells[uiCellIndex]->AddData(bounds, tags, pObject, uiLastVisibleFrameIdxAndVisType, uiDataIndex);

    m_CellDataMappings.EnsureCount(uiDataIndex + 1);
    EZ_ASSERT_DEBUG(m_CellDataMappings[uiDataIndex].m_uiCellIndex == ezInvalidIndex, "data has already been added to a cell");
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

  bool MigrateSpatialDataFromOtherGrid(ezUInt32 uiDataIndex, const Grid& other)
  {
    // Data has already been added
    if (uiDataIndex < m_CellDataMappings.GetCount() && m_CellDataMappings[uiDataIndex].m_uiCellIndex != ezInvalidIndex)
      return false;

    auto& mapping = other.m_CellDataMappings[uiDataIndex];
    if (mapping.m_uiCellIndex == ezInvalidIndex)
      return false;

    auto& pOtherCell = other.m_Cells[mapping.m_uiCellIndex];

    const ezTagSet& tags = pOtherCell->m_TagSets[mapping.m_uiCellDataIndex];
    if (FilterByTags(tags, &m_IncludeTags, &m_ExcludeTags))
      return false;

    ezSimdBBoxSphere bounds;
    bounds.m_CenterAndRadius = pOtherCell->m_BoundingSpheres[mapping.m_uiCellDataIndex].m_CenterAndRadius;
    bounds.m_BoxHalfExtents = pOtherCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex];
    ezGameObject* objectPointer = pOtherCell->m_ObjectPointers[mapping.m_uiCellDataIndex];
    const ezUInt64 uiLastVisibleFrameIdxAndVisType = pOtherCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

    EZ_ASSERT_DEBUG(pOtherCell->m_DataIndices[mapping.m_uiCellDataIndex] == uiDataIndex, "Implementation error");
    ezSpatialDataHandle hData = ezSpatialDataHandle(ezSpatialDataId(uiDataIndex, 1));

    AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
    return true;
  }

  EZ_ALWAYS_INLINE bool CachingCompleted() const { return m_uiLastMigrationIndex == ezInvalidIndex; }

  template <typename Functor>
  EZ_FORCE_INLINE void ForEachCellInBox(const ezSimdBBox& box, Functor func) const
  {
    ezSimdVec4i minIndex = ToVec3I32((box.m_Min - m_System.m_vOverlapSize) * m_System.m_fInvCellSize);
    ezSimdVec4i maxIndex = ToVec3I32((box.m_Max + m_System.m_vOverlapSize) * m_System.m_fInvCellSize);

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

    // The hash grid approach below is about 10 times slower than simply iterating over all cells
    // and doing an AABB overlap test
    const ezUInt64 uiHashGridCost = ezUInt64(iNumIterations) * 10;
    if (uiHashGridCost > m_Cells.GetCount())
    {
      for (auto& pCell : m_Cells)
      {
        if (box.Overlaps(pCell->m_Bounds.GetBox()) == false)
          continue;

        if (func(*pCell) == ezVisitorExecution::Stop)
          return;
      }
    }
    else
    {
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
  }

  ezSpatialSystem_RegularGrid& m_System;
  ezDynamicArray<ezUniquePtr<Cell>> m_Cells;

  ezHashTable<ezUInt64, ezUInt32, CellKeyHashHelper> m_CellKeyToCellIndex;
  static constexpr ezUInt32 m_uiOverflowCellIndex = 0;

  ezDynamicArray<CellDataMapping> m_CellDataMappings;

  const ezSpatialData::Category m_Category;
  const bool m_bCanBeCached;

  ezTagSet m_IncludeTags;
  ezTagSet m_ExcludeTags;

  ezUInt32 m_uiLastMigrationIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

struct ezSpatialSystem_RegularGrid::Stats
{
  ezUInt32 m_uiNumObjectsTested = 0;
  ezUInt32 m_uiNumObjectsPassed = 0;
  ezUInt32 m_uiNumObjectsFiltered = 0;
};

//////////////////////////////////////////////////////////////////////////

namespace ezInternal
{
  struct QueryHelper
  {
    template <typename T>
    struct ShapeQueryData
    {
      T m_Shape;
      ezSpatialSystem::QueryCallback m_Callback;
    };

    template <typename T, bool UseTagsFilter>
    static ezVisitorExecution::Enum ShapeQueryCallback(const ezSpatialSystem_RegularGrid::Cell& cell, const ezSpatialSystem::QueryParams& queryParams, ezSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, ezVisibilityState::Enum visType)
    {
      EZ_IGNORE_UNUSED(visType);

      auto pQueryData = static_cast<const ShapeQueryData<T>*>(pUserData);
      T shape = pQueryData->m_Shape;

      ezSimdBBox cellBox = cell.m_Bounds.GetBox();
      if (!cellBox.Overlaps(shape))
        return ezVisitorExecution::Continue;

      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();

      const ezUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      for (ezUInt32 i = 0; i < numSpheres; ++i)
      {
        if (!shape.Overlaps(boundingSpheres[i]))
          continue;

        if constexpr (UseTagsFilter)
        {
          if (FilterByTags(tagSets[i], queryParams.m_pIncludeTags, queryParams.m_pExcludeTags))
          {
            ref_stats.m_uiNumObjectsFiltered++;
            continue;
          }
        }

        ref_stats.m_uiNumObjectsPassed++;

        if (pQueryData->m_Callback(objectPointers[i]) == ezVisitorExecution::Stop)
          return ezVisitorExecution::Stop;
      }

      return ezVisitorExecution::Continue;
    }

    struct FrustumQueryData
    {
      PlaneData m_PlaneData;
      ezDynamicArray<const ezGameObject*>* m_pOutObjects;
      ezUInt64 m_uiFrameCounter;
      ezSpatialSystem::IsOccludedFunc m_IsOccludedCB;
    };

    template <bool UseTagsFilter, bool UseOcclusionCallback>
    static ezVisitorExecution::Enum FrustumQueryCallback(const ezSpatialSystem_RegularGrid::Cell& cell, const ezSpatialSystem::QueryParams& queryParams, ezSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, ezVisibilityState::Enum visType)
    {
      auto pQueryData = static_cast<FrustumQueryData*>(pUserData);
      PlaneData planeData = pQueryData->m_PlaneData;

      ezSimdBSphere cellSphere = cell.m_Bounds.GetSphere();
      if (!SphereFrustumIntersect(cellSphere, planeData))
        return ezVisitorExecution::Continue;

      if constexpr (UseOcclusionCallback)
      {
        if (pQueryData->m_IsOccludedCB(cell.m_Bounds.GetBox()))
        {
          return ezVisitorExecution::Continue;
        }
      }

      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto boundingBoxHalfExtents = cell.m_BoundingBoxHalfExtents.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();
      auto lastVisibleFrameIdxAndVisType = cell.m_LastVisibleFrameIdxAndVisType.GetData();

      const ezUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      ezUInt32 currentIndex = 0;
      const ezUInt64 uiFrameIdxAndType = (pQueryData->m_uiFrameCounter << 4) | static_cast<ezUInt64>(visType);

      while (currentIndex < numSpheres)
      {
        if (numSpheres - currentIndex >= 32)
        {
          ezUInt32 mask = 0;

          for (ezUInt32 i = 0; i < 32; i += 2)
          {
            auto& objectSphereA = boundingSpheres[currentIndex + i + 0];
            auto& objectSphereB = boundingSpheres[currentIndex + i + 1];

            mask |= SphereFrustumIntersect(objectSphereA, objectSphereB, planeData) << i;
          }

          while (mask > 0)
          {
            ezUInt32 i = ezMath::FirstBitLow(mask) + currentIndex;
            mask &= mask - 1;

            if constexpr (UseTagsFilter)
            {
              if (FilterByTags(tagSets[i], queryParams.m_pIncludeTags, queryParams.m_pExcludeTags))
              {
                ref_stats.m_uiNumObjectsFiltered++;
                continue;
              }
            }

            if constexpr (UseOcclusionCallback)
            {
              const ezSimdBBox bbox = ezSimdBBox::MakeFromCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);
              if (pQueryData->m_IsOccludedCB(bbox))
              {
                continue;
              }
            }

            lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
            pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

            ref_stats.m_uiNumObjectsPassed++;
          }

          currentIndex += 32;
        }
        else
        {
          ezUInt32 i = currentIndex;
          ++currentIndex;

          if (!SphereFrustumIntersect(boundingSpheres[i], planeData))
            continue;

          if constexpr (UseTagsFilter)
          {
            if (FilterByTags(tagSets[i], queryParams.m_pIncludeTags, queryParams.m_pExcludeTags))
            {
              ref_stats.m_uiNumObjectsFiltered++;
              continue;
            }
          }

          if constexpr (UseOcclusionCallback)
          {
            const ezSimdBBox bbox = ezSimdBBox::MakeFromCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);

            if (pQueryData->m_IsOccludedCB(bbox))
            {
              continue;
            }
          }

          lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
          pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

          ref_stats.m_uiNumObjectsPassed++;
        }
      }

      return ezVisitorExecution::Continue;
    }
  };
} // namespace ezInternal

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem_RegularGrid, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSpatialSystem_RegularGrid::ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize /*= 128*/)
  : m_AlignedAllocator("Spatial System Aligned", ezFoundation::GetAlignedAllocator())
  , m_vCellSize(uiCellSize)
  , m_vOverlapSize(uiCellSize / 4.0f)
  , m_fInvCellSize(1.0f / uiCellSize)
  , m_Grids(&m_Allocator)
  , m_DataTable(&m_Allocator)
{
  static_assert(sizeof(Data) == 8);

  m_Grids.SetCount(MAX_NUM_GRIDS);

  cvar_SpatialQueriesCachingThreshold.m_CVarEvents.AddEventHandler([&](const ezCVarEvent& e)
    {
    if (e.m_EventType == ezCVarEvent::ValueChanged)
    {
      RemoveAllCachedGrids();
    } });
}

ezSpatialSystem_RegularGrid::~ezSpatialSystem_RegularGrid() = default;

ezResult ezSpatialSystem_RegularGrid::GetCellBoxForSpatialData(const ezSpatialDataHandle& hData, ezBoundingBox& out_boundingBox) const
{
  Data* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return EZ_FAILURE;

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      auto& pCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

      out_boundingBox = pCell->GetBoundingBox();
      return ezVisitorExecution::Stop;
    });

  return EZ_SUCCESS;
}

template <>
struct ezHashHelper<ezBoundingBox>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezBoundingBox& value) { return ezHashingUtils::xxHash32(&value, sizeof(ezBoundingBox)); }

  EZ_ALWAYS_INLINE static bool Equal(const ezBoundingBox& a, const ezBoundingBox& b) { return a == b; }
};

void ezSpatialSystem_RegularGrid::GetAllCellBoxes(ezDynamicArray<ezBoundingBox>& out_boundingBoxes, ezSpatialData::Category filterCategory /*= ezInvalidSpatialDataCategory*/) const
{
  if (filterCategory != ezInvalidSpatialDataCategory)
  {
    ezUInt32 uiGridIndex = filterCategory.m_uiValue;
    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid != nullptr)
    {
      for (auto& pCell : pGrid->m_Cells)
      {
        out_boundingBoxes.ExpandAndGetRef() = pCell->GetBoundingBox();
      }
    }
  }
  else
  {
    ezHashSet<ezBoundingBox> boundingBoxes;

    for (auto& pGrid : m_Grids)
    {
      if (pGrid != nullptr)
      {
        for (auto& pCell : pGrid->m_Cells)
        {
          boundingBoxes.Insert(pCell->GetBoundingBox());
        }
      }
    }

    for (auto boundingBox : boundingBoxes)
    {
      out_boundingBoxes.PushBack(boundingBox);
    }
  }
}

void ezSpatialSystem_RegularGrid::StartNewFrame()
{
  SUPER::StartNewFrame();

  m_SortedCacheCandidates.Clear();

  {
    EZ_LOCK(m_CacheCandidatesMutex);

    for (ezUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
    {
      auto& cacheCandidate = m_CacheCandidates[i];

      const float fScore = cacheCandidate.m_fQueryCount + cacheCandidate.m_fFilteredRatio * 100.0f;
      m_SortedCacheCandidates.PushBack({i, fScore});

      // Query has to be issued at least once every 10 frames to keep a stable value
      cacheCandidate.m_fQueryCount = ezMath::Max(cacheCandidate.m_fQueryCount - 0.1f, 0.0f);
    }
  }

  m_SortedCacheCandidates.Sort();

  // First remove all cached grids that don't make it into the top MAX_NUM_CACHED_GRIDS to make space for new grids
  if (m_SortedCacheCandidates.GetCount() > MAX_NUM_CACHED_GRIDS)
  {
    for (ezUInt32 i = MAX_NUM_CACHED_GRIDS; i < m_SortedCacheCandidates.GetCount(); ++i)
    {
      RemoveCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
    }
  }

  // Then take the MAX_NUM_CACHED_GRIDS candidates with the highest score and migrate the data
  for (ezUInt32 i = 0; i < ezMath::Min<ezUInt32>(m_SortedCacheCandidates.GetCount(), MAX_NUM_CACHED_GRIDS); ++i)
  {
    MigrateCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
  }
}

ezSpatialDataHandle ezSpatialSystem_RegularGrid::CreateSpatialData(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return ezSpatialDataHandle();

  return AddSpatialDataToGrids(bounds, pObject, uiCategoryBitmask, tags, false);
}

ezSpatialDataHandle ezSpatialSystem_RegularGrid::CreateSpatialDataAlwaysVisible(ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return ezSpatialDataHandle();

  const ezSimdBBox hugeBox = ezSimdBBox::MakeFromCenterAndHalfExtents(ezSimdVec4f::MakeZero(), ezSimdVec4f((float)(m_vCellSize.x() * MAX_CELL_INDEX)));

  return AddSpatialDataToGrids(hugeBox, pObject, uiCategoryBitmask, tags, true);
}

void ezSpatialSystem_RegularGrid::DeleteSpatialData(const ezSpatialDataHandle& hData)
{
  Data oldData;
  EZ_VERIFY(m_DataTable.Remove(hData.GetInternalID(), &oldData), "Invalid spatial data handle");

  ForEachGrid(oldData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      EZ_IGNORE_UNUSED(mapping);
      ref_grid.RemoveSpatialData(hData);
      return ezVisitorExecution::Continue;
    });
}

void ezSpatialSystem_RegularGrid::UpdateSpatialDataBounds(const ezSpatialDataHandle& hData, const ezSimdBBoxSphere& bounds)
{
  Data* pData = nullptr;
  EZ_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  // No need to update bounds for always visible data
  if (IsAlwaysVisibleData(*pData))
    return;

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      auto& pOldCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

      if (pOldCell->m_Bounds.GetBox().Contains(bounds.GetBox()))
      {
        pOldCell->m_BoundingSpheres[mapping.m_uiCellDataIndex] = bounds.GetSphere();
        pOldCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex] = bounds.m_BoxHalfExtents;
      }
      else
      {
        const ezTagSet tags = pOldCell->m_TagSets[mapping.m_uiCellDataIndex];
        ezGameObject* objectPointer = pOldCell->m_ObjectPointers[mapping.m_uiCellDataIndex];

        const ezUInt64 uiLastVisibleFrameIdxAndVisType = pOldCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

        ref_grid.RemoveSpatialData(hData);

        ref_grid.AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
      }

      return ezVisitorExecution::Continue;
    });
}

void ezSpatialSystem_RegularGrid::UpdateSpatialDataObject(const ezSpatialDataHandle& hData, ezGameObject* pObject)
{
  Data* pData = nullptr;
  EZ_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping)
    {
      auto& pCell = ref_grid.m_Cells[mapping.m_uiCellIndex];
      pCell->m_ObjectPointers[mapping.m_uiCellDataIndex] = pObject;
      return ezVisitorExecution::Continue;
    });
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const
{
  EZ_PROFILE_SCOPE("FindObjectsInSphere");

  ezSimdBSphere simdSphere(ezSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);

  const ezSimdBBox simdBox = ezSimdBBox::MakeFromCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<ezSwizzle::WWWW>());

  ezInternal::QueryHelper::ShapeQueryData<ezSimdBSphere> queryData = {simdSphere, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
    &ezInternal::QueryHelper::ShapeQueryCallback<ezSimdBSphere, false>,
    &ezInternal::QueryHelper::ShapeQueryCallback<ezSimdBSphere, true>,
    &queryData, ezVisibilityState::Indirect);
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const
{
  EZ_PROFILE_SCOPE("FindObjectsInBox");

  ezSimdBBox simdBox(ezSimdConversion::ToVec3(box.m_vMin), ezSimdConversion::ToVec3(box.m_vMax));

  ezInternal::QueryHelper::ShapeQueryData<ezSimdBBox> queryData = {simdBox, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
    &ezInternal::QueryHelper::ShapeQueryCallback<ezSimdBBox, false>,
    &ezInternal::QueryHelper::ShapeQueryCallback<ezSimdBBox, true>,
    &queryData, ezVisibilityState::Indirect);
}

void ezSpatialSystem_RegularGrid::FindVisibleObjects(const ezFrustum& frustum, const QueryParams& queryParams, ezDynamicArray<const ezGameObject*>& out_Objects, ezSpatialSystem::IsOccludedFunc IsOccluded, ezVisibilityState::Enum visType) const
{
  EZ_PROFILE_SCOPE("FindVisibleObjects");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezStopwatch timer;
#endif

  ezVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints).AssertSuccess();

  ezSimdVec4f simdCornerPoints[8];
  for (ezUInt32 i = 0; i < 8; ++i)
  {
    simdCornerPoints[i] = ezSimdConversion::ToVec3(cornerPoints[i]);
  }

  const ezSimdBBox simdBox = ezSimdBBox::MakeFromPoints(simdCornerPoints, 8);

  ezInternal::QueryHelper::FrustumQueryData queryData;
  {
    // Compiler is too stupid to properly unroll a constant loop so we do it by hand
    ezSimdVec4f plane0 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(0).m_vNormal.x)));
    ezSimdVec4f plane1 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(1).m_vNormal.x)));
    ezSimdVec4f plane2 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(2).m_vNormal.x)));
    ezSimdVec4f plane3 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(3).m_vNormal.x)));
    ezSimdVec4f plane4 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(4).m_vNormal.x)));
    ezSimdVec4f plane5 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(5).m_vNormal.x)));

    ezSimdMat4f helperMat;
    helperMat.SetRows(plane0, plane1, plane2, plane3);

    queryData.m_PlaneData.m_x0x1x2x3 = helperMat.m_col0;
    queryData.m_PlaneData.m_y0y1y2y3 = helperMat.m_col1;
    queryData.m_PlaneData.m_z0z1z2z3 = helperMat.m_col2;
    queryData.m_PlaneData.m_w0w1w2w3 = helperMat.m_col3;

    helperMat.SetRows(plane4, plane5, plane4, plane5);

    queryData.m_PlaneData.m_x4x5x4x5 = helperMat.m_col0;
    queryData.m_PlaneData.m_y4y5y4y5 = helperMat.m_col1;
    queryData.m_PlaneData.m_z4z5z4z5 = helperMat.m_col2;
    queryData.m_PlaneData.m_w4w5w4w5 = helperMat.m_col3;

    queryData.m_pOutObjects = &out_Objects;
    queryData.m_uiFrameCounter = m_uiFrameCounter;

    queryData.m_IsOccludedCB = IsOccluded;
  }

  if (IsOccluded.IsValid())
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
      &ezInternal::QueryHelper::FrustumQueryCallback<false, true>,
      &ezInternal::QueryHelper::FrustumQueryCallback<true, true>,
      &queryData, visType);
  }
  else
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
      &ezInternal::QueryHelper::FrustumQueryCallback<false, false>,
      &ezInternal::QueryHelper::FrustumQueryCallback<true, false>,
      &queryData, visType);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_TimeTaken = timer.GetRunningTotal();
  }
#endif
}

ezVisibilityState::Enum ezSpatialSystem_RegularGrid::GetVisibilityState(const ezSpatialDataHandle& hData, ezUInt32 uiNumFramesBeforeInvisible) const
{
  Data* pData = nullptr;
  EZ_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  if (IsAlwaysVisibleData(*pData))
    return ezVisibilityState::Direct;

  ezUInt64 uiLastVisibleFrameIdxAndVisType = 0;
  ForEachGrid(*pData, hData,
    [&](const Grid& grid, const CellDataMapping& mapping)
    {
      auto& pCell = grid.m_Cells[mapping.m_uiCellIndex];
      uiLastVisibleFrameIdxAndVisType = ezMath::Max<ezUInt64>(uiLastVisibleFrameIdxAndVisType, pCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex]);
      return ezVisitorExecution::Continue;
    });

  const ezUInt64 uiLastVisibleFrameIdx = (uiLastVisibleFrameIdxAndVisType >> 4);
  const ezUInt64 uiLastVisibilityType = (uiLastVisibleFrameIdxAndVisType & static_cast<ezUInt64>(15)); // mask out lower 4 bits

  if (m_uiFrameCounter > uiLastVisibleFrameIdx + uiNumFramesBeforeInvisible)
    return ezVisibilityState::Invisible;

  return static_cast<ezVisibilityState::Enum>(uiLastVisibilityType);
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
void ezSpatialSystem_RegularGrid::GetInternalStats(ezStringBuilder& sb) const
{
  EZ_LOCK(m_CacheCandidatesMutex);

  ezUInt32 uiNumActiveGrids = 0;
  for (auto& pGrid : m_Grids)
  {
    uiNumActiveGrids += (pGrid != nullptr) ? 1 : 0;
  }

  sb.SetFormat("Num Grids: {}\n", uiNumActiveGrids);

  for (auto& pGrid : m_Grids)
  {
    if (pGrid == nullptr)
      continue;

    sb.AppendFormat(" \nCategory: {}, CanBeCached: {}\nIncludeTags: ", ezSpatialData::GetCategoryName(pGrid->m_Category), pGrid->m_bCanBeCached);
    TagsToString(pGrid->m_IncludeTags, sb);
    sb.Append(", ExcludeTags: ");
    TagsToString(pGrid->m_ExcludeTags, sb);
    sb.Append("\n");
  }

  sb.Append("\nCache Candidates:\n");

  for (auto& sortedCandidate : m_SortedCacheCandidates)
  {
    auto& candidate = m_CacheCandidates[sortedCandidate.m_uiIndex];
    const ezUInt32 uiGridIndex = candidate.m_uiGridIndex;
    Grid* pGrid = nullptr;

    if (uiGridIndex != ezInvalidIndex)
    {
      pGrid = m_Grids[uiGridIndex].Borrow();
      if (pGrid->CachingCompleted())
      {
        continue;
      }
    }

    sb.AppendFormat(" \nCategory: {}\nIncludeTags: ", ezSpatialData::GetCategoryName(candidate.m_Category));
    TagsToString(candidate.m_IncludeTags, sb);
    sb.Append(", ExcludeTags: ");
    TagsToString(candidate.m_ExcludeTags, sb);
    sb.AppendFormat("\nScore: {}", ezArgF(sortedCandidate.m_fScore, 2));

    if (pGrid != nullptr)
    {
      const ezUInt32 uiNumObjectsMigrated = pGrid->m_uiLastMigrationIndex;
      sb.AppendFormat("\nMigrationStatus: {}%%\n", ezArgF(float(uiNumObjectsMigrated) / m_DataTable.GetCount() * 100.0f, 2));
    }
  }
}
#endif

EZ_ALWAYS_INLINE bool ezSpatialSystem_RegularGrid::IsAlwaysVisibleData(const Data& data) const
{
  return data.m_uiAlwaysVisible != 0;
}

ezSpatialDataHandle ezSpatialSystem_RegularGrid::AddSpatialDataToGrids(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags, bool bAlwaysVisible)
{
  Data data;
  data.m_uiGridBitmask = uiCategoryBitmask;
  data.m_uiAlwaysVisible = bAlwaysVisible ? 1 : 0;

  // find matching cached grids and add them to data.m_uiGridBitmask
  for (ezUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiCategoryBitmask) == 0 ||
        FilterByTags(tags, &pGrid->m_IncludeTags, &pGrid->m_ExcludeTags))
      continue;

    data.m_uiGridBitmask |= EZ_BIT(uiCachedGridIndex);
  }

  auto hData = ezSpatialDataHandle(m_DataTable.Insert(data));

  ezUInt64 uiGridBitmask = data.m_uiGridBitmask;
  while (uiGridBitmask > 0)
  {
    ezUInt32 uiGridIndex = ezMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
    {
      pGrid = EZ_NEW(&m_Allocator, Grid, *this, ezSpatialData::Category(static_cast<ezUInt16>(uiGridIndex)));
    }

    pGrid->AddSpatialData(bounds, tags, pObject, m_uiFrameCounter, hData);
  }

  return hData;
}

template <typename Functor>
EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::ForEachGrid(const Data& data, const ezSpatialDataHandle& hData, Functor func) const
{
  ezUInt64 uiGridBitmask = data.m_uiGridBitmask;
  ezUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

  while (uiGridBitmask > 0)
  {
    ezUInt32 uiGridIndex = ezMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& grid = *m_Grids[uiGridIndex];
    auto& mapping = grid.m_CellDataMappings[uiDataIndex];

    if (func(grid, mapping) == ezVisitorExecution::Stop)
      break;
  }
}

void ezSpatialSystem_RegularGrid::ForEachCellInBoxInMatchingGrids(const ezSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, ezVisibilityState::Enum visType) const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_uiTotalNumObjects = m_DataTable.GetCount();
  }
#endif

  ezUInt32 uiGridBitmask = queryParams.m_uiCategoryBitmask;

  // search for cached grids that match the exact query params first
  for (ezUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr || pGrid->CachingCompleted() == false)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiGridBitmask) == 0 ||
        AreTagSetsEqual(pGrid->m_IncludeTags, queryParams.m_pIncludeTags) == false ||
        AreTagSetsEqual(pGrid->m_ExcludeTags, queryParams.m_pExcludeTags) == false)
      continue;

    uiGridBitmask &= ~pGrid->m_Category.GetBitmask();

    Stats stats;
    pGrid->ForEachCellInBox(box,
      [&](const Cell& cell)
      {
        return noFilterCallback(cell, queryParams, stats, pUserData, visType);
      });

    UpdateCacheCandidate(queryParams.m_pIncludeTags, queryParams.m_pExcludeTags, pGrid->m_Category, 0.0f);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }

  // then search for the rest
  const bool useTagsFilter = (queryParams.m_pIncludeTags && queryParams.m_pIncludeTags->IsEmpty() == false) || (queryParams.m_pExcludeTags && queryParams.m_pExcludeTags->IsEmpty() == false);
  CellCallback cellCallback = useTagsFilter ? filterByTagsCallback : noFilterCallback;

  while (uiGridBitmask > 0)
  {
    ezUInt32 uiGridIndex = ezMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
      continue;

    Stats stats;
    pGrid->ForEachCellInBox(box,
      [&](const Cell& cell)
      {
        return cellCallback(cell, queryParams, stats, pUserData, visType);
      });

    if (pGrid->m_bCanBeCached && useTagsFilter)
    {
      const ezUInt32 totalNumObjectsAfterSpatialTest = stats.m_uiNumObjectsFiltered + stats.m_uiNumObjectsPassed;
      const ezUInt32 cacheThreshold = ezUInt32(ezMath::Max(cvar_SpatialQueriesCachingThreshold.GetValue(), 1));

      // 1.0 => all objects filtered, 0.0 => no object filtered by tags
      const float filteredRatio = float(double(stats.m_uiNumObjectsFiltered) / totalNumObjectsAfterSpatialTest);

      // Doesn't make sense to cache if there are only few objects in total or only few objects have been filtered
      if (totalNumObjectsAfterSpatialTest > cacheThreshold && filteredRatio > 0.1f)
      {
        UpdateCacheCandidate(queryParams.m_pIncludeTags, queryParams.m_pExcludeTags, pGrid->m_Category, filteredRatio);
      }
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }
}

void ezSpatialSystem_RegularGrid::MigrateCachedGrid(ezUInt32 uiCandidateIndex)
{
  ezUInt32 uiTargetGridIndex = ezInvalidIndex;
  ezUInt32 uiSourceGridIndex = ezInvalidIndex;

  {
    EZ_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiTargetGridIndex = cacheCandidate.m_uiGridIndex;
    uiSourceGridIndex = cacheCandidate.m_Category.m_uiValue;

    if (uiTargetGridIndex == ezInvalidIndex)
    {
      for (ezUInt32 i = m_Grids.GetCount() - 1; i >= MAX_NUM_REGULAR_GRIDS; --i)
      {
        if (m_Grids[i] == nullptr)
        {
          uiTargetGridIndex = i;
          break;
        }
      }

      EZ_ASSERT_DEBUG(uiTargetGridIndex != ezInvalidIndex, "No free cached grid");
      cacheCandidate.m_uiGridIndex = uiTargetGridIndex;

      auto pGrid = EZ_NEW(&m_Allocator, Grid, *this, cacheCandidate.m_Category);
      pGrid->m_IncludeTags = cacheCandidate.m_IncludeTags;
      pGrid->m_ExcludeTags = cacheCandidate.m_ExcludeTags;

      m_Grids[uiTargetGridIndex] = pGrid;

      m_uiFirstCachedGridIndex = ezMath::Min(m_uiFirstCachedGridIndex, uiTargetGridIndex);
    }
  }

  MigrateSpatialData(uiTargetGridIndex, uiSourceGridIndex);
}

void ezSpatialSystem_RegularGrid::MigrateSpatialData(ezUInt32 uiTargetGridIndex, ezUInt32 uiSourceGridIndex)
{
  auto& pTargetGrid = m_Grids[uiTargetGridIndex];
  if (pTargetGrid->CachingCompleted())
    return;

  auto& pSourceGrid = m_Grids[uiSourceGridIndex];

  constexpr ezUInt32 uiNumObjectsPerStep = 64;
  ezUInt32& uiLastMigrationIndex = pTargetGrid->m_uiLastMigrationIndex;
  const ezUInt32 uiSourceCount = pSourceGrid->m_CellDataMappings.GetCount();
  const ezUInt32 uiEndIndex = ezMath::Min(uiLastMigrationIndex + uiNumObjectsPerStep, uiSourceCount);

  for (ezUInt32 i = uiLastMigrationIndex; i < uiEndIndex; ++i)
  {
    if (pTargetGrid->MigrateSpatialDataFromOtherGrid(i, *pSourceGrid))
    {
      m_DataTable.GetValueUnchecked(i).m_uiGridBitmask |= EZ_BIT(uiTargetGridIndex);
    }
  }

  uiLastMigrationIndex = (uiEndIndex == uiSourceCount) ? ezInvalidIndex : uiEndIndex;
}

void ezSpatialSystem_RegularGrid::RemoveCachedGrid(ezUInt32 uiCandidateIndex)
{
  ezUInt32 uiGridIndex;

  {
    EZ_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiGridIndex = cacheCandidate.m_uiGridIndex;

    if (uiGridIndex == ezInvalidIndex)
      return;

    cacheCandidate.m_fQueryCount = 0.0f;
    cacheCandidate.m_fFilteredRatio = 0.0f;
    cacheCandidate.m_uiGridIndex = ezInvalidIndex;
  }

  m_Grids[uiGridIndex] = nullptr;
}

void ezSpatialSystem_RegularGrid::RemoveAllCachedGrids()
{
  EZ_LOCK(m_CacheCandidatesMutex);

  for (ezUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
  {
    RemoveCachedGrid(i);
  }
}

void ezSpatialSystem_RegularGrid::UpdateCacheCandidate(const ezTagSet* pIncludeTags, const ezTagSet* pExcludeTags, ezSpatialData::Category category, float filteredRatio) const
{
  EZ_LOCK(m_CacheCandidatesMutex);

  CacheCandidate* pCacheCandiate = nullptr;
  for (auto& cacheCandidate : m_CacheCandidates)
  {
    if (cacheCandidate.m_Category == category &&
        AreTagSetsEqual(cacheCandidate.m_IncludeTags, pIncludeTags) &&
        AreTagSetsEqual(cacheCandidate.m_ExcludeTags, pExcludeTags))
    {
      pCacheCandiate = &cacheCandidate;
      break;
    }
  }

  if (pCacheCandiate != nullptr)
  {
    pCacheCandiate->m_fQueryCount = ezMath::Min(pCacheCandiate->m_fQueryCount + 1.0f, 100.0f);
    pCacheCandiate->m_fFilteredRatio = ezMath::Max(pCacheCandiate->m_fFilteredRatio, filteredRatio);
  }
  else
  {
    auto& cacheCandidate = m_CacheCandidates.ExpandAndGetRef();
    cacheCandidate.m_Category = category;
    cacheCandidate.m_fQueryCount = 1;
    cacheCandidate.m_fFilteredRatio = filteredRatio;

    if (pIncludeTags != nullptr)
    {
      cacheCandidate.m_IncludeTags = *pIncludeTags;
    }
    if (pExcludeTags != nullptr)
    {
      cacheCandidate.m_ExcludeTags = *pExcludeTags;
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem_RegularGrid);
