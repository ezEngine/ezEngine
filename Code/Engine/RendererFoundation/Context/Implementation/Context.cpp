

#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Logging/Log.h>

ezGALContext::ezGALContext(ezGALDevice* pDevice)
  : m_pDevice(pDevice),
    m_uiDrawCalls(0),
    m_uiDispatchCalls(0),
    m_uiStateChanges(0),
    m_uiRedundantStateChanges(0)
{
  EZ_ASSERT_DEV(pDevice != nullptr, "The context needs a valid device pointer!");

  InvalidateState();
}

ezGALContext::~ezGALContext()
{
}

void ezGALContext::Clear(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/,
                          float fDepthClear /*= 1.0f*/, ezUInt8 uiStencilClear /*= 0x0u*/)
{
  AssertRenderingThread();

  ClearPlatform(ClearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

void ezGALContext::Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices (0, 1, 2, ..)

  DrawPlatform(uiVertexCount, uiStartVertex);

  CountDrawCall();
}

void ezGALContext::DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();

  DrawIndexedPlatform(uiIndexCount, uiStartIndex);

  CountDrawCall();
}

void ezGALContext::DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);

  CountDrawCall();
}

void ezGALContext::DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for instancing
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  ezGALBuffer* pBuffer = m_pDevice->m_Buffers[hIndirectArgumentBuffer];
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void ezGALContext::DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices (0, 1, 2, ..)

  DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);

  CountDrawCall();
}

void ezGALContext::DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for instancing
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  ezGALBuffer* pBuffer = m_pDevice->m_Buffers[hIndirectArgumentBuffer];
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void ezGALContext::DrawAuto()
{
  AssertRenderingThread();
  /// \todo Assert for draw auto support

  DrawAutoPlatform();

  CountDrawCall();
}

void ezGALContext::BeginStreamOut()
{
  AssertRenderingThread();
  /// \todo Assert for streamout support

  BeginStreamOutPlatform();
}

void ezGALContext::EndStreamOut()
{
  AssertRenderingThread();

  BeginStreamOutPlatform();
}



void ezGALContext::Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  AssertRenderingThread();
  /// \todo Assert for compute

  DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  CountDispatchCall();
}

void ezGALContext::DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for compute
  /// \todo Assert for indirect dispatch
  /// \todo Assert offset < buffer size

  ezGALBuffer* pBuffer = m_pDevice->m_Buffers[hIndirectArgumentBuffer];
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDispatchCall();
}


void ezGALContext::SetShader(ezGALShaderHandle hShader)
{
  AssertRenderingThread();
  /// \todo Assert for shader capabilities (supported shader stages etc.)

  if (m_State.m_hShader == hShader)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALShader* pShader = m_pDevice->m_Shaders[hShader];
  EZ_ASSERT_DEV(pShader != nullptr, "The given shader handle isn't valid, this may be a use after destroy!");

  SetShaderPlatform(pShader);

  m_State.m_hShader = hShader;
  CountStateChange();
}

void ezGALContext::SetIndexBuffer(ezGALBufferHandle hIndexBuffer)
{
  if (m_State.m_hIndexBuffer == hIndexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALBuffer* pBuffer = nullptr;
  m_pDevice->m_Buffers.TryGetValue(hIndexBuffer, pBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transformfeedback/indirect-draw/...)

  SetIndexBufferPlatform(pBuffer);

  m_State.m_hIndexBuffer = hIndexBuffer;
  CountStateChange();
}

void ezGALContext::SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer)
{
  if (m_State.m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALBuffer* pBuffer = nullptr;
  m_pDevice->m_Buffers.TryGetValue(hVertexBuffer, pBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transformfeedback/indirect-draw/...)

  SetVertexBufferPlatform(uiSlot, pBuffer);

  m_State.m_hVertexBuffers[uiSlot] = hVertexBuffer;
  CountStateChange();
}

void ezGALContext::SetPrimitiveTopology(ezGALPrimitiveTopology::Enum Topology)
{
  AssertRenderingThread();

  if (m_State.m_Topology == Topology)
  {
    CountRedundantStateChange();
    return;
  }

  SetPrimitiveTopologyPlatform(Topology);

  m_State.m_Topology = Topology;

  CountStateChange();
}

void ezGALContext::SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration)
{
  AssertRenderingThread();

  if (m_State.m_hVertexDeclaration == hVertexDeclaration)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALVertexDeclaration* pVertexDeclaration = nullptr;
  m_pDevice->m_VertexDeclarations.TryGetValue(hVertexDeclaration, pVertexDeclaration);
  // Assert on vertex buffer type (if non-zero)

  SetVertexDeclarationPlatform(pVertexDeclaration);

  m_State.m_hVertexDeclaration = hVertexDeclaration;

  CountStateChange();
}

