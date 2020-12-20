#include <RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

void ezGALRenderCommandEncoder::Clear(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, ezUInt8 uiStencilClear /*= 0x0u*/)
{
  AssertRenderingThread();

  ClearPlatform(ClearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

void ezGALRenderCommandEncoder::Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  DrawPlatform(uiVertexCount, uiStartVertex);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();

  DrawIndexedPlatform(uiIndexCount, uiStartIndex);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);

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
  DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);

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
  DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void ezGALRenderCommandEncoder::DrawAuto()
{
  AssertRenderingThread();
  /// \todo Assert for draw auto support

  DrawAutoPlatform();

  CountDrawCall();
}

void ezGALRenderCommandEncoder::BeginStreamOut()
{
  AssertRenderingThread();
  /// \todo Assert for streamout support

  BeginStreamOutPlatform();
}

void ezGALRenderCommandEncoder::EndStreamOut()
{
  AssertRenderingThread();

  EndStreamOutPlatform();
}

void ezGALRenderCommandEncoder::SetIndexBuffer(ezGALBufferHandle hIndexBuffer)
{
  if (m_hIndexBuffer == hIndexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  SetIndexBufferPlatform(pBuffer);

  m_hIndexBuffer = hIndexBuffer;
  CountStateChange();
}

void ezGALRenderCommandEncoder::SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer)
{
  if (m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  SetVertexBufferPlatform(uiSlot, pBuffer);

  m_hVertexBuffers[uiSlot] = hVertexBuffer;
  CountStateChange();
}

void ezGALRenderCommandEncoder::SetPrimitiveTopology(ezGALPrimitiveTopology::Enum Topology)
{
  AssertRenderingThread();

  if (m_Topology == Topology)
  {
    CountRedundantStateChange();
    return;
  }

  SetPrimitiveTopologyPlatform(Topology);

  m_Topology = Topology;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration)
{
  AssertRenderingThread();

  if (m_hVertexDeclaration == hVertexDeclaration)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALVertexDeclaration* pVertexDeclaration = GetDevice().GetVertexDeclaration(hVertexDeclaration);
  // Assert on vertex buffer type (if non-zero)

  SetVertexDeclarationPlatform(pVertexDeclaration);

  m_hVertexDeclaration = hVertexDeclaration;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_hBlendState == hBlendState && m_BlendFactor.IsEqualRGBA(BlendFactor, 0.001f) && m_uiSampleMask == uiSampleMask)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALBlendState* pBlendState = GetDevice().GetBlendState(hBlendState);

  SetBlendStatePlatform(pBlendState, BlendFactor, uiSampleMask);

  m_hBlendState = hBlendState;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue /*= 0xFFu*/)
{
  AssertRenderingThread();

  if (m_hDepthStencilState == hDepthStencilState && uiStencilRefValue == m_uiStencilRefValue)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALDepthStencilState* pDepthStencilState = GetDevice().GetDepthStencilState(hDepthStencilState);

  SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_hDepthStencilState = hDepthStencilState;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_hRasterizerState == hRasterizerState)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALRasterizerState* pRasterizerState = GetDevice().GetRasterizerState(hRasterizerState);

  SetRasterizerStatePlatform(pRasterizerState);

  m_hRasterizerState = hRasterizerState;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetViewport(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_ViewPortRect == rect && m_fViewPortMinDepth == fMinDepth && m_fViewPortMaxDepth == fMaxDepth)
  {
    CountRedundantStateChange();
    return;
  }

  SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_ViewPortRect = rect;
  m_fViewPortMinDepth = fMinDepth;
  m_fViewPortMaxDepth = fMaxDepth;

  CountStateChange();
}

void ezGALRenderCommandEncoder::SetScissorRect(const ezRectU32& rect)
{
  AssertRenderingThread();

  if (m_ScissorRect == rect)
  {
    CountRedundantStateChange();
    return;
  }

  SetScissorRectPlatform(rect);

  m_ScissorRect = rect;

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

ezGALRenderCommandEncoder::ezGALRenderCommandEncoder(ezGALDevice& device)
  : ezGALCommandEncoder(device)
{
}

ezGALRenderCommandEncoder::~ezGALRenderCommandEncoder() = default;

void ezGALRenderCommandEncoder::InvalidateState()
{
  ezGALCommandEncoder::InvalidateState();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }
  m_hIndexBuffer.Invalidate();

  m_hVertexDeclaration.Invalidate();
  m_Topology = ezGALPrimitiveTopology::ENUM_COUNT;

  m_hBlendState.Invalidate();
  m_BlendFactor = ezColor(0, 0, 0, 0);
  m_uiSampleMask = 0x0;

  m_hDepthStencilState.Invalidate();
  m_uiStencilRefValue = 0;

  m_hRasterizerState.Invalidate();

  m_ScissorRect = ezRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect = ezRectFloat(ezMath::MaxValue<float>(), ezMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = ezMath::MaxValue<float>();
  m_fViewPortMaxDepth = -ezMath::MaxValue<float>();
}
