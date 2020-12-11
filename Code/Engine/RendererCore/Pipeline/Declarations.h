#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class ezCamera;
class ezExtractedRenderData;
class ezExtractor;
class ezView;
class ezRenderer;
class ezRenderData;
class ezRenderDataBatch;
class ezRenderPipeline;
class ezRenderPipelinePass;
class ezRenderContext;
class ezDebugRendererContext;

struct ezRenderPipelineNodePin;
struct ezRenderPipelinePassConnection;
struct ezViewData;

namespace ezInternal
{
  struct RenderDataCache;

  struct RenderDataCacheEntry
  {
    EZ_DECLARE_POD_TYPE();

    const ezRenderData* m_pRenderData = nullptr;
    ezUInt16 m_uiCategory = 0;
    ezUInt16 m_uiComponentIndex = 0;
    ezUInt16 m_uiPartIndex = 0;

    EZ_ALWAYS_INLINE bool operator==(const RenderDataCacheEntry& other) const { return m_pRenderData == other.m_pRenderData && m_uiCategory == other.m_uiCategory && m_uiComponentIndex == other.m_uiComponentIndex && m_uiPartIndex == other.m_uiPartIndex; }

    // Cache entries need to be sorted by component index and then by part index
    EZ_ALWAYS_INLINE bool operator<(const RenderDataCacheEntry& other) const
    {
      if (m_uiComponentIndex == other.m_uiComponentIndex)
        return m_uiPartIndex < other.m_uiPartIndex;

      return m_uiComponentIndex < other.m_uiComponentIndex;
    }
  };
} // namespace ezInternal

struct ezRenderViewContext
{
  const ezCamera* m_pCamera;
  const ezViewData* m_pViewData;
  ezRenderContext* m_pRenderContext;

  const ezDebugRendererContext* m_pWorldDebugContext;
  const ezDebugRendererContext* m_pViewDebugContext;
};

typedef ezGenericId<24, 8> ezViewId;

class ezViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezViewHandle, ezViewId);

  friend class ezRenderWorld;
};

/// \brief HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct ezHashHelper<ezViewHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  EZ_ALWAYS_INLINE static bool Equal(ezViewHandle a, ezViewHandle b) { return a == b; }
};

/// \brief Usage hint of a camera/view.
struct EZ_RENDERERCORE_DLL ezCameraUsageHint
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    MainView,
    EditorView,
    RenderTarget,
    Culling,
    Shadow,
    Reflection,
    Thumbnail,

    ENUM_COUNT,

    Default = None,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezCameraUsageHint);
