#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

void ezGALCommandEncoderRenderState::InvalidateState()
{
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
    m_hVertexBufferOffsets[i] = 0;
  }
  m_hIndexBuffer.Invalidate();

  m_hGraphicsPipeline.Invalidate();
  m_hComputePipeline.Invalidate();
  m_uiStencilRefValue = 0;

  m_ScissorRect = ezRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect = ezRectFloat(ezMath::MaxValue<float>(), ezMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = ezMath::MaxValue<float>();
  m_fViewPortMaxDepth = -ezMath::MaxValue<float>();
}
