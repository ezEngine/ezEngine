#include <Core/PCH.h>
#include <Core/World/SpatialSystem_RegularGrid.h>

namespace
{
  enum
  {
    MAX_CELL_INDEX = (1 << 20) - 1,
    CELL_INDEX_MASK = (1 << 21) - 1
  };

  EZ_FORCE_INLINE ezVec3 ToVec3(const ezVec3I32& v)
  {
    return ezVec3((float)v.x, (float)v.y, (float)v.z);
  }

  EZ_FORCE_INLINE ezVec3I32 ToVec3I32(const ezVec3& v)
  {
    float x = ezMath::Floor(v.x);
    float y = ezMath::Floor(v.y);
    float z = ezMath::Floor(v.z);

    return ezVec3I32((ezInt32)x, (ezInt32)y, (ezInt32)z);
  }

  EZ_FORCE_INLINE ezUInt64 GetCellKey(const ezVec3I32& cellIndex)
  {
    ezUInt64 x = (cellIndex.x + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    ezUInt64 y = (cellIndex.y + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    ezUInt64 z = (cellIndex.z + MAX_CELL_INDEX) & CELL_INDEX_MASK;

    return (x << 42) | (y << 21) | z;
  }

  EZ_FORCE_INLINE ezBoundingBox GetBoundingBox(const ezVec3I32& cellIndex, ezInt32 iCellSize)
  {
    return ezBoundingBox(ToVec3(cellIndex * iCellSize), ToVec3((cellIndex + ezVec3I32(1)) * iCellSize));
  }
}

//////////////////////////////////////////////////////////////////////////

ezSpatialSystem_RegularGrid::Cell::Cell(const ezVec3I32& index, ezAllocatorBase* pAllocator, ezAllocatorBase* pAlignedAllocator)
  : m_Index(index)
  , m_BoundingSpheres(pAlignedAllocator)
  , m_DataPointers(pAllocator)
  , m_PointerToIndexTable(pAllocator)
{

}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::AddData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = m_BoundingSpheres.GetCount();
  m_BoundingSpheres.PushBack(pData->m_Bounds.m_vCenter.GetAsVec4(pData->m_Bounds.m_fSphereRadius));
  m_DataPointers.PushBack(pData);
  EZ_VERIFY(!m_PointerToIndexTable.Insert(pData, dataIndex), "Implementation error");
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
}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::UpdateData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = 0;
  EZ_VERIFY(m_PointerToIndexTable.TryGetValue(pData, dataIndex), "Implementation error");

  m_BoundingSpheres[dataIndex] = pData->m_Bounds.m_vCenter.GetAsVec4(pData->m_Bounds.m_fSphereRadius);
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

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, ezDynamicArray<ezGameObject *>& out_Objects) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, QueryCallback& callback) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, ezDynamicArray<ezGameObject *>& out_Objects) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, QueryCallback& callback) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindVisibleObjects(const ezFrustum& frustum, ezDynamicArray<const ezGameObject *>& out_Objects) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::SpatialDataAdded(ezSpatialData* pData)
{
  ForEachCellInBox(pData->m_Bounds.GetBox(), [&](const ezVec3I32& cellIndex, ezUInt64 cellKey, Cell* pCell)
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
  ForEachCellInBox(pData->m_Bounds.GetBox(), [&](const ezVec3I32& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    EZ_ASSERT_DEBUG(pCell != nullptr, "Implementation error");
    pCell->RemoveData(pData);
  });
}

void ezSpatialSystem_RegularGrid::SpatialDataChanged(ezSpatialData* pData, const ezBoundingBoxSphere& oldBounds)
{
  ezBoundingBox oldBox = oldBounds.GetBox();
  ezBoundingBox newBox = pData->m_Bounds.GetBox();

  ezBoundingBoxTemplate<ezInt32> oldIndexBox;
  oldIndexBox.m_vMin = ToVec3I32(oldBox.m_vMin * m_fInvCellSize);
  oldIndexBox.m_vMax = ToVec3I32(oldBox.m_vMax * m_fInvCellSize);

  ezBoundingBoxTemplate<ezInt32> newIndexBox;
  newIndexBox.m_vMin = ToVec3I32(newBox.m_vMin * m_fInvCellSize);
  newIndexBox.m_vMax = ToVec3I32(newBox.m_vMax * m_fInvCellSize);

  ezBoundingBox combinedBox = oldBox;
  combinedBox.ExpandToInclude(newBox);

  ForEachCellInBox(combinedBox, [&](const ezVec3I32& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    const bool bContainsOld = oldIndexBox.Contains(cellIndex);
    const bool bContainsNew = newIndexBox.Contains(cellIndex);

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
  ForEachCellInBox(pNewPtr->m_Bounds.GetBox(), [&](const ezVec3I32& cellIndex, ezUInt64 cellKey, Cell* pCell)
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
EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::ForEachCellInBox(const ezBoundingBox& box, Functor func)
{
  ezVec3I32 minIndex = ToVec3I32(box.m_vMin * m_fInvCellSize);
  ezVec3I32 maxIndex = ToVec3I32(box.m_vMax * m_fInvCellSize);

  EZ_ASSERT_DEBUG(ezMath::Abs(minIndex.x) < MAX_CELL_INDEX && ezMath::Abs(minIndex.y) < MAX_CELL_INDEX && ezMath::Abs(minIndex.z) < MAX_CELL_INDEX, "Position is too big");
  EZ_ASSERT_DEBUG(ezMath::Abs(maxIndex.x) < MAX_CELL_INDEX && ezMath::Abs(maxIndex.y) < MAX_CELL_INDEX && ezMath::Abs(maxIndex.z) < MAX_CELL_INDEX, "Position is too big");

  for (ezInt32 z = minIndex.z; z <= maxIndex.z; ++z)
  {
    for (ezInt32 y = minIndex.y; y <= maxIndex.y; ++y)
    {
      for (ezInt32 x = minIndex.x; x <= maxIndex.x; ++x)
      {
        ezVec3I32 cellIndex = ezVec3I32(x, y, z);
        ezUInt64 cellKey = GetCellKey(cellIndex);

        Cell* pCell = nullptr;
        m_Cells.TryGetValue(cellKey, pCell);

        func(cellIndex, cellKey, pCell);
      }
    }
  }
}
