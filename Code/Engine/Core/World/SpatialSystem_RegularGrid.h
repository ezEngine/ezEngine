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
  ezResult GetCellBoxForSpatialData(const ezSpatialDataHandle& hData, ezBoundingBox& out_boundingBox) const;

  /// \brief Returns bounding boxes of all existing cells.
  void GetAllCellBoxes(ezDynamicArray<ezBoundingBox>& out_boundingBoxes, ezSpatialData::Category filterCategory = ezInvalidSpatialDataCategory) const;

private:
  friend ezInternal::QueryHelper;

  // ezSpatialSystem implementation
  virtual void StartNewFrame() override;

  ezSpatialDataHandle CreateSpatialData(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags) override;
  ezSpatialDataHandle CreateSpatialDataAlwaysVisible(ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags) override;

  void DeleteSpatialData(const ezSpatialDataHandle& hData) override;

  void UpdateSpatialDataBounds(const ezSpatialDataHandle& hData, const ezSimdBBoxSphere& bounds) override;
  void UpdateSpatialDataObject(const ezSpatialDataHandle& hData, ezGameObject* pObject) override;

  void FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const override;
  void FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const override;

  void FindVisibleObjects(const ezFrustum& frustum, const QueryParams& queryParams, ezDynamicArray<const ezGameObject*>& out_Objects, ezSpatialSystem::IsOccludedFunc IsOccluded, ezVisibilityState::Enum visType) const override;

  ezVisibilityState::Enum GetVisibilityState(const ezSpatialDataHandle& hData, ezUInt32 uiNumFramesBeforeInvisible) const override;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(ezStringBuilder& sb) const override;
#endif

  ezProxyAllocator m_AlignedAllocator;

  ezSimdVec4i m_vCellSize;
  ezSimdVec4f m_vOverlapSize;
  ezSimdFloat m_fInvCellSize;

  enum
  {
    MAX_NUM_GRIDS = 63,
    MAX_NUM_REGULAR_GRIDS = (sizeof(ezSpatialData::Category::m_uiValue) * 8),
    MAX_NUM_CACHED_GRIDS = MAX_NUM_GRIDS - MAX_NUM_REGULAR_GRIDS
  };

  struct Cell;
  struct Grid;
  ezDynamicArray<ezUniquePtr<Grid>> m_Grids;
  ezUInt32 m_uiFirstCachedGridIndex = MAX_NUM_GRIDS;

  struct Data
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt64 m_uiGridBitmask : MAX_NUM_GRIDS;
    ezUInt64 m_uiAlwaysVisible : 1;
  };

  ezIdTable<ezSpatialDataId, Data, ezLocalAllocatorWrapper> m_DataTable;

  bool IsAlwaysVisibleData(const Data& data) const;

  ezSpatialDataHandle AddSpatialDataToGrids(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags, bool bAlwaysVisible);

  template <typename Functor>
  void ForEachGrid(const Data& data, const ezSpatialDataHandle& hData, Functor func) const;

  struct Stats;
  using CellCallback = ezDelegate<ezVisitorExecution::Enum(const Cell&, const QueryParams&, Stats&, void*, ezVisibilityState::Enum)>;
  void ForEachCellInBoxInMatchingGrids(const ezSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, ezVisibilityState::Enum visType) const;

  struct CacheCandidate
  {
    ezTagSet m_IncludeTags;
    ezTagSet m_ExcludeTags;
    ezSpatialData::Category m_Category;
    float m_fQueryCount = 0.0f;
    float m_fFilteredRatio = 0.0f;
    ezUInt32 m_uiGridIndex = ezInvalidIndex;
  };

  mutable ezDynamicArray<CacheCandidate> m_CacheCandidates;
  mutable ezMutex m_CacheCandidatesMutex;

  struct SortedCacheCandidate
  {
    ezUInt32 m_uiIndex = 0;
    float m_fScore = 0;

    bool operator<(const SortedCacheCandidate& other) const
    {
      if (m_fScore != other.m_fScore)
        return m_fScore > other.m_fScore; // higher score comes first

      return m_uiIndex < other.m_uiIndex;
    }
  };

  ezDynamicArray<SortedCacheCandidate> m_SortedCacheCandidates;

  void MigrateCachedGrid(ezUInt32 uiCandidateIndex);
  void MigrateSpatialData(ezUInt32 uiTargetGridIndex, ezUInt32 uiSourceGridIndex);

  void RemoveCachedGrid(ezUInt32 uiCandidateIndex);
  void RemoveAllCachedGrids();

  void UpdateCacheCandidate(const ezTagSet* pIncludeTags, const ezTagSet* pExcludeTags, ezSpatialData::Category category, float filteredRatio) const;
};
