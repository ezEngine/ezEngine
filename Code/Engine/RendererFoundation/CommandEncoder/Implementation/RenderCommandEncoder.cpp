#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

ezGALRenderCommandEncoder::ezGALRenderCommandEncoder(ezGALDevice& ref_device, ezGALCommandEncoderRenderState& ref_renderState, ezGALCommandEncoderCommonPlatformInterface& ref_commonImpl, ezGALCommandEncoderRenderPlatformInterface& ref_renderImpl)
  : ezGALCommandEncoder(ref_device, ref_renderState, ref_commonImpl)
  , m_RenderState(ref_renderState)
  , m_RenderImpl(ref_renderImpl)
{
}

ezGALRenderCommandEncoder::~ezGALRenderCommandEncoder() = default;

void ezGALRenderCommandEncoder::Clear(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, ezUInt8 uiStencilClear /*= 0x0u*/)
{
  AssertRenderingThread();
  m_RenderImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

ezResult ezGALRenderCommandEncoder::Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawPlatform(uiVertexCount, uiStartVertex);
}

ezResult ezGALRenderCommandEncoder::DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

ezResult ezGALRenderCommandEncoder::DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

ezResult ezGALRenderCommandEncoder::DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_RenderImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

ezResult ezGALRenderCommandEncoder::DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

ezResult ezGALRenderCommandEncoder::DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_RenderImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void ezGALRenderCommandEncoder::SetIndexBuffer(ezGALBufferHandle hIndexBuffer)
{
  if (m_RenderState.m_hIndexBuffer == hIndexBuffer)
  {
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetIndexBufferPlatform(pBuffer);

  m_RenderState.m_hIndexBuffer = hIndexBuffer;
}

void ezGALRenderCommandEncoder::SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer)
{
  if (m_RenderState.m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetVertexBufferPlatform(uiSlot, pBuffer);

  m_RenderState.m_hVertexBuffers[uiSlot] = hVertexBuffer;
}

void ezGALRenderCommandEncoder::SetPrimitiveTopology(ezGALPrimitiveTopology::Enum topology)
{
  AssertRenderingThread();

  if (m_RenderState.m_Topology == topology)
  {
    return;
  }

  m_RenderImpl.SetPrimitiveTopologyPlatform(topology);

  m_RenderState.m_Topology = topology;
}

void ezGALRenderCommandEncoder::SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration)
{
  AssertRenderingThread();

  if (m_RenderState.m_hVertexDeclaration == hVertexDeclaration)
  {
    return;
  }

  const ezGALVertexDeclaration* pVertexDeclaration = GetDevice().GetVertexDeclaration(hVertexDeclaration);
  // Assert on vertex buffer type (if non-zero)

  m_RenderImpl.SetVertexDeclarationPlatform(pVertexDeclaration);

  m_RenderState.m_hVertexDeclaration = hVertexDeclaration;
}

void ezGALRenderCommandEncoder::SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& blendFactor, ezUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_RenderState.m_hBlendState == hBlendState && m_RenderState.m_BlendFactor.IsEqualRGBA(blendFactor, 0.001f) && m_RenderState.m_uiSampleMask == uiSampleMask)
  {
    return;
  }

  const ezGALBlendState* pBlendState = GetDevice().GetBlendState(hBlendState);

  m_RenderImpl.SetBlendStatePlatform(pBlendState, blendFactor, uiSampleMask);

  m_RenderState.m_hBlendState = hBlendState;
  m_RenderState.m_BlendFactor = blendFactor;
  m_RenderState.m_uiSampleMask = uiSampleMask;
}

void ezGALRenderCommandEncoder::SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue /*= 0xFFu*/)
{
  AssertRenderingThread();

  if (m_RenderState.m_hDepthStencilState == hDepthStencilState && m_RenderState.m_uiStencilRefValue == uiStencilRefValue)
  {
    return;
  }

  const ezGALDepthStencilState* pDepthStencilState = GetDevice().GetDepthStencilState(hDepthStencilState);

  m_RenderImpl.SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_RenderState.m_hDepthStencilState = hDepthStencilState;
  m_RenderState.m_uiStencilRefValue = uiStencilRefValue;
}

void ezGALRenderCommandEncoder::SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_RenderState.m_hRasterizerState == hRasterizerState)
  {
    return;
  }

  const ezGALRasterizerState* pRasterizerState = GetDevice().GetRasterizerState(hRasterizerState);

  m_RenderImpl.SetRasterizerStatePlatform(pRasterizerState);

  m_RenderState.m_hRasterizerState = hRasterizerState;
}

void ezGALRenderCommandEncoder::SetViewport(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_RenderState.m_ViewPortRect == rect && m_RenderState.m_fViewPortMinDepth == fMinDepth && m_RenderState.m_fViewPortMaxDepth == fMaxDepth)
  {
    return;
  }

  m_RenderImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_RenderState.m_ViewPortRect = rect;
  m_RenderState.m_fViewPortMinDepth = fMinDepth;
  m_RenderState.m_fViewPortMaxDepth = fMaxDepth;
}

void ezGALRenderCommandEncoder::SetScissorRect(const ezRectU32& rect)
{
  AssertRenderingThread();

  if (m_RenderState.m_ScissorRect == rect)
  {
    return;
  }

  m_RenderImpl.SetScissorRectPlatform(rect);

  m_RenderState.m_ScissorRect = rect;
}

void ezGALRenderCommandEncoder::ClearStatisticsCounters()
{
  ezGALCommandEncoder::ClearStatisticsCounters();

  m_uiDrawCalls = 0;
}
