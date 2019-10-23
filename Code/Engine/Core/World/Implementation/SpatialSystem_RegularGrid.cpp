#include <CorePCH.h>

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

struct ezSpatialSystem_RegularGrid::SpatialUserData
{
  Cell* m_pCell = nullptr;
  ezUInt32 m_uiCachedDataIndex = 0;
  ezUInt32 m_uiCachedCategory = 0;
};

//////////////////////////////////////////////////////////////////////////

struct ezSpatialSystem_RegularGrid::Cell
{
  Cell(ezAllocatorBase* pAllocator)
    : m_BoundingSpheres(pAllocator)
    , m_DataPointers(pAllocator)
    , m_DataPointersToIndex(pAllocator)
  {
  }

  EZ_FORCE_INLINE void AddData(ezSpatialData* pData, ezAllocatorBase* pAlignedAllocator)
  {
    ezUInt32 mask = pData->m_uiCategoryBitmask;
    ezUInt32 category = ezMath::FirstBitLow(mask);
    ezUInt32 highestCategory = ezMath::FirstBitHigh(mask);
    mask &= mask - 1;

    while (m_BoundingSpheres.GetCount() <= highestCategory)
    {
      m_BoundingSpheres.PushBack(ezDynamicArray<ezSimdBSphere>(pAlignedAllocator));
      m_DataPointers.PushBack(ezDynamicArray<ezSpatialData*>(m_DataPointers.GetAllocator()));
    }

    if (mask > 0)
    {
      while (m_DataPointersToIndex.GetCount() <= highestCategory)
      {
        m_DataPointersToIndex.PushBack(ezHashTable<ezSpatialData*, ezUInt32>(m_DataPointers.GetAllocator()));
      }
    }

    ezUInt32 dataIndex = m_BoundingSpheres[category].GetCount();
    m_BoundingSpheres[category].PushBack(pData->m_Bounds.GetSphere());
    m_DataPointers[category].PushBack(pData);

    auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);

    EZ_ASSERT_DEBUG(pUserData->m_pCell == nullptr, "Data can't be in multiple cells");
    pUserData->m_pCell = this;
    pUserData->m_uiCachedDataIndex = dataIndex;
    pUserData->m_uiCachedCategory = category;

    while (mask > 0)
    {
      category = ezMath::FirstBitLow(mask);
      mask &= mask - 1;

      dataIndex = m_BoundingSpheres[category].GetCount();
      m_BoundingSpheres[category].PushBack(pData->m_Bounds.GetSphere());
      m_DataPointers[category].PushBack(pData);
      m_DataPointersToIndex[category].Insert(pData, dataIndex);
    }

