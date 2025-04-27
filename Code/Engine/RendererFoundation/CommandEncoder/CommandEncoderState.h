
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
