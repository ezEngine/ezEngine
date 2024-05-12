#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
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
  EZ_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferFlags::ConstantBuffer), "Wrong buffer type");

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

void ezGALCommandEncoder::BeginQuery(ezGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  EZ_ASSERT_DEV(!query->m_bStarted, "Can't stat ezGALQuery because it is already running.");

  m_CommonImpl.BeginQueryPlatform(query);
}

void ezGALCommandEncoder::EndQuery(ezGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  EZ_ASSERT_DEV(query->m_bStarted, "Can't end ezGALQuery, query hasn't started yet.");

  m_CommonImpl.EndQueryPlatform(query);
}

ezResult ezGALCommandEncoder::GetQueryResult(ezGALQueryHandle hQuery, ezUInt64& ref_uiQueryResult)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  EZ_ASSERT_DEV(!query->m_bStarted, "Can't retrieve data from ezGALQuery while query is still running.");

  return m_CommonImpl.GetQueryResultPlatform(query, ref_uiQueryResult);
}

ezGALTimestampHandle ezGALCommandEncoder::InsertTimestamp()
{
  ezGALTimestampHandle hTimestamp = m_Device.GetTimestamp();

  m_CommonImpl.InsertTimestampPlatform(hTimestamp);

  return hTimestamp;
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

void ezGALCommandEncoder::ClearStatisticsCounters()
{
}

ezGALCommandEncoder::ezGALCommandEncoder(ezGALDevice& device, ezGALCommandEncoderState& state, ezGALCommandEncoderCommonPlatformInterface& commonImpl)
  : m_Device(device)
  , m_State(state)
  , m_CommonImpl(commonImpl)
{
}

ezGALCommandEncoder::~ezGALCommandEncoder() = default;

void ezGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
}
