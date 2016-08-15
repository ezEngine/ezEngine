#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Foundation/Memory/FrameAllocator.h>

/// \brief Base class for all render data. Render data must contain all information that is needed to render the corresponding object.
class EZ_RENDERERCORE_DLL ezRenderData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderData, ezReflectedClass);

public:
  typedef ezUInt32 Category;

  /// \brief This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  typedef ezDelegate<ezUInt64(const ezRenderData*, ezUInt32, const ezCamera&)> SortingKeyFunc;

  static Category RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc);

  static const char* GetCategoryName(Category category);
  static ezProfilingId& GetCategoryProfilingID(Category category);

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
    ezProfilingId m_ProfilingID;
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
  static ezRenderData::Category LitOpaque;
  static ezRenderData::Category LitMasked;
  static ezRenderData::Category LitTransparent;
  static ezRenderData::Category LitForeground;
  static ezRenderData::Category SimpleOpaque;
  static ezRenderData::Category SimpleTransparent;
  static ezRenderData::Category SimpleForeground;
  static ezRenderData::Category Selection;
};

struct EZ_RENDERERCORE_DLL ezExtractRenderDataMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezExtractRenderDataMessage);

  const ezView* m_pView;
  ezExtractedRenderData* m_pExtractedRenderData;
  ezRenderData::Category m_OverrideCategory;
};

#include <RendererCore/Pipeline/Implementation/RenderData_inl.h>
