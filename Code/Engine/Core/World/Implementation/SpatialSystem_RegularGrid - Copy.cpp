#include <CorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/SimdMath/SimdConversion.h>

#if 0



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
    , m_ObjectPointers(pAllocator)
    , m_DataPointers(pAllocator)
    , m_DataPointersToIndex(pAllocator)
  {
  }

  EZ_FORCE_INLINE void AddData(ezSpatialData* pData, ezUInt64 uiLastVisibleFrame, ezAllocatorBase* pAlignedAllocator)
  {
    ezUInt32 mask = pData->m_uiCategoryBitmask;
    ezUInt32 category = ezMath::FirstBitLow(mask);
    ezUInt32 highestCategory = ezMath::FirstBitHigh(mask);
    mask &= mask - 1;

    while (m_BoundingSpheres.GetCount() <= highestCategory)
    {
      m_BoundingSpheres.PushBack(ezDynamicArray<ezSimdBSphere>(pAlignedAllocator));
      m_ObjectPointers.PushBack(ezDynamicArray<ObjectPointerAndFrame>(m_DataPointers.GetAllocator()));
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
    m_ObjectPointers[category].PushBack({pData->m_pObject, uiLastVisibleFrame});
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
      m_ObjectPointers[category].PushBack({pData->m_pObject, uiLastVisibleFrame});
      m_DataPointers[category].PushBack(pData);
      m_DataPointersToIndex[category].Insert(pData, dataIndex);
    }

    m_uiCategoryBitmask |= pData->m_uiCategoryBitmask;
  }

  EZ_FORCE_INLINE void RemoveData(ezSpatialData* pData)
  {
    auto PatchMovedData = [&](ezSpatialData* pMovedData, ezUInt32 uiNewDataIndex, ezUInt32 category) {
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
    m_ObjectPointers[category].RemoveAtAndSwap(dataIndex);
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
      m_ObjectPointers[category].RemoveAtAndSwap(dataIndex);
      m_DataPointers[category].RemoveAtAndSwap(dataIndex);
    }
  }

  EZ_FORCE_INLINE void UpdateBounds(ezSpatialData* pData)
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

  EZ_FORCE_INLINE void UpdateObject(ezSpatialData* pData)
  {
    ezUInt32 mask = pData->m_uiCategoryBitmask;
    ezUInt32 category = ezMath::FirstBitLow(mask);
    mask &= mask - 1;

    auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);
    ezUInt32 dataIndex = pUserData->m_uiCachedDataIndex;
    EZ_ASSERT_DEBUG(pUserData->m_uiCachedCategory == category, "Implementation error");

    m_ObjectPointers[category][dataIndex].m_pObject = pData->m_pObject;

    while (mask > 0)
    {
      category = ezMath::FirstBitLow(mask);
      mask &= mask - 1;

      const bool found = m_DataPointersToIndex[category].TryGetValue(pData, dataIndex);
      EZ_ASSERT_DEBUG(found, "Implementation error");

      m_ObjectPointers[category][dataIndex].m_pObject = pData->m_pObject;
    }
  }

  ezSimdBBoxSphere m_Bounds;
  ezUInt32 m_uiCategoryBitmask = 0;

  struct ObjectPointerAndFrame
  {
    EZ_DECLARE_POD_TYPE();

    ezGameObject* m_pObject;
    ezUInt64 m_uiLastVisibleFrame;
  };

  ezHybridArray<ezDynamicArray<ezSimdBSphere>, 4> m_BoundingSpheres;
  ezHybridArray<ezDynamicArray<ObjectPointerAndFrame>, 4> m_ObjectPointers;
  ezHybridArray<ezDynamicArray<ezSpatialData*>, 4> m_DataPointers;
  ezHybridArray<ezHashTable<ezSpatialData*, ezUInt32>, 4> m_DataPointersToIndex;
};

//////////////////////////////////////////////////////////////////////////

struct ezSpatialSystem_RegularGrid::CellKeyHashHelper
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    // return ezUInt32(value * 2654435761U);
    return ezHashHelper<ezUInt64>::Hash(value);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezUInt64 a, ezUInt64 b) { return a == b; }
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

ezSpatialSystem_RegularGrid::~ezSpatialSystem_RegularGrid() {}

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

void ezSpatialSystem_RegularGrid::FindVisibleObjectsInternal(const ezFrustum& frustum, ezUInt32 uiCategoryBitmask, ezDynamicArray<const ezGameObject*>& out_Objects, QueryStats* pStats) const
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

  ForEachCellInBox(
    simdBox, uiCategoryBitmask, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, const Cell& cell, ezUInt32 uiFilteredCategoryBitmask) {
      ezSimdBSphere cellSphere = cell.m_Bounds.GetSphere();
      if (!SphereFrustumIntersect(cellSphere, planeData))
        return;

      ezUInt32 filteredMask = uiFilteredCategoryBitmask;
      while (filteredMask > 0)
      {
        ezUInt32 category = ezMath::FirstBitLow(filteredMask);
        filteredMask &= filteredMask - 1;

        auto boundingSpheres = cell.m_BoundingSpheres[category].GetData();
        auto objectPointers = cell.m_ObjectPointers[category].GetData();

        const ezUInt32 numSpheres = cell.m_BoundingSpheres[category].GetCount();

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

              auto& objectAndFrame = objectPointers[currentIndex + i];
              const_cast<Cell::ObjectPointerAndFrame&>(objectAndFrame).m_uiLastVisibleFrame = m_uiFrameCounter;
              out_Objects.PushBack(objectAndFrame.m_pObject);

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

            auto& objectAndFrame = objectPointers[i];
            const_cast<Cell::ObjectPointerAndFrame&>(objectAndFrame).m_uiLastVisibleFrame = m_uiFrameCounter;
            out_Objects.PushBack(objectAndFrame.m_pObject);

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

ezUInt64 ezSpatialSystem_RegularGrid::GetLastFrameVisibleInternal(ezSpatialData* pData) const
{
  auto pUserData = reinterpret_cast<const SpatialUserData*>(&pData->m_uiUserData[0]);
  if (auto pCell = pUserData->m_pCell)
  {
    return pCell->GetLastFrameVisible(pData);
  }

  return -1;
}

void ezSpatialSystem_RegularGrid::SpatialDataAdded(ezSpatialData* pData)
{
  Cell* pCell = GetOrCreateCell(pData->m_Bounds);
  pCell->AddData(pData, m_uiFrameCounter, &m_AlignedAllocator);
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
      pOldCell->UpdateBounds(pData);
    }
    else
    {
      Cell* pNewCell = GetOrCreateCell(pData->m_Bounds);
      if (pOldCell == pNewCell)
      {
        pOldCell->UpdateBounds(pData);
      }
      else
      {
        ezUInt64 uiLastVisibleFrame = pOldCell->GetLastFrameVisible(pData);
        pOldCell->RemoveData(pData);
        pNewCell->AddData(pData, uiLastVisibleFrame, &m_AlignedAllocator);
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

void ezSpatialSystem_RegularGrid::SpatialDataObjectChanged(ezSpatialData* pData)
{
  auto pUserData = reinterpret_cast<SpatialUserData*>(&pData->m_uiUserData[0]);
  pUserData->m_pCell->UpdateObject(pData);
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




#endif
