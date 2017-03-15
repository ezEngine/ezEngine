#include <PCH.h>
#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Containers/HashSet.h>

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

  EZ_FORCE_INLINE ezBoundingBox GetBoundingBox(const ezSimdVec4i& cellIndex, const ezSimdVec4i& iCellSize)
  {
    ezSimdVec4i minPos = cellIndex.CompMul(iCellSize);
    ezSimdVec4f bmin = ToVec3(minPos);
    ezSimdVec4f bmax = ToVec3(minPos + iCellSize);

    ezBoundingBox box;
    bmin.Store<3>(&box.m_vMin.x);
    bmax.Store<3>(&box.m_vMax.x);

    return box;
  }

  EZ_ALWAYS_INLINE ezSimdBBox GetBoundingBoxSimd(const ezSimdVec4i& cellIndex, const ezSimdVec4i& iCellSize)
  {
    ezSimdVec4i minPos = cellIndex.CompMul(iCellSize);
    ezSimdVec4f bmin = ToVec3(minPos);
    ezSimdVec4f bmax = ToVec3(minPos + iCellSize);

    return ezSimdBBox(bmin, bmax);
  }

  static ezThreadLocalPointer<ezHashSet<ezSpatialData*>> s_pDeduplicationSet;

  EZ_FORCE_INLINE ezHashSet<ezSpatialData*>* ClearAndGetDeduplicationSet()
  {
    ezHashSet<ezSpatialData*>* pSet = s_pDeduplicationSet;

    if (pSet == nullptr)
    {
      // Force to static allocator so we don't report a memory leak for this allocation.
      pSet = EZ_NEW(ezStaticAllocatorWrapper::GetAllocator(), ezHashSet<ezSpatialData*>);
      s_pDeduplicationSet = pSet;
    }
    else
    {
      pSet->Clear();
    }

    return pSet;
  }
}

//////////////////////////////////////////////////////////////////////////

ezSpatialSystem_RegularGrid::Cell::Cell(const ezSimdVec4i& index, ezAllocatorBase* pAllocator, ezAllocatorBase* pAlignedAllocator)
  : m_Index(index)
  , m_BoundingSpheres(pAlignedAllocator)
  , m_DataPointers(pAllocator)
  , m_PointerToIndexTable(pAllocator)
{

}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::AddData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = m_BoundingSpheres.GetCount();
  m_BoundingSpheres.PushBack(pData->m_Bounds.GetSphere());
  m_DataPointers.PushBack(pData);
  EZ_VERIFY(!m_PointerToIndexTable.Insert(pData, dataIndex), "Implementation error");

  pData->m_uiRefCount++;
}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::RemoveData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = 0;
  EZ_VERIFY(m_PointerToIndexTable.Remove(pData, &dataIndex), "Implementation error");

  if (dataIndex != m_DataPointers.GetCount() - 1)
  {
    ezSpatialData* pLastData = m_DataPointers.PeekBack();
    m_PointerToIndexTable[pLastData] = dataIndex;
  }

  m_BoundingSpheres.RemoveAtSwap(dataIndex);
  m_DataPointers.RemoveAtSwap(dataIndex);

  pData->m_uiRefCount--;
}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::UpdateData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = 0;
  EZ_VERIFY(m_PointerToIndexTable.TryGetValue(pData, dataIndex), "Implementation error");

  m_BoundingSpheres[dataIndex] = pData->m_Bounds.GetSphere();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem_RegularGrid, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSpatialSystem_RegularGrid::ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize /* = 128 */)
  : m_AlignedAllocator("Spatial System Aligned", ezFoundation::GetAlignedAllocator())
  , m_iCellSize(uiCellSize)
  , m_fInvCellSize(1.0f / uiCellSize)
{

}

ezSpatialSystem_RegularGrid::~ezSpatialSystem_RegularGrid()
{
  for (auto it = m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    Cell* pCell = it.Value();
    EZ_DELETE(&m_AlignedAllocator, pCell);
  }
}

void ezSpatialSystem_RegularGrid::GetCellBoxesForSpatialData(const ezSpatialDataHandle& hData, ezHybridArray<ezBoundingBox, 16>& out_BoundingBoxes) const
{
  ezSpatialData* pData;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return;

  for (auto it = m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    Cell& cell = *it.Value();
    if (cell.m_PointerToIndexTable.Contains(pData))
    {
      out_BoundingBoxes.ExpandAndGetRef() = GetBoundingBox(cell.m_Index, m_iCellSize);
    }
  }
}

