#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

void ezGALCommandEncoder::SetShader(ezGALShaderHandle hShader)
{
  AssertRenderingThread();
  /// \todo Assert for shader capabilities (supported shader stages etc.)

  if (m_State.m_hShader == hShader)
    return;

  const ezGALShader* pShader = m_Device.GetShader(hShader);
  EZ_ASSERT_DEV(pShader != nullptr, "The given shader handle isn't valid, this may be a use after destroy!");

  m_CommonImpl.SetShaderPlatform(pShader);

  m_State.m_hShader = hShader;
}

void ezGALCommandEncoder::SetConstantBuffer(const ezShaderResourceBinding& binding, ezGALBufferHandle hBuffer)
{
  AssertRenderingThread();

  const ezGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
  EZ_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::ConstantBuffer), "Wrong buffer type");

  m_CommonImpl.SetConstantBufferPlatform(binding, pBuffer);
}

void ezGALCommandEncoder::SetSamplerState(const ezShaderResourceBinding& binding, ezGALSamplerStateHandle hSamplerState)
{
  AssertRenderingThread();

  const ezGALSamplerState* pSamplerState = m_Device.GetSamplerState(hSamplerState);

  m_CommonImpl.SetSamplerStatePlatform(binding, pSamplerState);
}

void ezGALCommandEncoder::SetResourceView(const ezShaderResourceBinding& binding, ezGALTextureResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const ezGALTextureResourceView* pResourceView = m_Device.GetResourceView(hResourceView);

  m_CommonImpl.SetResourceViewPlatform(binding, pResourceView);
}

void ezGALCommandEncoder::SetResourceView(const ezShaderResourceBinding& binding, ezGALBufferResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const ezGALBufferResourceView* pResourceView = m_Device.GetResourceView(hResourceView);

  m_CommonImpl.SetResourceViewPlatform(binding, pResourceView);
}

void ezGALCommandEncoder::SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  const ezGALTextureUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  m_CommonImpl.SetUnorderedAccessViewPlatform(binding, pUnorderedAccessView);
}

void ezGALCommandEncoder::SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  const ezGALBufferUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  m_CommonImpl.SetUnorderedAccessViewPlatform(binding, pUnorderedAccessView);
}

void ezGALCommandEncoder::SetPushConstants(ezArrayPtr<const ezUInt8> data)
{
  AssertRenderingThread();
  m_CommonImpl.SetPushConstantsPlatform(data);
}

ezGALTimestampHandle ezGALCommandEncoder::InsertTimestamp()
{
  AssertRenderingThread();
  return m_CommonImpl.InsertTimestampPlatform();
}

ezGALOcclusionHandle ezGALCommandEncoder::BeginOcclusionQuery(ezEnum<ezGALQueryType> type)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Occlusion queries can only be started within a render scope");
  AssertRenderingThread();
  ezGALOcclusionHandle hOcclusion = m_CommonImpl.BeginOcclusionQueryPlatform(type);

  EZ_ASSERT_DEBUG(m_PendingOcclusionQuery.IsInvalidated(), "Only one occusion query can be active at any give time.");
  m_PendingOcclusionQuery = hOcclusion;

  return hOcclusion;
}

void ezGALCommandEncoder::EndOcclusionQuery(ezGALOcclusionHandle hOcclusion)
{
  AssertRenderingThread();
  m_CommonImpl.EndOcclusionQueryPlatform(hOcclusion);

  EZ_ASSERT_DEBUG(m_PendingOcclusionQuery == hOcclusion, "The EndOcclusionQuery parameter does not match the currently started query");
  m_PendingOcclusionQuery = {};
}

ezGALFenceHandle ezGALCommandEncoder::InsertFence()
{
  return m_CommonImpl.InsertFencePlatform();
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues)
{
  AssertRenderingThread();

  const ezGALTextureUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues)
{
  AssertRenderingThread();

  const ezGALBufferUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues)
{
  AssertRenderingThread();

  const ezGALTextureUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues)
{
  AssertRenderingThread();

  const ezGALBufferUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource)
{
  AssertRenderingThread();

  const ezGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const ezGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::CopyBufferRegion(
  ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezGALBufferHandle hSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  AssertRenderingThread();

  const ezGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const ezGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    const ezUInt32 uiDestSize = pDest->GetSize();
    const ezUInt32 uiSourceSize = pSource->GetSize();

    EZ_IGNORE_UNUSED(uiDestSize);
    EZ_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    EZ_IGNORE_UNUSED(uiSourceSize);
    EZ_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

    m_CommonImpl.CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode)
{
  AssertRenderingThread();

  EZ_ASSERT_DEV(!sourceData.IsEmpty(), "Source data for buffer update is invalid!");

  const ezGALBuffer* pDest = m_Device.GetBuffer(hDest);

  if (pDest != nullptr)
  {
    if (updateMode == ezGALUpdateMode::NoOverwrite && !(GetDevice().GetCapabilities().m_bNoOverwriteBufferUpdate))
    {
      updateMode = ezGALUpdateMode::CopyToTempStorage;
    }

    EZ_ASSERT_DEV(pDest->GetSize() >= (uiDestOffset + sourceData.GetCount()), "Buffer {} is too small (or offset {} too big) for {} bytes", pDest->GetSize(), uiDestOffset, sourceData.GetCount());
    m_CommonImpl.UpdateBufferPlatform(pDest, uiDestOffset, sourceData, updateMode);
  }
  else
  {
    EZ_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void ezGALCommandEncoder::CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);
  const ezGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource,
  const ezVec3U32& vDestinationPoint, ezGALTextureHandle hSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);
  const ezGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTextureRegionPlatform(pDest, destinationSubResource, vDestinationPoint, pSource, sourceSubResource, box);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource,
  const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);

  if (pDest != nullptr)
  {
    m_CommonImpl.UpdateTexturePlatform(pDest, destinationSubResource, destinationBox, sourceData);
  }
  else
  {
    EZ_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}", ezArgP(pDest));
  }
}

