#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/TagSet.h>

class EZ_CORE_DLL ezSpatialSystem : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpatialSystem, ezReflectedClass);

public:
  ezSpatialSystem();
  ~ezSpatialSystem();

  virtual void StartNewFrame();

  /// \name Spatial Data Functions
  ///@{

  virtual ezSpatialDataHandle CreateSpatialData(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags) = 0;
  virtual ezSpatialDataHandle CreateSpatialDataAlwaysVisible(ezGameObject* pObject, ezUInt32 uiCategoryBitmask, const ezTagSet& tags) = 0;

  virtual void DeleteSpatialData(const ezSpatialDataHandle& hData) = 0;

  virtual void UpdateSpatialDataBounds(const ezSpatialDataHandle& hData, const ezSimdBBoxSphere& bounds) = 0;
  virtual void UpdateSpatialDataObject(const ezSpatialDataHandle& hData, ezGameObject* pObject) = 0;

  ///@}
  /// \name Simple Queries
  ///@{

  typedef ezDelegate<ezVisitorExecution::Enum(ezGameObject*)> QueryCallback;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  struct QueryStats
  {
    ezUInt32 m_uiTotalNumObjects = 0;  ///< The total number of spatial objects in this system.
    ezUInt32 m_uiNumObjectsTested = 0; ///< Number of objects tested for the query condition.
    ezUInt32 m_uiNumObjectsPassed = 0; ///< Number of objects that passed the query condition.
    ezTime m_TimeTaken;                ///< Time taken to execute the query
  };
#endif

  struct QueryParams
  {
    ezUInt32 m_uiCategoryBitmask = 0;
    ezTagSet m_IncludeTags;
    ezTagSet m_ExcludeTags;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    QueryStats* m_pStats = nullptr;
#endif
  };

  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_Objects) const;
  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const = 0;

  virtual void FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_Objects) const;
  virtual void FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const = 0;

  ///@}
  /// \name Visibility Queries
  ///@{

  virtual void FindVisibleObjects(const ezFrustum& frustum, const QueryParams& queryParams, ezDynamicArray<const ezGameObject*>& out_Objects) const = 0;

  virtual ezUInt64 GetNumFramesSinceVisible(const ezSpatialDataHandle& hData) const = 0;

  ///@}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(ezStringBuilder& sb) const;
#endif

protected:
  ezProxyAllocator m_Allocator;

  ezUInt64 m_uiFrameCounter = 0;
};
