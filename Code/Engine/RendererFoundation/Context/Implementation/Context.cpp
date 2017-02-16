

#include <PCH.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>
#include <RendererFoundation/Resources/Query.h>

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

void ezGALContext::ClearUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 clearValues)
{
  AssertRenderingThread();

  const ezGALUnorderedAccessView* pUnorderedAccessView = m_pDevice->GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  ClearUnorderedAccessViewPlatform(pUnorderedAccessView, clearValues);
}

void ezGALContext::ClearUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 clearValues)
{
  AssertRenderingThread();

  const ezGALUnorderedAccessView* pUnorderedAccessView = m_pDevice->GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  ClearUnorderedAccessViewPlatform(pUnorderedAccessView, clearValues);
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

  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
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

  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
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

  EZ_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

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

  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
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

  const ezGALShader* pShader = m_pDevice->GetShader(hShader);
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

  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

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

  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

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

  const ezGALVertexDeclaration* pVertexDeclaration = m_pDevice->GetVertexDeclaration(hVertexDeclaration);
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

  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  EZ_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BufferType == ezGALBufferType::ConstantBuffer, "Wrong buffer type");

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

  const ezGALSamplerState* pSamplerState = m_pDevice->GetSamplerState(hSamplerState);

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

  const ezGALResourceView* pResourceView = m_pDevice->GetResourceView(hResourceView);

  SetResourceViewPlatform(Stage, uiSlot, pResourceView);

  m_State.m_hResourceViews[Stage][uiSlot] = hResourceView;
  m_State.m_pResourcesForResourceViews[Stage][uiSlot] = pResourceView != nullptr ? pResourceView->GetResource() : nullptr;

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

  const ezGALRenderTargetView* pRenderTargetViews[EZ_GAL_MAX_RENDERTARGET_COUNT] = { nullptr };
  const ezGALRenderTargetView* pDepthStencilView = nullptr;

  ezUInt32 uiRenderTargetCount = 0;

  bool bFlushNeeded = false;

  if ( RenderTargetSetup.HasRenderTargets() )
  {
    for ( ezUInt8 uiIndex = 0; uiIndex <= RenderTargetSetup.GetMaxRenderTargetIndex(); ++uiIndex )
    {
      const ezGALRenderTargetView* pRenderTargetView = m_pDevice->GetRenderTargetView(RenderTargetSetup.GetRenderTarget(uiIndex));
      if (pRenderTargetView != nullptr)
      {
        bFlushNeeded |= UnsetResourceViews(pRenderTargetView->GetTexture());
        bFlushNeeded |= UnsetUnorderedAccessViews(pRenderTargetView->GetTexture());
      }

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    uiRenderTargetCount = RenderTargetSetup.GetMaxRenderTargetIndex() + 1;
  }

  pDepthStencilView = m_pDevice->GetRenderTargetView(RenderTargetSetup.GetDepthStencilTarget());
  if (pDepthStencilView != nullptr)
  {
    bFlushNeeded |= UnsetResourceViews(pDepthStencilView->GetTexture());
    bFlushNeeded |= UnsetUnorderedAccessViews(pDepthStencilView->GetTexture());
  }

  if (bFlushNeeded)
  {
    FlushPlatform();
  }
  SetRenderTargetSetupPlatform( ezMakeArrayPtr(pRenderTargetViews, uiRenderTargetCount), pDepthStencilView );

  m_State.m_RenderTargetSetup = RenderTargetSetup;

  CountStateChange();
}

void ezGALContext::SetUnorderedAccessView(ezUInt32 uiSlot, ezGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  if (m_State.m_hUnorderedAccessViews[uiSlot] == hUnorderedAccessView)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALUnorderedAccessView* pUnorderedAccessView = m_pDevice->GetUnorderedAccessView(hUnorderedAccessView);

  SetUnorderedAccessViewPlatform(uiSlot, pUnorderedAccessView);

  m_State.m_hUnorderedAccessViews[uiSlot] = hUnorderedAccessView;
  m_State.m_pResourcesForUnorderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? pUnorderedAccessView->GetResource() : nullptr;

  CountStateChange();
}

void ezGALContext::SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_State.m_hBlendState == hBlendState && m_State.m_BlendFactor.IsEqualRGBA(BlendFactor, 0.001f) && m_State.m_uiSampleMask == uiSampleMask)
  {
    CountRedundantStateChange();
    return;
  }

  const ezGALBlendState* pBlendState = m_pDevice->GetBlendState(hBlendState);

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

  const ezGALDepthStencilState* pDepthStencilState = m_pDevice->GetDepthStencilState(hDepthStencilState);

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

  const ezGALRasterizerState* pRasterizerState = m_pDevice->GetRasterizerState(hRasterizerState);

  SetRasterizerStatePlatform(pRasterizerState);

  m_State.m_hRasterizerState = hRasterizerState;

  CountStateChange();
}

void ezGALContext::SetViewport(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_State.m_ViewPortRect == rect && m_State.m_fViewPortMinDepth == fMinDepth && m_State.m_fViewPortMaxDepth == fMaxDepth)
  {
    CountRedundantStateChange();
    return;
  }

  SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_State.m_ViewPortRect = rect;
  m_State.m_fViewPortMinDepth = fMinDepth;
  m_State.m_fViewPortMaxDepth = fMaxDepth;

  CountStateChange();
}

void ezGALContext::SetScissorRect(const ezRectU32& rect)
{
  AssertRenderingThread();

  if (m_State.m_ScissorRect == rect)
  {
    CountRedundantStateChange();
    return;
  }

  SetScissorRectPlatform(rect);

  m_State.m_ScissorRect = rect;

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

  InsertFencePlatform(m_pDevice->GetFence(hFence));
}