void ezGALContext::SetConstantBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer)
{
  AssertRenderingThread();
  EZ_ASSERT_RELEASE(uiSlot < EZ_GAL_MAX_CONSTANT_BUFFER_COUNT, "Constant buffer slot index too big!");

  if (m_State.m_hConstantBuffers[uiSlot] == hBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALBuffer* pBuffer = nullptr;
  m_pDevice->m_Buffers.TryGetValue(hBuffer, pBuffer);
  // Assert on constant buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transformfeedback/indirect-draw/...)

  /// \todo Get buffer by handle
  SetConstantBufferPlatform(uiSlot, pBuffer);

  m_State.m_hConstantBuffers[uiSlot] = hBuffer;

  CountStateChange();
}

void ezGALContext::SetSamplerState(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALSamplerStateHandle hSamplerState)
{
  AssertRenderingThread();
  EZ_ASSERT_RELEASE(uiSlot < EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT, "Sampler state slot index too big!");

  if (m_State.m_hSamplerStates[Stage][uiSlot] == hSamplerState)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALSamplerState* pSamplerState = nullptr;
  m_pDevice->m_SamplerStates.TryGetValue(hSamplerState, pSamplerState);

  SetSamplerStatePlatform(Stage, uiSlot, pSamplerState);

  m_State.m_hSamplerStates[Stage][uiSlot] = hSamplerState;

  CountStateChange();
}

void ezGALContext::SetResourceView(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  if (m_State.m_hResourceViews[Stage][uiSlot] == hResourceView)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALResourceView* pResourceView = nullptr;
  m_pDevice->m_ResourceViews.TryGetValue(hResourceView, pResourceView);

  SetResourceViewPlatform(Stage, uiSlot, pResourceView);

  m_State.m_hResourceViews[Stage][uiSlot] = hResourceView;

  CountStateChange();
}

void ezGALContext::SetRenderTargetSetup(const ezGALRenderTagetSetup& RenderTargetSetup)
{
  AssertRenderingThread();

  if (m_State.m_RenderTargetSetup == RenderTargetSetup)
  {
    CountRedundantStateChange();
    return;
  }
  
  ezGALRenderTargetView* ppRenderTargetViews[EZ_GAL_MAX_RENDERTARGET_COUNT] = { nullptr };
  ezGALRenderTargetView* pDepthStencilView = nullptr;

  ezUInt32 uiRenderTargetCount = 0;

  if ( RenderTargetSetup.HasRenderTargets() )
  {
    for ( ezUInt8 uiIndex = 0; uiIndex <= RenderTargetSetup.GetMaxRenderTargetIndex(); ++uiIndex )
    {
      m_pDevice->m_RenderTargetViews.TryGetValue( RenderTargetSetup.GetRenderTarget( uiIndex ), ppRenderTargetViews[uiIndex] );
    }

    uiRenderTargetCount = RenderTargetSetup.GetMaxRenderTargetIndex() + 1;
  }

  m_pDevice->m_RenderTargetViews.TryGetValue( RenderTargetSetup.GetDepthStencilTarget(), pDepthStencilView );

  SetRenderTargetSetupPlatform( ppRenderTargetViews, uiRenderTargetCount, pDepthStencilView );

  m_State.m_RenderTargetSetup = RenderTargetSetup;

  CountStateChange();
}

void ezGALContext::SetUnorderedAccessView(ezUInt32 uiSlot, ezGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();
  /// \todo
  EZ_REPORT_FAILURE("not implemented");
}

void ezGALContext::SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_State.m_hBlendState == hBlendState && m_State.m_BlendFactor.IsEqualRGBA(BlendFactor, 0.001f) && m_State.m_uiSampleMask == uiSampleMask)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALBlendState* pBlendState = nullptr;
  m_pDevice->m_BlendStates.TryGetValue(hBlendState, pBlendState);

  SetBlendStatePlatform(pBlendState, BlendFactor, uiSampleMask);

  m_State.m_hBlendState = hBlendState;

  CountStateChange();
}

