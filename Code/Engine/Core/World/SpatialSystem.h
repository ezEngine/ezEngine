#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/Types/TagSet.h>

/// \brief Abstract base class for spatial systems that organize objects for efficient spatial queries.
///
/// Spatial systems manage spatial data for objects in a world, enabling efficient queries like
/// finding objects in a sphere or box, frustum culling, and visibility testing. Concrete
/// implementations use different spatial data structures (octrees, grids, etc.) for optimization.
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

  using QueryCallback = ezDelegate<ezVisitorExecution::Enum(ezGameObject*)>;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  struct QueryStats
  {
    ezUInt32 m_uiTotalNumObjects = 0;  ///< The total number of spatial objects in this system.
    ezUInt32 m_uiNumObjectsTested = 0; ///< Number of objects tested for the query condition.
    ezUInt32 m_uiNumObjectsPassed = 0; ///< Number of objects that passed the query condition.
    ezTime m_TimeTaken;                ///< Time taken to execute the query
  };
#endif

  /// \brief Parameters for spatial queries to filter and track results.
  struct QueryParams
  {
    ezUInt32 m_uiCategoryBitmask = 0;         ///< Bitmask of spatial data categories to include in the query
    const ezTagSet* m_pIncludeTags = nullptr; ///< Only include objects that have all of these tags
    const ezTagSet* m_pExcludeTags = nullptr; ///< Exclude objects that have any of these tags
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    QueryStats* m_pStats = nullptr;           ///< Optional stats tracking for development builds
#endif
  };

  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_objects) const;
  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const = 0;

  virtual void FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_objects) const;
  virtual void FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const = 0;

  ///@}
  /// \name Visibility Queries
  ///@{

  using IsOccludedFunc = ezDelegate<bool(const ezSimdBBox&)>;

  virtual void FindVisibleObjects(const ezFrustum& frustum, const QueryParams& queryParams, ezDynamicArray<const ezGameObject*>& out_objects, IsOccludedFunc isOccluded, ezVisibilityState::Enum visType) const = 0;

  /// \brief Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  virtual ezVisibilityState::Enum GetVisibilityState(const ezSpatialDataHandle& hData, ezUInt32 uiNumFramesBeforeInvisible) const = 0;

  ///@}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(ezStringBuilder& ref_sSb) const;
#endif

protected:
  ezProxyAllocator m_Allocator;

  ezUInt64 m_uiFrameCounter = 0;
};

/// \brief Script extension class providing spatial query functions for scripting languages.
class EZ_CORE_DLL ezScriptExtensionClass_Spatial
{
public:
  /// \brief Finds the closest object in a sphere within the given category.
  static ezGameObject* FindClosestObjectInSphere(ezWorld* pWorld, ezStringView sCategory, const ezVec3& vCenter, float fRadius);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Spatial);
