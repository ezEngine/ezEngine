#include <RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

ezGALRenderCommandEncoder::ezGALRenderCommandEncoder(ezGALDevice& device, ezGALCommandEncoderRenderState& renderState, ezGALCommandEncoderCommonPlatformInterface& commonImpl, ezGALCommandEncoderRenderPlatformInterface& renderImpl)
  : ezGALCommandEncoder(device, renderState, commonImpl)
  , m_RenderState(renderState)
  , m_RenderImpl(renderImpl)
{
}

ezGALRenderCommandEncoder::~ezGALRenderCommandEncoder() = default;

void ezGALRenderCommandEncoder::Clear(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, ezUInt8 uiStencilClear /*= 0x0u*/)
{
  AssertRenderingThread();

  m_RenderImpl.ClearPlatform(ClearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

void ezGALRenderCommandEncoder::Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  m_RenderImpl.DrawPlatform(uiVertexCount, uiStartVertex);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();

  m_RenderImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  m_RenderImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for instancing
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_RenderImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  m_RenderImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for instancing
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_RenderImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawAuto()
{
  AssertRenderingThread();
  /// \todo Assert for draw auto support

  m_RenderImpl.DrawAutoPlatform();

  CountDrawCall();
}

void ezGALRenderCommandEncoder::BeginStreamOut()
{
  AssertRenderingThread();
  /// \todo Assert for streamout support

  m_RenderImpl.BeginStreamOutPlatform();
}

void ezGALRenderCommandEncoder::EndStreamOut()
{
  AssertRenderingThread();

  m_RenderImpl.EndStreamOutPlatform();
}

void ezGALRenderCommandEncoder::SetIndexBuffer(ezGALBufferHandle hIndexBuffer)
{
  if (m_RenderState.m_hIndexBuffer == hIndexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetIndexBufferPlatform(pBuffer);

  m_RenderState.m_hIndexBuffer = hIndexBuffer;
  CountStateChange();
}

void ezGALRenderCommandEncoder::SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer)
{
  if (m_RenderState.m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetVertexBufferPlatform(uiSlot, pBuffer);

  m_RenderState.m_hVertexBuffers[uiSlot] = hVertexBuffer;
  CountStateChange();
}

void ezGALRenderCommandEncoder::SetPrimitiveTopology(ezGALPrimitiveTopology::Enum Topology)
{
  AssertRenderingThread();

  if (m_RenderState.m_Topology == Topology)
  {
    CountRedundantStateChange();
    return;
  }

  m_RenderImpl.SetPrimitiveTopologyPlatform(Topology);

  m_RenderState.m_Topology = Topology;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration)
{
  AssertRenderingThread();

  if (m_RenderState.m_hVertexDeclaration == hVertexDeclaration)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALVertexDeclaration* pVertexDeclaration = GetDevice().GetVertexDeclaration(hVertexDeclaration);
  // Assert on vertex buffer type (if non-zero)

  m_RenderImpl.SetVertexDeclarationPlatform(pVertexDeclaration);

  m_RenderState.m_hVertexDeclaration = hVertexDeclaration;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_RenderState.m_hBlendState == hBlendState && m_RenderState.m_BlendFactor.IsEqualRGBA(BlendFactor, 0.001f) && m_RenderState.m_uiSampleMask == uiSampleMask)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALBlendState* pBlendState = GetDevice().GetBlendState(hBlendState);

  m_RenderImpl.SetBlendStatePlatform(pBlendState, BlendFactor, uiSampleMask);

  m_RenderState.m_hBlendState = hBlendState;
  m_RenderState.m_BlendFactor = BlendFactor;
  m_RenderState.m_uiSampleMask = uiSampleMask;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue /*= 0xFFu*/)
{
  AssertRenderingThread();

  if (m_RenderState.m_hDepthStencilState == hDepthStencilState && m_RenderState.m_uiStencilRefValue == uiStencilRefValue)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALDepthStencilState* pDepthStencilState = GetDevice().GetDepthStencilState(hDepthStencilState);

  m_RenderImpl.SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_RenderState.m_hDepthStencilState = hDepthStencilState;
  m_RenderState.m_uiStencilRefValue = uiStencilRefValue;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_RenderState.m_hRasterizerState == hRasterizerState)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALRasterizerState* pRasterizerState = GetDevice().GetRasterizerState(hRasterizerState);

  m_RenderImpl.SetRasterizerStatePlatform(pRasterizerState);

  m_RenderState.m_hRasterizerState = hRasterizerState;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetViewport(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_RenderState.m_ViewPortRect == rect && m_RenderState.m_fViewPortMinDepth == fMinDepth && m_RenderState.m_fViewPortMaxDepth == fMaxDepth)
  {
    CountRedundantStateChange();
    return;
  }

  m_RenderImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_RenderState.m_ViewPortRect = rect;
  m_RenderState.m_fViewPortMinDepth = fMinDepth;
  m_RenderState.m_fViewPortMaxDepth = fMaxDepth;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetScissorRect(const ezRectU32& rect)
{
  AssertRenderingThread();

  if (m_RenderState.m_ScissorRect == rect)
  {
    CountRedundantStateChange();
    return;
  }

  m_RenderImpl.SetScissorRectPlatform(rect);

  m_RenderState.m_ScissorRect = rect;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetStreamOutBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  CountStateChange();
}

void ezGALRenderCommandEncoder::ClearStatisticsCounters()
{
  ezGALCommandEncoder::ClearStatisticsCounters();

  m_uiDrawCalls = 0;
}