    m_uiCategoryBitmask |= pData->m_uiCategoryBitmask;
  }

  EZ_FORCE_INLINE void RemoveData(ezSpatialData* pData)
  {
    auto PatchMovedData = [&](ezSpatialData* pMovedData, ezUInt32 uiNewDataIndex, ezUInt32 category)
    {
      auto pMovedUserData = reinterpret_cast<SpatialUserData*>(&pMovedData->m_uiUserData[0]);
      if (pMovedUserData->m_uiCachedCategory == category)
      {
        pMovedUserData->m_uiCachedDataIndex = uiNewDataIndex;
      }
      else
      {
        const bool replaced = m_DataPointersToIndex[category].Insert(pMovedData, uiNewDataIndex);
        EZ_ASSERT_DEBUG(replaced, "Implementation error");
      }
    };

    ezUInt32 mask = pData->m_uiCategoryBitmask;
    ezUInt32 category = ezMath::FirstBitLow(mask);
    mask &= mask - 1;

    auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);
    ezUInt32 dataIndex = pUserData->m_uiCachedDataIndex;

    if (dataIndex != m_DataPointers[category].GetCount() - 1)
    {
      ezSpatialData* pLastData = m_DataPointers[category].PeekBack();
      PatchMovedData(pLastData, dataIndex, category);
    }

    m_BoundingSpheres[category].RemoveAtAndSwap(dataIndex);
    m_DataPointers[category].RemoveAtAndSwap(dataIndex);

    EZ_ASSERT_DEBUG(pUserData->m_pCell == this, "Implementation error");
    pUserData->m_pCell = nullptr;
    pUserData->m_uiCachedDataIndex = ezInvalidIndex;
    pUserData->m_uiCachedCategory = ezInvalidIndex;

    while (mask > 0)
    {
      category = ezMath::FirstBitLow(mask);
      mask &= mask - 1;

      const bool found = m_DataPointersToIndex[category].TryGetValue(pData, dataIndex);
      EZ_ASSERT_DEBUG(found, "Implementation error");

      if (dataIndex != m_DataPointers[category].GetCount() - 1)
      {
        ezSpatialData* pLastData = m_DataPointers[category].PeekBack();
        PatchMovedData(pLastData, dataIndex, category);
      }

      m_BoundingSpheres[category].RemoveAtAndSwap(dataIndex);
      m_DataPointers[category].RemoveAtAndSwap(dataIndex);
    }
  }

  EZ_FORCE_INLINE void UpdateData(ezSpatialData* pData)
  {
    ezUInt32 mask = pData->m_uiCategoryBitmask;
    ezUInt32 category = ezMath::FirstBitLow(mask);
    mask &= mask - 1;

    auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);
    ezUInt32 dataIndex = pUserData->m_uiCachedDataIndex;
    EZ_ASSERT_DEBUG(pUserData->m_uiCachedCategory == category, "Implementation error");

    m_BoundingSpheres[category][dataIndex] = pData->m_Bounds.GetSphere();

    while (mask > 0)
    {
      category = ezMath::FirstBitLow(mask);
      mask &= mask - 1;

      const bool found = m_DataPointersToIndex[category].TryGetValue(pData, dataIndex);
      EZ_ASSERT_DEBUG(found, "Implementation error");

      m_BoundingSpheres[category][dataIndex] = pData->m_Bounds.GetSphere();
    }
  }

  EZ_ALWAYS_INLINE ezBoundingBox GetBoundingBox() const
  {
    return ezSimdConversion::ToBBoxSphere(m_Bounds).GetBox();
  }

  ezSimdBBoxSphere m_Bounds;
  ezUInt32 m_uiCategoryBitmask = 0;

  ezHybridArray<ezDynamicArray<ezSimdBSphere>, 4> m_BoundingSpheres;
  ezHybridArray<ezDynamicArray<ezSpatialData*>, 4> m_DataPointers;
  ezHybridArray<ezHashTable<ezSpatialData*, ezUInt32>, 4> m_DataPointersToIndex;
};

//////////////////////////////////////////////////////////////////////////

struct ezSpatialSystem_RegularGrid::CellKeyHashHelper
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    //return ezUInt32(value * 2654435761U);
    return ezHashHelper<ezUInt64>::Hash(value);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezUInt64 a, ezUInt64 b)
  {
    return a == b;
  }
};

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem_RegularGrid, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSpatialSystem_RegularGrid::ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize /* = 128 */)
  : m_AlignedAllocator("Spatial System Aligned", ezFoundation::GetAlignedAllocator())
  , m_iCellSize(uiCellSize)
  , m_fOverlapSize(uiCellSize / 4.0f)
  , m_fInvCellSize(1.0f / uiCellSize)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(ezSpatialSystem_RegularGrid::SpatialUserData) <= sizeof(ezSpatialData::m_uiUserData));

  ezSimdBBox overflowBox;
  overflowBox.SetCenterAndHalfExtents(ezSimdVec4f::ZeroVector(), ezSimdVec4f((float)(m_iCellSize.x() * MAX_CELL_INDEX)));

  m_pOverflowCell = EZ_NEW(&m_AlignedAllocator, Cell, &m_Allocator);
  m_pOverflowCell->m_Bounds = overflowBox;
}

ezSpatialSystem_RegularGrid::~ezSpatialSystem_RegularGrid()
{
}

