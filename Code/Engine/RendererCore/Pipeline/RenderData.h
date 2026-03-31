#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/Vec3.h>
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
      FlipWinding = EZ_BIT(1),

      Default = 0
    };

    struct Bits
    {
      StorageType Dynamic : 1;
      StorageType FlipWinding : 1;
    };
  };

  bool IsDynamic() const;
  bool IsStatic() const;
  bool FlipWinding() const;

  /// \brief Returns the final sorting for this render data with the given category and camera.
  ezUInt64 GetFinalSortingKey(Category category, const ezCamera& camera) const;

  /// \brief Returns whether this render data and the other render data can be batched together, e.g. rendered in one draw call.
  /// An implementation can assume that the other render data is of the same type as this render data.
  virtual bool CanBatch(const ezRenderData& other) const { return false; }

  ezBitflags<Flags> m_Flags;

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
  float m_fSortingDepthOffset = 0.0f;

  ezUInt32 m_uiSortingKey = 0;

  ezGameObjectHandle m_hOwner;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezGameObject* m_pOwner = nullptr; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

private:
  struct CategoryData
  {
    Category m_baseCategory;
    Category m_staticCategory;
    Category m_dynamicCategory;

    ezHashedString m_sName;
    SortingKeyFunc m_sortingKeyFunc;
  };

  static ezHybridArray<CategoryData, 32> s_CategoryData;
};

/// \brief Base class for render data that make uses of the instance data offset buffer which will be generated during the extraction phase.
class EZ_RENDERERCORE_DLL ezInstanceableRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInstanceableRenderData, ezRenderData);

public:
  struct DataOffsets
  {
    ezUInt32 m_uiInstance = 0;
    ezUInt32 m_uiCustomInstance = 0;
    ezUInt32 m_uiMaterial = 0;
    ezUInt32 m_uiSkinning = 0; // TODO: this could be removed if we switch to compute shader skinning
  };

  DataOffsets m_DataOffsets;

  ezUInt32 m_uiNumInstances = 1;
  ezGALDynamicBufferHandle m_hInstanceDataBuffer;

protected:
  bool CanBatchByBaseValues(const ezInstanceableRenderData& other) const;
};

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
  static ezRenderData::Category LensEffects;
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
  const ezRenderDataManager* m_pRenderDataManager = nullptr;
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

struct ezInstanceDataOffset
{
  EZ_DECLARE_POD_TYPE();

  ezInstanceDataOffset()
    : m_uiOffset(ezMath::Bitmask_LowN<ezUInt32>(31))
    , m_uiIsDynamic(0)
  {
  }

  EZ_ALWAYS_INLINE bool IsInvalidated() const { return m_uiOffset == ezMath::Bitmask_LowN<ezUInt32>(31); }

  ezUInt32 m_uiOffset : 31;
  ezUInt32 m_uiIsDynamic : 1;
};

struct ezCustomInstanceDataOffset
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE bool IsInvalidated() const { return m_uiOffset == ezInvalidIndex; }

  ezUInt32 m_uiOffset = ezInvalidIndex;
};

struct EZ_RENDERERCORE_DLL ezMsgCustomInstanceDataOffsetChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgCustomInstanceDataOffsetChanged, ezMessage);

  ezCustomInstanceDataOffset m_NewOffset;
};

#include <RendererCore/Pipeline/Implementation/RenderData_inl.h>
