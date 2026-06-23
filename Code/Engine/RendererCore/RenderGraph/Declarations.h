#pragma once

#include <Foundation/Types/Id.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Forward declarations
class ezRenderGraph;
class ezRenderGraphPassBuilder;
class ezRenderGraphContext;
class ezRenderGraphManager;
struct ezRenderGraphInspectionInfo;
struct ezRenderGraphDebugTarget;
class ezRenderGraphResourcePool;
class ezRenderGraphResourceAllocator;
class ezPooledRenderTexture;
class ezPooledRenderBuffer;
struct ezRenderGraphInspectionSummary;

/// Opaque handle to a texture within a render graph.
class ezRenderGraphTextureHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezRenderGraphTextureHandle, ezGAL::ez18_14Id);
  friend class ezRenderGraph;
  friend class ezRenderGraphPassBuilder;
  friend class ezRenderGraphManager;
};

/// Opaque handle to a buffer within a render graph.
class ezRenderGraphBufferHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezRenderGraphBufferHandle, ezGAL::ez18_14Id);
  friend class ezRenderGraph;
  friend class ezRenderGraphPassBuilder;
  friend class ezRenderGraphManager;
};

template <>
struct ezHashHelper<ezRenderGraphTextureHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezRenderGraphTextureHandle value)
  {
    return ezHashHelper<ezRenderGraphTextureHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezRenderGraphTextureHandle a, ezRenderGraphTextureHandle b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<ezRenderGraphBufferHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezRenderGraphBufferHandle value)
  {
    return ezHashHelper<ezRenderGraphBufferHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezRenderGraphBufferHandle a, ezRenderGraphBufferHandle b)
  {
    return a == b;
  }
};



/// Coarse execution phase for render graphs. Graphs within the same phase execute in registration (FIFO) order.
struct ezRenderGraphPhase
{
  using StorageType = ezUInt8;

  enum Enum
  {
    PreRender,  ///< E.g. Subsystem setup: decals, shadows, reflections, RmlUi texture updates.
    Render,     ///< E.g. Main view rendering via render pipelines.
    PostRender, ///< E.g. Companion view blits, screen captures, readbacks.
    Default = Render
  };
};