ezResult ezSpatialSystem_RegularGrid::GetCellBoxForSpatialData(const ezSpatialDataHandle& hData, ezBoundingBox& out_BoundingBox) const
{
  ezSpatialData* pData;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return EZ_FAILURE;

  auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);
  if (pUserData->m_pCell != nullptr)
  {
    out_BoundingBox = pUserData->m_pCell->GetBoundingBox();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezSpatialSystem_RegularGrid::GetAllCellBoxes(ezHybridArray<ezBoundingBox, 16>& out_BoundingBoxes, ezSpatialData::Category filterCategory) const
{
  for (auto it = m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    const Cell& cell = *it.Value();
    if (filterCategory == ezInvalidSpatialDataCategory || (cell.m_uiCategoryBitmask & filterCategory.GetBitmask()) != 0)
    {
      out_BoundingBoxes.ExpandAndGetRef() = cell.GetBoundingBox();
    }
  }
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphereInternal(const ezBoundingSphere& sphere, ezUInt32 uiCategoryBitmask, QueryCallback callback,
  QueryStats* pStats) const
{
  ezSimdBSphere simdSphere(ezSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);
  ezSimdBBox simdBox;
  simdBox.SetCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<ezSwizzle::WWWW>());

  ForEachCellInBox(simdBox, uiCategoryBitmask, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, const Cell& cell, ezUInt32 uiFilteredCategoryBitmask) {
    ezSimdBBox cellBox = cell.m_Bounds.GetBox();
    if (!cellBox.Overlaps(simdSphere))
      return;

    ezUInt32 mask = uiFilteredCategoryBitmask;
    while (mask > 0)
    {
      ezUInt32 category = ezMath::FirstBitLow(mask);
      mask &= mask - 1;

      auto& boundingSpheres = cell.m_BoundingSpheres[category];
      auto& dataPointers = cell.m_DataPointers[category];

      const ezUInt32 numSpheres = boundingSpheres.GetCount();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (pStats != nullptr)
      {
        pStats->m_uiNumObjectsTested += numSpheres;
      }
#endif

      for (ezUInt32 i = 0; i < numSpheres; ++i)
      {
        auto& objectSphere = boundingSpheres[i];
        if (!simdSphere.Overlaps(objectSphere))
          continue;

        const ezSpatialData* pData = dataPointers[i];

        // TODO: The return value has to have more control
        if (callback(pData->m_pObject) == ezVisitorExecution::Stop)
          return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
        if (pStats != nullptr)
        {
          pStats->m_uiNumObjectsPassed++;
        }
#endif
      }
    }
  });
}

void ezSpatialSystem_RegularGrid::FindObjectsInBoxInternal(const ezBoundingBox& box, ezUInt32 uiCategoryBitmask, QueryCallback callback, QueryStats* pStats) const
{
  ezSimdBBox simdBox(ezSimdConversion::ToVec3(box.m_vMin), ezSimdConversion::ToVec3(box.m_vMax));

  ForEachCellInBox(simdBox, uiCategoryBitmask, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, const Cell& cell, ezUInt32 uiFilteredCategoryBitmask) {
    ezUInt32 mask = uiFilteredCategoryBitmask;
    while (mask > 0)
    {
      ezUInt32 category = ezMath::FirstBitLow(mask);
      mask &= mask - 1;

      auto& boundingSpheres = cell.m_BoundingSpheres[category];
      auto& dataPointers = cell.m_DataPointers[category];

      const ezUInt32 numSpheres = boundingSpheres.GetCount();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (pStats != nullptr)
      {
        pStats->m_uiNumObjectsTested += numSpheres;
      }
#endif

      for (ezUInt32 i = 0; i < numSpheres; ++i)
      {
        auto& objectSphere = boundingSpheres[i];
        if (!simdBox.Overlaps(objectSphere))
          continue;

        const ezSpatialData* pData = dataPointers[i];
        if (!simdBox.Overlaps(pData->m_Bounds.GetBox()))
          continue;

        // TODO: The return value has to have more control
        if (callback(pData->m_pObject) == ezVisitorExecution::Stop)
          return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
        if (pStats != nullptr)
        {
          pStats->m_uiNumObjectsPassed++;
        }
#endif
      }
    }
  });
}

void ezSpatialSystem_RegularGrid::FindVisibleObjectsInternal(const ezFrustum& frustum, ezUInt32 uiCategoryBitmask, ezDynamicArray<const ezGameObject*>& out_Objects,
  QueryStats* pStats) const
{
  ezVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints);

  ezSimdVec4f simdCornerPoints[8];
  for (ezUInt32 i = 0; i < 8; ++i)
  {
    simdCornerPoints[i] = ezSimdConversion::ToVec3(cornerPoints[i]);
  }

  ezSimdBBox simdBox;
  simdBox.SetFromPoints(simdCornerPoints, 8);

  PlaneData planeData;
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

    planeData.m_x0x1x2x3 = helperMat.m_col0;
    planeData.m_y0y1y2y3 = helperMat.m_col1;
    planeData.m_z0z1z2z3 = helperMat.m_col2;
    planeData.m_w0w1w2w3 = helperMat.m_col3;

    helperMat.SetRows(plane4, plane5, plane4, plane5);

    planeData.m_x4x5x4x5 = helperMat.m_col0;
    planeData.m_y4y5y4y5 = helperMat.m_col1;
    planeData.m_z4z5z4z5 = helperMat.m_col2;
    planeData.m_w4w5w4w5 = helperMat.m_col3;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezUInt32 uiNumObjectsTested = 0;
  ezUInt32 uiNumObjectsPassed = 0;
