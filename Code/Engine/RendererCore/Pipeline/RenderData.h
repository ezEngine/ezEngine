#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

class ezRasterizerObject;

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
    bool IsValid() const { return m_uiValue != 0xFFFF; }

    ezUInt16 m_uiValue = 0xFFFF;
  };

  /// \brief This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  using SortingKeyFunc = ezUInt64 (*)(const ezRenderData*, const ezCamera&);

  static Category RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc);
  static Category RegisterDerivedCategory(const char* szCategoryName, Category baseCategory);
  static Category RegisterRedirectedCategory(const char* szCategoryName, Category staticCategory, Category dynamicCategory);
  static Category FindCategory(ezTempHashedString sCategoryName);
  static Category ResolveCategory(Category category, bool bDynamic);

  static ezHashedString GetCategoryName(Category category);
  static void GetAllCategoryNames(ezDynamicArray<ezHashedString>& out_categoryNames);

  static const ezRenderer* GetCategoryRenderer(Category category, const ezRTTI* pRenderDataType);

public:
  struct Caching
  {
    enum Enum
    {
      Never,
      IfStatic
    };
  };

  struct Flags
  {
    using StorageType = ezUInt32;

    enum Enum
    {
      Dynamic = EZ_BIT(0),

      Default = 0
    };

    struct Bits
    {
      StorageType Dynamic : 1;
    };
  };

  /// \brief Returns the final sorting for this render data with the given category and camera.
  ezUInt64 GetFinalSortingKey(Category category, const ezCamera& camera) const;

  /// \brief Returns whether this render data and the other render data can be batched together, e.g. rendered in one draw call.
  /// An implementation can assume that the other render data is of the same type as this render data.
  virtual bool CanBatch(const ezRenderData& other) const { return false; }

  ezBitflags<Flags> m_Flags;

  ezTransform m_GlobalTransform = ezTransform::MakeIdentity();
  ezBoundingBoxSphere m_GlobalBounds;

  ezUInt32 m_uiSortingKey = 0;
  float m_fSortingDepthOffset = 0.0f;

  ezGameObjectHandle m_hOwner;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezGameObject* m_pOwner = nullptr; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderData);

  static void PluginEventHandler(const ezPluginEvent& e);
  static void UpdateRendererTypes();

  static void CreateRendererInstances();
  static void ClearRendererInstances();

  struct CategoryData
  {
    Category m_baseCategory;
    Category m_staticCategory;
    Category m_dynamicCategory;

    ezHashedString m_sName;
    SortingKeyFunc m_sortingKeyFunc;

    ezHashTable<const ezRTTI*, ezUInt32> m_TypeToRendererIndex;
  };

  static ezHybridArray<CategoryData, 32> s_CategoryData;

  static ezHybridArray<const ezRTTI*, 16> s_RendererTypes;
  static ezDynamicArray<ezUniquePtr<ezRenderer>> s_RendererInstances;
  static bool s_bRendererInstancesDirty;
};

/// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
template <typename T>
static T* ezCreateRenderDataForThisFrame(const ezGameObject* pOwner);

struct EZ_RENDERERCORE_DLL ezDefaultRenderDataCategories
{
  static ezRenderData::Category Light;
  static ezRenderData::Category Decal;
  static ezRenderData::Category ReflectionProbe;
  static ezRenderData::Category Sky;
  static ezRenderData::Category LitOpaque;
  static ezRenderData::Category LitOpaqueStatic;
  static ezRenderData::Category LitOpaqueDynamic;
  static ezRenderData::Category LitMasked;
  static ezRenderData::Category LitMaskedStatic;
  static ezRenderData::Category LitMaskedDynamic;
  static ezRenderData::Category LitTransparent;
  static ezRenderData::Category LitForeground;
  static ezRenderData::Category LitScreenFX;
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
  /// Non-cached data is only valid for this frame. Cached data must be manually deleted using the ezRenderWorld::DeleteCachedRenderData
  /// function.
  void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category, ezRenderData::Caching::Enum cachingBehavior);

private:
  friend class ezExtractor;

  struct Data
  {
    const ezRenderData* m_pRenderData = nullptr;
    ezRenderData::Category m_Category;
  };

  ezHybridArray<Data, 16> m_ExtractedRenderData;
  ezUInt32 m_uiNumCacheIfStatic = 0;
};

struct EZ_RENDERERCORE_DLL ezMsgExtractOccluderData : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractOccluderData, ezMessage);

  void AddOccluder(const ezRasterizerObject* pObject, const ezTransform& transform)
  {
    auto& d = m_ExtractedOccluderData.ExpandAndGetRef();
    d.m_pObject = pObject;
    d.m_Transform = transform;
  }

private:
  friend class ezRenderPipeline;

  struct Data
  {
    const ezRasterizerObject* m_pObject = nullptr;
    ezTransform m_Transform;
  };

  ezHybridArray<Data, 16> m_ExtractedOccluderData;
};

#include <RendererCore/Pipeline/Implementation/RenderData_inl.h>
