#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Communication/Message.h>

/// \brief Base class for all render data. Render data must contain all information that is needed to render the corresponding object.
class EZ_RENDERERCORE_DLL ezRenderData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderData, ezReflectedClass);

public:

  struct Category
  {
    Category();
    explicit Category(ezUInt16 uiValue);

    bool operator==(const Category& other) const;
    bool operator!=(const Category& other) const;

    ezUInt16 m_uiValue;
  };

  struct Caching
  {
    enum Enum
    {
      Never,
      IfStatic
    };
  };

  /// \brief This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  typedef ezDelegate<ezUInt64(const ezRenderData*, ezUInt32, const ezCamera&)> SortingKeyFunc;

  static Category RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc);
  static Category FindCategory(const char* szCategoryName);

  static const char* GetCategoryName(Category category);

  /// \brief Returns the sorting key for this render data by using the sorting key function for the given category.
  ezUInt64 GetCategorySortingKey(Category category, ezUInt32 uiRenderDataSortingKey, const ezCamera& camera) const;

  ezUInt32 m_uiBatchId; ///< BatchId is used to group render data in batches.
  ezGameObjectHandle m_hOwner;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezGameObject* m_pOwner; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

  ezTransform m_GlobalTransform;
  ezBoundingBoxSphere m_GlobalBounds;

private:
  struct CategoryData
  {
    ezHashedString m_sName;
    SortingKeyFunc m_sortingKeyFunc;
  };

  static ezHybridArray<CategoryData, 32> s_CategoryData;
};

/// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
template <typename T>
static T* ezCreateRenderDataForThisFrame(const ezGameObject* pOwner, ezUInt32 uiBatchId);

struct EZ_RENDERERCORE_DLL ezDefaultRenderDataCategories
{
  static ezRenderData::Category Light;
  static ezRenderData::Category Decal;
  static ezRenderData::Category Sky;
  static ezRenderData::Category LitOpaque;
  static ezRenderData::Category LitMasked;
  static ezRenderData::Category LitTransparent;
  static ezRenderData::Category LitForeground;
  static ezRenderData::Category SimpleOpaque;
  static ezRenderData::Category SimpleTransparent;
  static ezRenderData::Category SimpleForeground;
  static ezRenderData::Category Selection;
  static ezRenderData::Category GUI;
};

#define ezInvalidRenderDataCategory ezRenderData::Category()

struct EZ_RENDERERCORE_DLL ezMsgExtractRenderData : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractRenderData, ezMessage);

  const ezView* m_pView = nullptr;
  ezRenderData::Category m_OverrideCategory = ezInvalidRenderDataCategory;

  /// \brief Adds render data for the current view. This data can be cached depending on the specified caching behavior.
  /// Non-cached data is only valid for this frame. Cached data must be manually deleted using the ezRenderWorld::DeleteCachedRenderData function.
  void AddRenderData(ezRenderData* pRenderData, ezRenderData::Category category, ezUInt32 uiSortingKey,
    ezRenderData::Caching::Enum cachingBehavior = ezRenderData::Caching::Never);

private:
  friend class ezExtractor;

  ezHybridArray<ezInternal::RenderDataCacheEntry, 4> m_ExtractedRenderData;
};

#include <RendererCore/Pipeline/Implementation/RenderData_inl.h>

