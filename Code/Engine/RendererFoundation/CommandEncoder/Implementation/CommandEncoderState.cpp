#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

void ezGALCommandEncoderRenderState::InvalidateState()
{
  m_hShader = ezGALShaderHandle();
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
    m_hVertexBufferOffsets[i] = 0;
  }
  m_hIndexBuffer.Invalidate();

  m_hVertexDeclaration.Invalidate();
  m_Topology = ezGALPrimitiveTopology::ENUM_COUNT;

  m_hBlendState.Invalidate();
  m_BlendFactor = ezColor(0, 0, 0, 0);
  m_uiSampleMask = 0;

  m_hDepthStencilState.Invalidate();
  m_uiStencilRefValue = 0;

  m_hRasterizerState.Invalidate();

  m_ScissorRect = ezRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect = ezRectFloat(ezMath::MaxValue<float>(), ezMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = ezMath::MaxValue<float>();
  m_fViewPortMaxDepth = -ezMath::MaxValue<float>();
}