void ezGALCommandEncoder::ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, ezGALTextureHandle hSource,
  const ezGALTextureSubresource& sourceSubResource)
{
  AssertRenderingThread();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);
  const ezGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ResolveTexturePlatform(pDest, destinationSubResource, pSource, sourceSubResource);
  }
  else
  {
    EZ_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::ReadbackTexture(ezGALTextureHandle hTexture)
{
  AssertRenderingThread();

  const ezGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    EZ_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
      "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.ReadbackTexturePlatform(pTexture);
  }
}

void ezGALCommandEncoder::CopyTextureReadbackResult(ezGALTextureHandle hTexture, ezArrayPtr<ezGALTextureSubresource> sourceSubResource, ezArrayPtr<ezGALSystemMemoryDescription> targetData)
{
  AssertRenderingThread();

  const ezGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    EZ_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
      "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.CopyTextureReadbackResultPlatform(pTexture, sourceSubResource, targetData);
  }
}

void ezGALCommandEncoder::GenerateMipMaps(ezGALTextureResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const ezGALTextureResourceView* pResourceView = m_Device.GetResourceView(hResourceView);
  if (pResourceView != nullptr)
  {
    EZ_ASSERT_DEV(!pResourceView->GetDescription().m_hTexture.IsInvalidated(), "Resource view needs a valid texture to generate mip maps.");
    const ezGALTexture* pTexture = m_Device.GetTexture(pResourceView->GetDescription().m_hTexture);
    EZ_IGNORE_UNUSED(pTexture);
    EZ_ASSERT_DEV(pTexture->GetDescription().m_bAllowDynamicMipGeneration,
      "Dynamic mip map generation needs to be enabled (m_bAllowDynamicMipGeneration = true)!");

    m_CommonImpl.GenerateMipMapsPlatform(pResourceView);
  }
}

void ezGALCommandEncoder::Flush()
{
  AssertRenderingThread();
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render, "Flush can't be called inside a rendering scope");

  m_CommonImpl.FlushPlatform();
}

// Debug helper functions

void ezGALCommandEncoder::PushMarker(const char* szMarker)
{
  AssertRenderingThread();

  EZ_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.PushMarkerPlatform(szMarker);
}

void ezGALCommandEncoder::PopMarker()
{
  AssertRenderingThread();

  m_CommonImpl.PopMarkerPlatform();
}

void ezGALCommandEncoder::InsertEventMarker(const char* szMarker)
{
  AssertRenderingThread();

  EZ_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.InsertEventMarkerPlatform(szMarker);
}

ezGALCommandEncoder::ezGALCommandEncoder(ezGALDevice& ref_device, ezGALCommandEncoderCommonPlatformInterface& ref_commonImpl)
  : m_Device(ref_device)
  , m_CommonImpl(ref_commonImpl)
{
}

ezGALCommandEncoder::~ezGALCommandEncoder() = default;

void ezGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
}

ezResult ezGALCommandEncoder::Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  EZ_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

  CountDispatchCall();
  return m_CommonImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

ezResult ezGALCommandEncoder::DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  CountDispatchCall();
  return m_CommonImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void ezGALCommandEncoder::ClearStatisticsCounters()
{
  m_uiDrawCalls = 0;
  m_uiDispatchCalls = 0;
}

void ezGALCommandEncoder::Clear(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, ezUInt8 uiStencilClear /*= 0x0u*/)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  m_CommonImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

ezResult ezGALCommandEncoder::Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawPlatform(uiVertexCount, uiStartVertex);
}

ezResult ezGALCommandEncoder::DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

ezResult ezGALCommandEncoder::DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

ezResult ezGALCommandEncoder::DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_CommonImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

ezResult ezGALCommandEncoder::DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