#endif

  ForEachCellInBox(simdBox, uiCategoryBitmask, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, const Cell& cell, ezUInt32 uiFilteredCategoryBitmask) {
    ezSimdBSphere cellSphere = cell.m_Bounds.GetSphere();
    if (!SphereFrustumIntersect(cellSphere, planeData))
      return;

    ezUInt32 filteredMask = uiFilteredCategoryBitmask;
    while (filteredMask > 0)
    {
      ezUInt32 category = ezMath::FirstBitLow(filteredMask);
      filteredMask &= filteredMask - 1;

      auto& boundingSpheres = cell.m_BoundingSpheres[category];
      auto& dataPointers = cell.m_DataPointers[category];

      const ezUInt32 numSpheres = boundingSpheres.GetCount();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      uiNumObjectsTested += numSpheres;
#endif
      ezUInt32 currentIndex = 0;

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
            ezUInt32 i = ezMath::FirstBitLow(mask);
            mask &= mask - 1;

            ezSpatialData* pData = dataPointers[currentIndex + i];
            out_Objects.PushBack(pData->m_pObject);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
            uiNumObjectsPassed++;
#endif
          }

          currentIndex += 32;
        }
        else
        {
          ezUInt32 i = currentIndex;
          ++currentIndex;

          auto& objectSphere = boundingSpheres[i];
          if (!SphereFrustumIntersect(objectSphere, planeData))
            continue;

          ezSpatialData* pData = dataPointers[i];
          out_Objects.PushBack(pData->m_pObject);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
          uiNumObjectsPassed++;
#endif
        }
      }
    }
  });

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pStats != nullptr)
  {
    pStats->m_uiNumObjectsTested = uiNumObjectsTested;
    pStats->m_uiNumObjectsPassed = uiNumObjectsPassed;
  }
#endif
}

void ezSpatialSystem_RegularGrid::SpatialDataAdded(ezSpatialData* pData)
{
  Cell* pCell = GetOrCreateCell(pData->m_Bounds);
  pCell->AddData(pData, &m_AlignedAllocator);
}

void ezSpatialSystem_RegularGrid::SpatialDataRemoved(ezSpatialData* pData)
{
  auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);
  if (pUserData->m_pCell != nullptr)
  {
    pUserData->m_pCell->RemoveData(pData);
  }
}

void ezSpatialSystem_RegularGrid::SpatialDataChanged(ezSpatialData* pData, const ezSimdBBoxSphere& oldBounds, ezUInt32 uiOldCategoryBitmask)
{
  if (pData->m_uiCategoryBitmask == uiOldCategoryBitmask)
  {
    auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);

    Cell* pOldCell = pUserData->m_pCell;
    if (pOldCell->m_Bounds.GetBox().Contains(pData->m_Bounds.GetBox()))
    {
      pOldCell->UpdateData(pData);
    }
    else
    {
      Cell* pNewCell = GetOrCreateCell(pData->m_Bounds);
      if (pOldCell == pNewCell)
      {
        pOldCell->UpdateData(pData);
      }
      else
      {
        pOldCell->RemoveData(pData);
        pNewCell->AddData(pData, &m_AlignedAllocator);
      }
    }
  }
  else
  {
    // Restore old bitmask so SpatialDataRemoved works correctly.
    ezUInt32 uiNewCategoryBitmask = pData->m_uiCategoryBitmask;
    pData->m_uiCategoryBitmask = uiOldCategoryBitmask;

    SpatialDataRemoved(pData);

    pData->m_uiCategoryBitmask = uiNewCategoryBitmask;

    if (pData->m_uiCategoryBitmask != 0)
    {
      SpatialDataAdded(pData);
    }
  }
}

