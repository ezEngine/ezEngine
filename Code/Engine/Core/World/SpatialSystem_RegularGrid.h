#pragma once

#include <Core/World/SpatialSystem.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/UniquePtr.h>

namespace ezInternal
{
  struct QueryHelper;
}

class EZ_CORE_DLL ezSpatialSystem_RegularGrid : public ezSpatialSystem
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpatialSystem_RegularGrid, ezSpatialSystem);

public:
  ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize = 128);
  ~ezSpatialSystem_RegularGrid();

  /// \brief Returns the bounding box of the cell associated with the given spatial data. Useful for debug visualizations.
  ezResult GetCellBoxForSpatialData(const ezSpatialDataHandle& hData, ezBoundingBox& out_BoundingBox) const;

  /// \brief Returns bounding boxes of all existing cells.
  void GetAllCellBoxes(ezDynamicArray<ezBoundingBox>& out_BoundingBoxes, ezSpatialData::Category filterCategory = ezInvalidSpatialDataCategory) const;

private:
  friend ezInternal::QueryHelper;

  // ezSpatialSystem implementation
  ezSpatialDataHandle CreateSpatialData(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags) override;
  ezSpatialDataHandle CreateSpatialDataAlwaysVisible(ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags) override;

  void DeleteSpatialData(const ezSpatialDataHandle& hData) override;

  void UpdateSpatialDataBounds(const ezSpatialDataHandle& hData, const ezSimdBBoxSphere& bounds) override;
  void UpdateSpatialDataObject(const ezSpatialDataHandle& hData, ezGameObject* pObject) override;

  void FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const override;
  void FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const override;

  void FindVisibleObjects(const ezFrustum& frustum, const QueryParams& queryParams, ezDynamicArray<const ezGameObject*>& out_Objects) const override;

  ezUInt64 GetNumFramesSinceVisible(const ezSpatialDataHandle& hData) const override;

  ezProxyAllocator m_AlignedAllocator;

  ezSimdVec4i m_iCellSize;
  ezSimdVec4f m_fOverlapSize;
  ezSimdFloat m_fInvCellSize;

  struct Cell;
  struct Grid;
  ezDynamicArray<ezUniquePtr<Grid>> m_Grids;

  struct Data
  {
    ezUInt64 m_uiGridBitmask = 0;
  };
  
  ezIdTable<ezSpatialDataId, Data, ezLocalAllocatorWrapper> m_DataTable;

  bool IsAlwaysVisibleData(const Data& data) const;

  template <typename Functor>
  void ForEachGrid(const Data& data, const ezSpatialDataHandle& hData, Functor func) const;

  using CellCallback = ezDelegate<ezVisitorExecution::Enum(const Cell&, const QueryParams&, void*)>;
  void ForEachCellInBoxInMatchingGrids(const ezSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByCategoryCallback, CellCallback filterByTagsCallback, CellCallback filterByCategoryAndTagsCallback, void* pUserData) const;
};