void ezSpatialSystem_RegularGrid::GetAllCellBoxes(ezHybridArray<ezBoundingBox, 16>& out_BoundingBoxes) const
{
  for (auto it = m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    Cell& cell = *it.Value();
    out_BoundingBoxes.ExpandAndGetRef() = GetBoundingBox(cell.m_Index, m_iCellSize);
  }
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, ezDynamicArray<ezGameObject*>& out_Objects) const
{
  FindObjectsInSphere(sphere, [&](ezGameObject* pObject)
  {
    out_Objects.PushBack(pObject);

    return ezVisitorExecution::Continue;
  });
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, QueryCallback callback) const
{
  ezSimdBSphere simdSphere(ezSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);
  ezSimdBBox simdBox;
  simdBox.SetCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<ezSwizzle::WWWW>());

  auto pSet = ClearAndGetDeduplicationSet();

  ForEachCellInBox(simdBox, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    if (pCell != nullptr)
    {
      ezSimdBBox cellBox = GetBoundingBoxSimd(cellIndex, m_iCellSize);
      if (cellBox.Overlaps(simdSphere))
      {
        const ezUInt32 numSpheres = pCell->m_BoundingSpheres.GetCount();
        for (ezUInt32 i = 0; i < numSpheres; ++i)
        {
          auto& objectSphere = pCell->m_BoundingSpheres[i];
          if (!simdSphere.Overlaps(objectSphere))
            continue;

          ezSpatialData* pData = pCell->m_DataPointers[i];
          if (pData->m_uiRefCount > 1)
          {
            if (pSet->Insert(pData))
              continue;
          }

          callback(pCell->m_DataPointers[i]->m_pObject);
        }
      }
    }
  });
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, ezDynamicArray<ezGameObject*>& out_Objects) const
{
  FindObjectsInBox(box, [&](ezGameObject* pObject)
  {
    out_Objects.PushBack(pObject);

    return ezVisitorExecution::Continue;
  });
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, QueryCallback callback) const
{
  ezSimdBBox simdBox(ezSimdConversion::ToVec3(box.m_vMin), ezSimdConversion::ToVec3(box.m_vMax));

  auto pSet = ClearAndGetDeduplicationSet();

  ForEachCellInBox(simdBox, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    if (pCell != nullptr)
    {
      ezSimdBBox cellBox = GetBoundingBoxSimd(cellIndex, m_iCellSize);
      if (cellBox.Overlaps(simdBox))
      {
        const ezUInt32 numSpheres = pCell->m_BoundingSpheres.GetCount();
        for (ezUInt32 i = 0; i < numSpheres; ++i)
        {
          auto& objectSphere = pCell->m_BoundingSpheres[i];
          if (!simdBox.Overlaps(objectSphere))
            continue;

          ezSpatialData* pData = pCell->m_DataPointers[i];
          if (!simdBox.Overlaps(pData->m_Bounds.GetBox()))
            continue;

          if (pData->m_uiRefCount > 1)
          {
            if (pSet->Insert(pData))
              continue;
          }

          callback(pCell->m_DataPointers[i]->m_pObject);
        }
      }
    }
  });
}

void ezSpatialSystem_RegularGrid::FindVisibleObjects(const ezFrustum& frustum, ezDynamicArray<const ezGameObject*>& out_Objects) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::SpatialDataAdded(ezSpatialData* pData)
{
  ForEachCellInBox(pData->m_Bounds.GetBox(), [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    if (pCell == nullptr)
    {
      pCell = EZ_NEW(&m_AlignedAllocator, Cell, cellIndex, &m_Allocator, &m_AlignedAllocator);

      m_Cells.Insert(cellKey, pCell);
    }

    pCell->AddData(pData);
  });
}

void ezSpatialSystem_RegularGrid::SpatialDataRemoved(ezSpatialData* pData)
{
  ForEachCellInBox(pData->m_Bounds.GetBox(), [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    EZ_ASSERT_DEBUG(pCell != nullptr, "Implementation error");
    pCell->RemoveData(pData);
  });
}

void ezSpatialSystem_RegularGrid::SpatialDataChanged(ezSpatialData* pData, const ezSimdBBoxSphere& oldBounds)
{
  ezSimdBBox oldBox = oldBounds.GetBox();
  ezSimdBBox newBox = pData->m_Bounds.GetBox();

  ezSimdVec4i oldIndexBoxMin = ToVec3I32(oldBox.m_Min * m_fInvCellSize);
  ezSimdVec4i oldIndexBoxMax = ToVec3I32(oldBox.m_Max * m_fInvCellSize);

  ezSimdVec4i newIndexBoxMin = ToVec3I32(newBox.m_Min * m_fInvCellSize);
  ezSimdVec4i newIndexBoxMax = ToVec3I32(newBox.m_Max * m_fInvCellSize);

  ezSimdBBox combinedBox = oldBox;
  combinedBox.ExpandToInclude(newBox);

  ForEachCellInBox(combinedBox, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    const bool bContainsOld = (cellIndex >= oldIndexBoxMin && cellIndex <= oldIndexBoxMax).AllSet<3>();
    const bool bContainsNew = (cellIndex >= newIndexBoxMin && cellIndex <= newIndexBoxMax).AllSet<3>();

    if (bContainsOld)
    {
      if (bContainsNew)
      {
        pCell->UpdateData(pData);
      }
      else
      {
        pCell->RemoveData(pData);
      }
    }
    else
    {
      if (bContainsNew)
      {
        if (pCell == nullptr)
        {
          pCell = EZ_NEW(&m_AlignedAllocator, Cell, cellIndex, &m_Allocator, &m_AlignedAllocator);

          m_Cells.Insert(cellKey, pCell);
        }

        pCell->AddData(pData);
      }
    }
  });
}

void ezSpatialSystem_RegularGrid::FixSpatialDataPointer(ezSpatialData* pOldPtr, ezSpatialData* pNewPtr)
{
  ForEachCellInBox(pNewPtr->m_Bounds.GetBox(), [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    if (pCell != nullptr)
    {
      ezUInt32 dataIndex = 0;
      EZ_VERIFY(pCell->m_PointerToIndexTable.Remove(pOldPtr, &dataIndex), "Implementation error");

      pCell->m_DataPointers[dataIndex] = pNewPtr;
      pCell->m_PointerToIndexTable.Insert(pNewPtr, dataIndex);
    }
  });
}

template <typename Functor>
EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::ForEachCellInBox(const ezSimdBBox& box, Functor func) const
{
  ezSimdVec4i minIndex = ToVec3I32(box.m_Min * m_fInvCellSize);
  ezSimdVec4i maxIndex = ToVec3I32(box.m_Max * m_fInvCellSize);

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

    Cell* pCell = nullptr;
    m_Cells.TryGetValue(cellKey, pCell);

    ezSimdVec4i cellIndex(x, y, z);
    func(cellIndex, cellKey, pCell);
  }

#endif
}