void ezSpatialSystem_RegularGrid::FixSpatialDataPointer(ezSpatialData* pOldPtr, ezSpatialData* pNewPtr)
{
  auto pUserData = reinterpret_cast<SpatialUserData*>(&pNewPtr->m_uiUserData[0]);
  Cell* pCell = pUserData->m_pCell;

  ezUInt32 mask = pNewPtr->m_uiCategoryBitmask;
  while (mask > 0)
  {
    ezUInt32 category = ezMath::FirstBitLow(mask);
    mask &= mask - 1;

    if (pUserData->m_uiCachedCategory == category)
    {
      ezUInt32 dataIndex = pUserData->m_uiCachedDataIndex;

      pCell->m_DataPointers[category][dataIndex] = pNewPtr;
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

template <typename Functor>
EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::ForEachCellInBox(const ezSimdBBox& box, ezUInt32 uiCategoryBitmask, Functor func) const
{
  ezSimdVec4i minIndex = ToVec3I32((box.m_Min - m_fOverlapSize) * m_fInvCellSize);
  ezSimdVec4i maxIndex = ToVec3I32((box.m_Max + m_fOverlapSize) * m_fInvCellSize);

  EZ_ASSERT_DEBUG((minIndex.Abs() < ezSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");
  EZ_ASSERT_DEBUG((maxIndex.Abs() < ezSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");

  const ezInt32 iMinX = minIndex.x();
  const ezInt32 iMinY = minIndex.y();
  const ezInt32 iMinZ = minIndex.z();

#if 0

  const ezInt32 iMaxX = maxIndex.x();
  const ezInt32 iMaxY = maxIndex.y();
  const ezInt32 iMaxZ = maxIndex.z();

  for (ezInt32 z = iMinZ; z <= iMaxZ; ++z)
  {
    for (ezInt32 y = iMinY; y <= iMaxY; ++y)
    {
      for (ezInt32 x = iMinX; x <= iMaxX; ++x)
      {
        ezUInt64 cellKey = GetCellKey(x, y, z);

        Cell* pCell = nullptr;
        m_Cells.TryGetValue(cellKey, pCell);

        ezSimdVec4i cellIndex(x, y, z);
        func(cellIndex, cellKey, pCell);
      }
    }
  }

#else

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

    if (auto ppCell = m_Cells.GetValue(cellKey))
    {
      const Cell& constCell = *(*ppCell);
      ezUInt32 uiFilteredCategoryBitmask = constCell.m_uiCategoryBitmask & uiCategoryBitmask;
      if (uiFilteredCategoryBitmask != 0)
      {
        ezSimdVec4i cellIndex(x, y, z);
        func(cellIndex, cellKey, constCell, uiFilteredCategoryBitmask);
      }
    }
  }

  ezUInt32 uiFilteredCategoryBitmask = m_pOverflowCell->m_uiCategoryBitmask & uiCategoryBitmask;
  if (uiFilteredCategoryBitmask != 0)
  {
    func(ezSimdVec4i::ZeroVector(), 0, *(m_pOverflowCell), uiFilteredCategoryBitmask);
  }
#endif
}

ezSpatialSystem_RegularGrid::Cell* ezSpatialSystem_RegularGrid::GetOrCreateCell(const ezSimdBBoxSphere& bounds)
{
  ezSimdVec4i cellIndex = ToVec3I32(bounds.m_CenterAndRadius * m_fInvCellSize);
  ezSimdBBox cellBox = ComputeCellBoundingBox(cellIndex, m_iCellSize);

  if (cellBox.Contains(bounds.GetBox()))
  {
    ezUInt64 cellKey = GetCellKey(cellIndex.x(), cellIndex.y(), cellIndex.z());

    if (auto ppCell = m_Cells.GetValue(cellKey))
    {
      return ppCell->Borrow();
    }

    ezUniquePtr<Cell> pNewCell = EZ_NEW(&m_AlignedAllocator, Cell, &m_Allocator);
    pNewCell->m_Bounds = cellBox;

    Cell* pCell = pNewCell.Borrow();
    m_Cells.Insert(cellKey, std::move(pNewCell));

    return pCell;
  }
  else
  {
    return m_pOverflowCell.Borrow();
  }
}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem_RegularGrid);
