#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezStreamReader;
class ezStreamWriter;

/// Identifies a render graph execution that was observed during the last frame.
struct ezRenderGraphExecutionSummary
{
  ezUInt64 m_uiRenderGraphId = 0;
  ezString m_sGraphName;
  ezString m_sUserName;
  ezEnum<ezRenderGraphPhase> m_Phase;
  ezInt32 m_uiExecutionOrder = -1; ///< execution oder in the phase. -1 if not executed this frame.
};

/// Describes a swap-chain that can be used as a preview target by the render graph inspector.
struct ezRenderGraphSwapChainSummary
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiSwapChainId = 0;
  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
};

/// Aggregates the render graphs and preview targets currently known to the inspector.
struct ezRenderGraphInspectionSummary
{
  ezDynamicArray<ezRenderGraphExecutionSummary> m_RenderGraphs;
  ezDynamicArray<ezRenderGraphSwapChainSummary> m_AvailableSwapChains;
};

EZ_RENDERERCORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphExecutionSummary& value);
EZ_RENDERERCORE_DLL void operator>>(ezStreamReader& inout_stream, ezRenderGraphExecutionSummary& ref_value);
EZ_RENDERERCORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphSwapChainSummary& value);
EZ_RENDERERCORE_DLL void operator>>(ezStreamReader& inout_stream, ezRenderGraphSwapChainSummary& ref_value);
EZ_RENDERERCORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphInspectionSummary& value);
EZ_RENDERERCORE_DLL void operator>>(ezStreamReader& inout_stream, ezRenderGraphInspectionSummary& ref_value);

/// Read-only snapshot of a compiled render graph's structure, taken after compilation. Used for visualization and debugging without exposing internal graph state.
struct EZ_RENDERERCORE_DLL ezRenderGraphInspectionInfo
{
  void Swap(ezRenderGraphInspectionInfo& ref_data);
  void Clear();

  /// Describes one declared pass and whether it remains active after graph compilation.
  struct PassInfo
  {
    ezString m_sName;
    ezEnum<ezGALQueueType> m_QueueType;
    bool m_bHasSideEffects = false;
    bool m_bAlive = true; ///< false if the pass was culled during compilation.
  };

  /// Describes a texture resource used by the compiled render graph.
  struct TextureResourceInfo
  {
    EZ_DECLARE_POD_TYPE();
    ezGALTextureCreationDescription m_Desc;
    bool m_bImported = false;
    ezUInt16 m_uiFirstUsePassIndex = 0xFFFF;
    ezUInt16 m_uiLastUsePassIndex = 0xFFFF;
    ezUInt16 m_uiResolvedIndex = 0xFFFF;
  };

  /// Describes a buffer resource used by the compiled render graph.
  struct BufferResourceInfo
  {
    EZ_DECLARE_POD_TYPE();
    ezGALBufferCreationDescription m_Desc;
    bool m_bImported = false;
    ezUInt16 m_uiFirstUsePassIndex = 0xFFFF;
    ezUInt16 m_uiLastUsePassIndex = 0xFFFF;
    ezUInt16 m_uiResolvedIndex = 0xFFFF;
  };

  /// Describes a single resource access made by one pass in the compiled render graph.
  struct AccessInfo
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt16 m_uiPassIndex = 0;     ///< Index into m_Passes (declaration order).
    ezUInt16 m_uiResourceIndex = 0; ///< Index into m_Textures or m_Buffers.
    ezUInt16 m_uiAccessIndex = 0;
    bool m_bIsTexture = true;
    ezBitflags<ezGALResourceState> m_Access;
    ezGALTextureRange m_TextureRange; ///< Only meaningful for textures.
  };

  /// All passes in declaration order. Culled passes have m_bAlive == false.
  ezDynamicArray<PassInfo> m_Passes;

  ezDynamicArray<TextureResourceInfo> m_Textures;
  ezDynamicArray<BufferResourceInfo> m_Buffers;

  /// All resource accesses across all passes, flattened first by passId, then by access types.
  ezDynamicArray<AccessInfo> m_Accesses;
};

EZ_RENDERERCORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphInspectionInfo& value);
EZ_RENDERERCORE_DLL void operator>>(ezStreamReader& inout_stream, ezRenderGraphInspectionInfo& ref_value);