void ezGALContext::SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue /*= 0xFFu*/)
{
  AssertRenderingThread();

  if (m_State.m_hDepthStencilState == hDepthStencilState && uiStencilRefValue == m_State.m_uiStencilRefValue)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALDepthStencilState* pDepthStencilState = nullptr;
  m_pDevice->m_DepthStencilStates.TryGetValue(hDepthStencilState, pDepthStencilState);

  SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_State.m_hDepthStencilState = hDepthStencilState;

  CountStateChange();
}

void ezGALContext::SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_State.m_hRasterizerState == hRasterizerState)
  {
    CountRedundantStateChange();
    return;
  }

  ezGALRasterizerState* pRasterizerState = nullptr;
  m_pDevice->m_RasterizerStates.TryGetValue(hRasterizerState, pRasterizerState);

  SetRasterizerStatePlatform(pRasterizerState);

  m_State.m_hRasterizerState = hRasterizerState;

  CountStateChange();
}

void ezGALContext::SetViewport(float fX, float fY, float fWidth, float fHeight, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  const ezRectFloat& CurrentRect = m_State.m_ViewPortRect;

  // Epsilon compare necessary? Recommended?
  if (CurrentRect.x == fX && CurrentRect.y == fY && CurrentRect.width == fWidth && CurrentRect.height == fHeight && m_State.m_fViewPortMinDepth == fMinDepth && m_State.m_fViewPortMaxDepth == fMaxDepth)
  {
    CountRedundantStateChange();
    return;
  }

  SetViewportPlatform(fX, fY, fWidth, fHeight, fMinDepth, fMaxDepth);

  m_State.m_ViewPortRect = ezRectFloat(fX, fY, fWidth, fHeight);
  m_State.m_fViewPortMinDepth = fMinDepth;
  m_State.m_fViewPortMaxDepth = fMaxDepth;

  CountStateChange();
}

void ezGALContext::SetScissorRect(ezUInt32 uiX, ezUInt32 uiY, ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  AssertRenderingThread();

  const ezRectU32& CurrentRect = m_State.m_ScissorRect;

  if (CurrentRect.x == uiX && CurrentRect.y == uiY && CurrentRect.width == uiWidth && CurrentRect.height == uiHeight)
  {
    CountRedundantStateChange();
    return;
  }

  SetScissorRectPlatform(uiX, uiY, uiWidth, uiHeight);

  m_State.m_ScissorRect = ezRectU32(uiX, uiY, uiWidth, uiHeight);

  CountStateChange();
}

void ezGALContext::SetStreamOutBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer, ezUInt32 uiOffset)
{
  EZ_REPORT_FAILURE("not implemented");

  CountStateChange();
}


void ezGALContext::InsertFence(ezGALFenceHandle hFence)
{
  AssertRenderingThread();

  InsertFencePlatform(m_pDevice->m_Fences[hFence]);
}

bool ezGALContext::IsFenceReached(ezGALFenceHandle hFence)
{
  AssertRenderingThread();

  return IsFenceReachedPlatform(m_pDevice->m_Fences[hFence]);
}

void ezGALContext::WaitForFence(ezGALFenceHandle hFence)
{
  AssertRenderingThread();

  m_pDevice->Flush(); /// \todo - make this toggle-able

  ezGALFence* pPlatformSpecificFence = m_pDevice->m_Fences[hFence];

  while (!IsFenceReachedPlatform(pPlatformSpecificFence))
  {
    ezThreadUtils::YieldTimeSlice(); /// \todo Spin lock count perhaps?
  }
}

void ezGALContext::BeginQuery(ezGALQueryHandle hQuery)
{
  AssertRenderingThread();
  /// \todo Assert on query support?

  BeginQueryPlatform(m_pDevice->m_Queries[hQuery]);
}

void ezGALContext::EndQuery(ezGALQueryHandle hQuery)
{
  AssertRenderingThread();
  /// \todo Assert on query support?
  /// \todo Assert on query started

  EndQueryPlatform(m_pDevice->m_Queries[hQuery]);
}