bool ezGALContext::IsFenceReached(ezGALFenceHandle hFence)
{
  AssertRenderingThread();

  return IsFenceReachedPlatform(m_pDevice->GetFence(hFence));
}

void ezGALContext::WaitForFence(ezGALFenceHandle hFence)
{
  AssertRenderingThread();

  WaitForFencePlatform(m_pDevice->GetFence(hFence));
}

void ezGALContext::BeginQuery(ezGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_pDevice->GetQuery(hQuery);
  EZ_ASSERT_DEV(query->GetDescription().m_type != ezGALQueryType::Timestamp, "You can only call 'EndQuery' on queries of type ezGALQueryType::Timestamp.");
  EZ_ASSERT_DEV(!query->m_bStarted, "Can't stat ezGALQuery because it is already running.");

  BeginQueryPlatform(query);
}

void ezGALContext::EndQuery(ezGALQueryHandle hQuery)
{
  AssertRenderingThread();
  
  auto query = m_pDevice->GetQuery(hQuery);
  EZ_ASSERT_DEV(query->m_bStarted || query->GetDescription().m_type == ezGALQueryType::Timestamp, "Can't end ezGALQuery, query hasn't started yet.");

  EndQueryPlatform(query);
}

ezResult ezGALContext::GetQueryResult(ezGALQueryHandle hQuery, ezUInt64& uiQueryResult)
{
  AssertRenderingThread();

  auto query = m_pDevice->GetQuery(hQuery);
  EZ_ASSERT_DEV(!query->m_bStarted, "Can't retrieve data from ezGALQuery while query is still running.");

  return GetQueryResultPlatform(query, uiQueryResult);
}

void ezGALContext::CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource)
{
  AssertRenderingThread();

  const ezGALBuffer* pDest = m_pDevice->GetBuffer(hDest);
  const ezGALBuffer* pSource = m_pDevice->GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALContext::CopyBufferRegion(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezGALBufferHandle hSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  AssertRenderingThread();

  const ezGALBuffer* pDest = m_pDevice->GetBuffer(hDest);
  const ezGALBuffer* pSource = m_pDevice->GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    const ezUInt32 uiDestSize = pDest->GetSize();
    const ezUInt32 uiSourceSize = pSource->GetSize();

    EZ_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    EZ_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

    CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALContext::UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData)
{
  AssertRenderingThread();

  EZ_VERIFY(!pSourceData.IsEmpty(), "Source data for buffer update is invalid!");

  const ezGALBuffer* pDest = m_pDevice->GetBuffer(hDest);

  if (pDest != nullptr)
  {
    EZ_VERIFY(pDest->GetSize() >= (uiDestOffset + pSourceData.GetCount()), "Buffer is too small (or offset too big)");
    UpdateBufferPlatform(pDest, uiDestOffset, pSourceData);
  }
  else
  {
    EZ_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void ezGALContext::CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_pDevice->GetTexture(hDest);
  const ezGALTexture* pSource = m_pDevice->GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALContext::CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, ezGALTextureHandle hSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_pDevice->GetTexture(hDest);
  const ezGALTexture* pSource = m_pDevice->GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    CopyTextureRegionPlatform(pDest, DestinationSubResource, DestinationPoint, pSource, SourceSubResource, Box);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALContext::UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_pDevice->GetTexture(hDest);

  if (pDest != nullptr)
  {
    UpdateTexturePlatform(pDest, DestinationSubResource, DestinationBox, pSourceData);
  }
  else
  {
    EZ_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}", ezArgP(pDest));
  }
}

void ezGALContext::ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, ezGALTextureHandle hSource, const ezGALTextureSubresource& SourceSubResource)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_pDevice->GetTexture(hDest);
  const ezGALTexture* pSource = m_pDevice->GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    ResolveTexturePlatform(pDest, DestinationSubResource, pSource, SourceSubResource);
  }
  else
  {
    EZ_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALContext::ReadbackTexture(ezGALTextureHandle hTexture)
{
  AssertRenderingThread();

  const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    EZ_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack, "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    ReadbackTexturePlatform(pTexture);
  }
}

void ezGALContext::CopyTextureReadbackResult(ezGALTextureHandle hTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  AssertRenderingThread();

  const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    EZ_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack, "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    CopyTextureReadbackResultPlatform(pTexture, pData);
  }
}


void ezGALContext::Flush()
{
  AssertRenderingThread();

  FlushPlatform();
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

bool ezGALContext::UnsetResourceViews(const ezGALResourceBase* pResource)
{
  bool bResult = false;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (ezUInt32 uiSlot = 0; uiSlot < EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT; ++uiSlot)
    {
      if (m_State.m_pResourcesForResourceViews[stage][uiSlot] == pResource)
      {
        SetResourceViewPlatform((ezGALShaderStage::Enum)stage, uiSlot, nullptr);

        m_State.m_hResourceViews[stage][uiSlot].Invalidate();
        m_State.m_pResourcesForResourceViews[stage][uiSlot] = nullptr;

        bResult = true;
      }
    }
  }

  return bResult;
}

bool ezGALContext::UnsetUnorderedAccessViews(const ezGALResourceBase* pResource)
{
  bool bResult = false;

  for (ezUInt32 uiSlot = 0; uiSlot < EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT; ++uiSlot)
  {
    if (m_State.m_pResourcesForUnorderedAccessViews[uiSlot] == pResource)
    {
      SetUnorderedAccessViewPlatform(uiSlot, nullptr);

      m_State.m_hUnorderedAccessViews[uiSlot].Invalidate();
      m_State.m_pResourcesForUnorderedAccessViews[uiSlot] = nullptr;

      bResult = true;
    }
  }

  return bResult;
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Context_Implementation_Context);

