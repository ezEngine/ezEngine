#pragma once

#include <Core/World/SpatialSystem.h>

class EZ_CORE_DLL ezSpatialSystem_RegularGrid : public ezSpatialSystem
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpatialSystem_RegularGrid, ezSpatialSystem);

public:
  ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize = 128);
  ~ezSpatialSystem_RegularGrid();

  /// \brief Returns bounding boxes of all cells associated with the given spatial data. Useful for debug visualizations.
  void GetCellBoxesForSpatialData(const ezSpatialDataHandle& hData, ezHybridArray<ezBoundingBox, 16>& out_BoundingBoxes) const;

  /// \brief Returns bounding boxes of all existing cells.
  void GetAllCellBoxes(ezHybridArray<ezBoundingBox, 16>& out_BoundingBoxes) const;

private:
  // ezSpatialSystem implementation
  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, ezDynamicArray<ezGameObject *>& out_Objects) const override;
  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, QueryCallback& callback) const override;

  virtual void FindObjectsInBox(const ezBoundingBox& box, ezDynamicArray<ezGameObject *>& out_Objects) const override;
  virtual void FindObjectsInBox(const ezBoundingBox& box, QueryCallback& callback) const override;

  virtual void FindVisibleObjects(const ezFrustum& frustum, ezDynamicArray<const ezGameObject *>& out_Objects) const override;

  virtual void SpatialDataAdded(ezSpatialData* pData) override;
  virtual void SpatialDataRemoved(ezSpatialData* pData) override;
  virtual void SpatialDataChanged(ezSpatialData* pData, const ezBoundingBoxSphere& oldBounds) override;
  virtual void FixSpatialDataPointer(ezSpatialData* pOldPtr, ezSpatialData* pNewPtr) override;

  template <typename Functor>
  void ForEachCellInBox(const ezBoundingBox& box, Functor func);

  ezProxyAllocator m_AlignedAllocator;
  ezInt32 m_iCellSize;
  float m_fInvCellSize;

  struct Cell
  {
    Cell(const ezVec3I32& index, ezAllocatorBase* pAllocator, ezAllocatorBase* pAlignedAllocator);

    void AddData(ezSpatialData* pData);
    void RemoveData(ezSpatialData* pData);
    void UpdateData(ezSpatialData* pData);

    ezVec3I32 m_Index;

    ezDynamicArray<ezVec4> m_BoundingSpheres; ///\todo should be simd vec4
    ezDynamicArray<ezSpatialData*> m_DataPointers;
    ezHashTable<ezSpatialData*, ezUInt32> m_PointerToIndexTable;
  };

  ezHashTable<ezUInt64, Cell*, ezHashHelper<ezUInt64>, ezLocalAllocatorWrapper> m_Cells;
};