void ezGALContext::CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource)
{
  AssertRenderingThread();

  ezGALBuffer* pDest = nullptr;
  ezGALBuffer* pSource = nullptr;

  if (m_pDevice->m_Buffers.TryGetValue(hDest, pDest) && m_pDevice->m_Buffers.TryGetValue(hSource, pSource))
  {
    CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = %p, source = %p", pDest, pSource);
  }
}

void ezGALContext::CopyBufferRegion(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezGALBufferHandle hSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  AssertRenderingThread();

  ezGALBuffer* pDest = nullptr;
  ezGALBuffer* pSource = nullptr;

  if (m_pDevice->m_Buffers.TryGetValue(hDest, pDest) && m_pDevice->m_Buffers.TryGetValue(hSource, pSource))
  {
    const ezUInt32 uiDestSize = pDest->GetSize();
    const ezUInt32 uiSourceSize = pSource->GetSize();

    EZ_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    EZ_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

    CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = %p, source = %p", pDest, pSource);
  }
}

void ezGALContext::UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, const void* pSourceData, ezUInt32 uiByteCount)
{
  AssertRenderingThread();

  EZ_VERIFY(pSourceData != nullptr, "Source data pointer for buffer update is invalid!");

  ezGALBuffer* pDest = nullptr;

  if (m_pDevice->m_Buffers.TryGetValue(hDest, pDest))
  {
    EZ_VERIFY(pDest->GetSize() >= (uiDestOffset + uiByteCount), "Buffer is too small (or offset too big)");
    UpdateBufferPlatform(pDest, uiDestOffset, pSourceData, uiByteCount);
  }
  else
  {
    EZ_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void ezGALContext::CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource)
{
  AssertRenderingThread();

  ezGALTexture* pDest = nullptr;
  ezGALTexture* pSource = nullptr;

  if (m_pDevice->m_Textures.TryGetValue(hDest, pDest) && m_pDevice->m_Textures.TryGetValue(hSource, pSource))
  {
    CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = %p, source = %p", pDest, pSource);
  }
}

void ezGALContext::CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, ezGALTextureHandle hSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  AssertRenderingThread();

  /// \todo
  EZ_REPORT_FAILURE("Not implemented!");
}

void ezGALContext::UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const void* pSourceData, ezUInt32 uiSourceRowPitch, ezUInt32 uiSourceDepthPitch)
{
  AssertRenderingThread();

  EZ_REPORT_FAILURE("Not implemented!");
}

void ezGALContext::ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, ezGALTextureHandle hSource, const ezGALTextureSubresource& SourceSubResource)
{
  AssertRenderingThread();

  EZ_REPORT_FAILURE("Not implemented!");
}

void ezGALContext::ReadbackTexture(ezGALTextureHandle hTexture)
{
  AssertRenderingThread();

  ezGALTexture* pTexture = nullptr;

  if (m_pDevice->m_Textures.TryGetValue(hTexture, pTexture))
  {
    EZ_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack, "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    ReadbackTexturePlatform(pTexture);
  }
}

void ezGALContext::CopyTextureReadbackResult(ezGALTextureHandle hTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  AssertRenderingThread();

  ezGALTexture* pTexture = nullptr;

  if (m_pDevice->m_Textures.TryGetValue(hTexture, pTexture))
  {
    EZ_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack, "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    CopyTextureReadbackResultPlatform(pTexture, pData);
  }
}

// Debug helper functions

void ezGALContext::PushMarker(const char* Marker)
{
  AssertRenderingThread();

  EZ_ASSERT_DEV(Marker != nullptr, "Invalid marker!");

  PushMarkerPlatform(Marker);
}

void ezGALContext::PopMarker()
{
  AssertRenderingThread();

  PopMarkerPlatform();
}

void ezGALContext::InsertEventMarker(const char* Marker)
{
  AssertRenderingThread();

  EZ_ASSERT_DEV(Marker != nullptr, "Invalid marker!");

  InsertEventMarkerPlatform(Marker);
}

void ezGALContext::ClearStatisticsCounters()
{
  // Reset counters for various statistics
  m_uiDrawCalls = 0;
  m_uiDispatchCalls = 0;
  m_uiStateChanges = 0;
  m_uiRedundantStateChanges = 0;
}

void ezGALContext::InvalidateState()
{
  m_State.Invalidate();
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Context_Implementation_Context);

