
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>


struct EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoderRenderState final
{
  void InvalidateState();
  ezGALBufferHandle m_hVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezUInt32 m_hVertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  ezGALBufferHandle m_hIndexBuffer;

  ezGALGraphicsPipelineHandle m_hGraphicsPipeline;
  ezGALComputePipelineHandle m_hComputePipeline;
  ezUInt8 m_uiStencilRefValue = 0;

  ezRectU32 m_ScissorRect = ezRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  ezRectFloat m_ViewPortRect = ezRectFloat(ezMath::MaxValue<float>(), ezMath::MaxValue<float>(), 0.0f, 0.0f);
  float m_fViewPortMinDepth = ezMath::MaxValue<float>();
  float m_fViewPortMaxDepth = -ezMath::MaxValue<float>();
};

struct EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoderStats
{
  void Reset();
  void SetStatistics();
  void operator+=(const ezGALCommandEncoderStats& rhs);

  // Operations
  ezUInt32 m_uiInsertTimestamp = 0;
  ezUInt32 m_uiBeginOcclusionQuery = 0;
  ezUInt32 m_uiInsertFence = 0;
  ezUInt32 m_uiCopyBuffer = 0;
  ezUInt32 m_uiUpdateBuffer = 0;
  ezUInt32 m_uiCopyTexture = 0;
  ezUInt32 m_uiUpdateTexture = 0;
  ezUInt32 m_uiResolveTexture = 0;
  ezUInt32 m_uiReadbackTexture = 0;
  ezUInt32 m_uiReadbackBuffer = 0;
  ezUInt32 m_uiFlush = 0;
  // Compute
  ezUInt32 m_uiBeginCompute = 0;
  ezUInt32 m_uiDispatch = 0;
  // Rendering
  ezUInt32 m_uiBeginRendering = 0;
  ezUInt32 m_uiClear = 0;
  ezUInt32 m_uiDraw = 0;
  // State Changes
  ezUInt32 m_uiSetIndexBuffer = 0;
  ezUInt32 m_uiSetVertexBuffer = 0;
  ezUInt32 m_uiSetGraphicsPipeline = 0;
  ezUInt32 m_uiSetComputePipeline = 0;
  // Dynamic State Changes
  ezUInt32 m_uiSetViewport = 0;
  ezUInt32 m_uiSetScissorRect = 0;
  ezUInt32 m_uiSetStencilReference = 0;
};
