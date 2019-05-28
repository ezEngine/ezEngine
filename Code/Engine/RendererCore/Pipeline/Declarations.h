#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Core/World/Declarations.h>
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
class ezGALContext;
class ezRenderContext;
class ezDebugRendererContext;

struct ezNodePin;
struct ezRenderPipelinePassConnection;
struct ezViewData;

namespace ezInternal
{
  struct RenderDataCache;

  struct RenderDataCacheEntry
  {
    EZ_DECLARE_POD_TYPE();

    const ezRenderData* m_pRenderData;
    ezUInt32 m_uiSortingKey;
    ezUInt16 m_uiCategory;
    ezUInt16 m_uiComponentIndex : 15;
    ezUInt16 m_uiCacheIfStatic : 1;
  };
}

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
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezViewHandle value)
  {
    return value.GetInternalID().m_Data * 2654435761U;
  }

  EZ_ALWAYS_INLINE static bool Equal(ezViewHandle a, ezViewHandle b)
  {
    return a == b;
  }
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