ezResult ezGALCommandEncoder::DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_CommonImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void ezGALCommandEncoder::SetIndexBuffer(ezGALBufferHandle hIndexBuffer)
{
  if (m_State.m_hIndexBuffer == hIndexBuffer)
  {
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_CommonImpl.SetIndexBufferPlatform(pBuffer);

  m_State.m_hIndexBuffer = hIndexBuffer;
}

void ezGALCommandEncoder::SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer)
{
  if (m_State.m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_CommonImpl.SetVertexBufferPlatform(uiSlot, pBuffer);

  m_State.m_hVertexBuffers[uiSlot] = hVertexBuffer;
}

void ezGALCommandEncoder::SetPrimitiveTopology(ezGALPrimitiveTopology::Enum topology)
{
  AssertRenderingThread();

  if (m_State.m_Topology == topology)
  {
    return;
  }

  m_CommonImpl.SetPrimitiveTopologyPlatform(topology);

  m_State.m_Topology = topology;
}

void ezGALCommandEncoder::SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration)
{
  AssertRenderingThread();

  if (m_State.m_hVertexDeclaration == hVertexDeclaration)
  {
    return;
  }

  const ezGALVertexDeclaration* pVertexDeclaration = GetDevice().GetVertexDeclaration(hVertexDeclaration);
  // Assert on vertex buffer type (if non-zero)

  m_CommonImpl.SetVertexDeclarationPlatform(pVertexDeclaration);

  m_State.m_hVertexDeclaration = hVertexDeclaration;
}

void ezGALCommandEncoder::SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& blendFactor, ezUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_State.m_hBlendState == hBlendState && m_State.m_BlendFactor.IsEqualRGBA(blendFactor, 0.001f) && m_State.m_uiSampleMask == uiSampleMask)
  {
    return;
  }

  const ezGALBlendState* pBlendState = GetDevice().GetBlendState(hBlendState);

  m_CommonImpl.SetBlendStatePlatform(pBlendState, blendFactor, uiSampleMask);

  m_State.m_hBlendState = hBlendState;
  m_State.m_BlendFactor = blendFactor;
  m_State.m_uiSampleMask = uiSampleMask;
}

void ezGALCommandEncoder::SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue /*= 0xFFu*/)
{
  AssertRenderingThread();

  if (m_State.m_hDepthStencilState == hDepthStencilState && m_State.m_uiStencilRefValue == uiStencilRefValue)
  {
    return;
  }

  const ezGALDepthStencilState* pDepthStencilState = GetDevice().GetDepthStencilState(hDepthStencilState);

  m_CommonImpl.SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_State.m_hDepthStencilState = hDepthStencilState;
  m_State.m_uiStencilRefValue = uiStencilRefValue;
}

void ezGALCommandEncoder::SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_State.m_hRasterizerState == hRasterizerState)
  {
    return;
  }

  const ezGALRasterizerState* pRasterizerState = GetDevice().GetRasterizerState(hRasterizerState);

  m_CommonImpl.SetRasterizerStatePlatform(pRasterizerState);

  m_State.m_hRasterizerState = hRasterizerState;
}

void ezGALCommandEncoder::SetViewport(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_State.m_ViewPortRect == rect && m_State.m_fViewPortMinDepth == fMinDepth && m_State.m_fViewPortMaxDepth == fMaxDepth)
  {
    return;
  }

  m_CommonImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_State.m_ViewPortRect = rect;
  m_State.m_fViewPortMinDepth = fMinDepth;
  m_State.m_fViewPortMaxDepth = fMaxDepth;
}

void ezGALCommandEncoder::SetScissorRect(const ezRectU32& rect)
{
  AssertRenderingThread();

  if (m_State.m_ScissorRect == rect)
  {
    return;
  }

  m_CommonImpl.SetScissorRectPlatform(rect);

  m_State.m_ScissorRect = rect;
}

void ezGALCommandEncoder::BeginCompute(const char* szName)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  m_CommonImpl.BeginComputePlatform();

  m_bMarker = !ezStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    PushMarker(szName);
  }
}

void ezGALCommandEncoder::EndCompute()
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "BeginCompute has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    PopMarker();
    m_bMarker = false;
  }

  m_CommonImpl.EndComputePlatform();
}

void ezGALCommandEncoder::BeginRendering(const ezGALRenderingSetup& renderingSetup, const char* szName)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  m_CommonImpl.BeginRenderingPlatform(renderingSetup);

  m_bMarker = !ezStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    PushMarker(szName);
  }
}

void ezGALCommandEncoder::EndRendering()
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Render, "BeginRendering has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    PopMarker();
    m_bMarker = false;
  }

  EZ_ASSERT_DEBUG(m_PendingOcclusionQuery.IsInvalidated(), "An occlusion query was started and not stopped within this render scope.");

  m_CommonImpl.EndRenderingPlatform();
}
