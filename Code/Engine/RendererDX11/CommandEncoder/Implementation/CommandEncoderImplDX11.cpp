#include <RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/FenceDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <d3d11_1.h>

ezGALCommandEncoderImplDX11::ezGALCommandEncoderImplDX11(ezGALDeviceDX11& deviceDX11)
  : m_GALDeviceDX11(deviceDX11)
{
  m_pDXContext = m_GALDeviceDX11.GetDXImmediateContext();

  if (FAILED(m_pDXContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_pDXAnnotation)))
  {
    ezLog::Warning("Failed to get annotation interface. GALContext marker will not work");
  }
}

ezGALCommandEncoderImplDX11::~ezGALCommandEncoderImplDX11()
{
  EZ_GAL_DX11_RELEASE(m_pDXAnnotation);
}

// State setting functions

void ezGALCommandEncoderImplDX11::SetShaderPlatform(const ezGALShader* pShader)
{
  ID3D11VertexShader* pVS = nullptr;
  ID3D11HullShader* pHS = nullptr;
  ID3D11DomainShader* pDS = nullptr;
  ID3D11GeometryShader* pGS = nullptr;
  ID3D11PixelShader* pPS = nullptr;
  ID3D11ComputeShader* pCS = nullptr;

  if (pShader != nullptr)
  {
    const ezGALShaderDX11* pDXShader = static_cast<const ezGALShaderDX11*>(pShader);

    pVS = pDXShader->GetDXVertexShader();
    pHS = pDXShader->GetDXHullShader();
    pDS = pDXShader->GetDXDomainShader();
    pGS = pDXShader->GetDXGeometryShader();
    pPS = pDXShader->GetDXPixelShader();
    pCS = pDXShader->GetDXComputeShader();
  }

  if (pVS != m_pBoundShaders[ezGALShaderStage::VertexShader])
  {
    m_pDXContext->VSSetShader(pVS, nullptr, 0);
    m_pBoundShaders[ezGALShaderStage::VertexShader] = pVS;
  }

  if (pHS != m_pBoundShaders[ezGALShaderStage::HullShader])
  {
    m_pDXContext->HSSetShader(pHS, nullptr, 0);
    m_pBoundShaders[ezGALShaderStage::HullShader] = pHS;
  }

  if (pDS != m_pBoundShaders[ezGALShaderStage::DomainShader])
  {
    m_pDXContext->DSSetShader(pDS, nullptr, 0);
    m_pBoundShaders[ezGALShaderStage::DomainShader] = pDS;
  }

  if (pGS != m_pBoundShaders[ezGALShaderStage::GeometryShader])
  {
    m_pDXContext->GSSetShader(pGS, nullptr, 0);
    m_pBoundShaders[ezGALShaderStage::GeometryShader] = pGS;
  }

  if (pPS != m_pBoundShaders[ezGALShaderStage::PixelShader])
  {
    m_pDXContext->PSSetShader(pPS, nullptr, 0);
    m_pBoundShaders[ezGALShaderStage::PixelShader] = pPS;
  }

  if (pCS != m_pBoundShaders[ezGALShaderStage::ComputeShader])
  {
    m_pDXContext->CSSetShader(pCS, nullptr, 0);
    m_pBoundShaders[ezGALShaderStage::ComputeShader] = pCS;
  }
}

void ezGALCommandEncoderImplDX11::SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const ezGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);
}

void ezGALCommandEncoderImplDX11::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<const ezGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);
}

void ezGALCommandEncoderImplDX11::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<const ezGALResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);
}

void ezGALCommandEncoderImplDX11::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<const ezGALUnorderedAccessViewDX11*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);
}

// Fence & Query functions

void ezGALCommandEncoderImplDX11::InsertFencePlatform(const ezGALFence* pFence)
{
  m_pDXContext->End(static_cast<const ezGALFenceDX11*>(pFence)->GetDXFence());
}

bool ezGALCommandEncoderImplDX11::IsFenceReachedPlatform(const ezGALFence* pFence)
{
  BOOL data = FALSE;
  if (m_pDXContext->GetData(static_cast<const ezGALFenceDX11*>(pFence)->GetDXFence(), &data, sizeof(data), 0) == S_OK)
  {
    EZ_ASSERT_DEV(data == TRUE, "Implementation error");
    return true;
  }

  return false;
}

void ezGALCommandEncoderImplDX11::WaitForFencePlatform(const ezGALFence* pFence)
{
  BOOL data = FALSE;
  while (m_pDXContext->GetData(static_cast<const ezGALFenceDX11*>(pFence)->GetDXFence(), &data, sizeof(data), 0) != S_OK)
  {
    ezThreadUtils::YieldTimeSlice();
  }

  EZ_ASSERT_DEV(data == TRUE, "Implementation error");
}

void ezGALCommandEncoderImplDX11::BeginQueryPlatform(const ezGALQuery* pQuery)
{
  m_pDXContext->Begin(static_cast<const ezGALQueryDX11*>(pQuery)->GetDXQuery());
}

void ezGALCommandEncoderImplDX11::EndQueryPlatform(const ezGALQuery* pQuery)
{
  m_pDXContext->End(static_cast<const ezGALQueryDX11*>(pQuery)->GetDXQuery());
}

ezResult ezGALCommandEncoderImplDX11::GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult)
{
  return m_pDXContext->GetData(
           static_cast<const ezGALQueryDX11*>(pQuery)->GetDXQuery(), &uiQueryResult, sizeof(ezUInt64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_FALSE
           ? EZ_FAILURE
           : EZ_SUCCESS;
}

// Timestamp functions

void ezGALCommandEncoderImplDX11::InsertTimestampPlatform(ezGALTimestampHandle hTimestamp)
{
  ID3D11Query* pDXQuery = m_GALDeviceDX11.GetTimestamp(hTimestamp);

  m_pDXContext->End(pDXQuery);
}

// Resource update functions

void ezGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues)
{
  const ezGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewFloat(pUnorderedAccessViewDX11->GetDXResourceView(), &clearValues.x);
}

void ezGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues)
{
  const ezGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewUint(pUnorderedAccessViewDX11->GetDXResourceView(), &clearValues.x);
}

void ezGALCommandEncoderImplDX11::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const ezGALBufferDX11*>(pSource)->GetDXBuffer();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void ezGALCommandEncoderImplDX11::CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const ezGALBufferDX11*>(pSource)->GetDXBuffer();

  D3D11_BOX srcBox = {uiSourceOffset, 0, 0, uiSourceOffset + uiByteCount, 1, 1};
  m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXSource, 0, &srcBox);
}

void ezGALCommandEncoderImplDX11::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode)
{
  EZ_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferDX11*>(pDestination)->GetDXBuffer();

  if (pDestination->GetDescription().m_BufferType == ezGALBufferType::ConstantBuffer)
  {
    EZ_ASSERT_DEV(uiDestOffset == 0 && pSourceData.GetCount() == pDestination->GetSize(),
      "Constant buffers can't be updated partially (and we don't check for DX11.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult)))
    {
      memcpy(MapResult.pData, pSourceData.GetPtr(), pSourceData.GetCount());

      m_pDXContext->Unmap(pDXDestination, 0);
    }
  }
  else
  {
    if (updateMode == ezGALUpdateMode::CopyToTempStorage)
    {
      if (ID3D11Resource* pDXTempBuffer = m_GALDeviceDX11.FindTempBuffer(pSourceData.GetCount()))
      {
        D3D11_MAPPED_SUBRESOURCE MapResult;
        HRESULT hRes = m_pDXContext->Map(pDXTempBuffer, 0, D3D11_MAP_WRITE, 0, &MapResult);
        EZ_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

        memcpy(MapResult.pData, pSourceData.GetPtr(), pSourceData.GetCount());

        m_pDXContext->Unmap(pDXTempBuffer, 0);

        D3D11_BOX srcBox = {0, 0, 0, pSourceData.GetCount(), 1, 1};
        m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXTempBuffer, 0, &srcBox);
      }
      else
      {
        EZ_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      D3D11_MAP mapType = (updateMode == ezGALUpdateMode::Discard) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

      D3D11_MAPPED_SUBRESOURCE MapResult;
      if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, mapType, 0, &MapResult)))
      {
        memcpy(ezMemoryUtils::AddByteOffset(MapResult.pData, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());

        m_pDXContext->Unmap(pDXDestination, 0);
      }
    }
  }
}

void ezGALCommandEncoderImplDX11::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureDX11*>(pSource)->GetDXTexture();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void ezGALCommandEncoderImplDX11::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezVec3U32& DestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureDX11*>(pSource)->GetDXTexture();

  ezUInt32 dstSubResource = D3D11CalcSubresource(
    DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  ezUInt32 srcSubResource =
    D3D11CalcSubresource(SourceSubResource.m_uiMipLevel, SourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  D3D11_BOX srcBox = {Box.m_vMin.x, Box.m_vMin.y, Box.m_vMin.z, Box.m_vMax.x, Box.m_vMax.y, Box.m_vMax.z};
  m_pDXContext->CopySubresourceRegion(
    pDXDestination, dstSubResource, DestinationPoint.x, DestinationPoint.y, DestinationPoint.z, pDXSource, srcSubResource, &srcBox);
}

void ezGALCommandEncoderImplDX11::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();

  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ID3D11Resource* pDXTempTexture = m_GALDeviceDX11.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT hRes = m_pDXContext->Map(pDXTempTexture, 0, D3D11_MAP_WRITE, 0, &MapResult);
    EZ_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

    ezUInt32 uiRowPitch = uiWidth * ezGALResourceFormat::GetBitsPerElement(format) / 8;
    ezUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    EZ_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
    EZ_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}",
      uiSlicePitch, pSourceData.m_uiSlicePitch);

    memcpy(MapResult.pData, pSourceData.m_pData, uiSlicePitch * uiDepth);

    m_pDXContext->Unmap(pDXTempTexture, 0);

    ezUInt32 dstSubResource = D3D11CalcSubresource(DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, DestinationBox.m_vMin.x, DestinationBox.m_vMin.y, DestinationBox.m_vMin.z, pDXTempTexture, 0, &srcBox);
  }
  else
  {
    EZ_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void ezGALCommandEncoderImplDX11::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureDX11*>(pSource)->GetDXTexture();

  ezUInt32 dstSubResource = D3D11CalcSubresource(DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  ezUInt32 srcSubResource = D3D11CalcSubresource(SourceSubResource.m_uiMipLevel, SourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat = m_GALDeviceDX11.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

void ezGALCommandEncoderImplDX11::ReadbackTexturePlatform(const ezGALTexture* pTexture)
{
  const ezGALTextureDX11* pDXTexture = static_cast<const ezGALTextureDX11*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;

  EZ_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  EZ_ASSERT_DEV(pDXTexture->GetDXTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    /// \todo Other mip levels etc?
    m_pDXContext->ResolveSubresource(pDXTexture->GetDXStagingTexture(), 0, pDXTexture->GetDXTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
  }
  else
  {
    m_pDXContext->CopyResource(pDXTexture->GetDXStagingTexture(), pDXTexture->GetDXTexture());
  }
}

void ezGALCommandEncoderImplDX11::CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  const ezGALTextureDX11* pDXTexture = static_cast<const ezGALTextureDX11*>(pTexture);

  EZ_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");

  D3D11_MAPPED_SUBRESOURCE Mapped;
  if (SUCCEEDED(m_pDXContext->Map(pDXTexture->GetDXStagingTexture(), 0, D3D11_MAP_READ, 0, &Mapped)))
  {
    if (Mapped.RowPitch == (*pData)[0].m_uiRowPitch)
    {
      const ezUInt32 uiMemorySize = ezGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) *
                                    pDXTexture->GetDescription().m_uiWidth * pDXTexture->GetDescription().m_uiHeight / 8;
      memcpy((*pData)[0].m_pData, Mapped.pData, uiMemorySize);
    }
    else
    {
      // Copy row by row
      for (ezUInt32 y = 0; y < pDXTexture->GetDescription().m_uiHeight; ++y)
      {
        const void* pSource = ezMemoryUtils::AddByteOffset(Mapped.pData, y * Mapped.RowPitch);
        void* pDest = ezMemoryUtils::AddByteOffset((*pData)[0].m_pData, y * (*pData)[0].m_uiRowPitch);

        memcpy(
          pDest, pSource, ezGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) * pDXTexture->GetDescription().m_uiWidth / 8);
      }
    }

    m_pDXContext->Unmap(pDXTexture->GetDXStagingTexture(), 0);
  }
}

void ezGALCommandEncoderImplDX11::GenerateMipMapsPlatform(const ezGALResourceView* pResourceView)
{
  const ezGALResourceViewDX11* pDXResourceView = static_cast<const ezGALResourceViewDX11*>(pResourceView);

  m_pDXContext->GenerateMips(pDXResourceView->GetDXResourceView());
}

void ezGALCommandEncoderImplDX11::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void ezGALCommandEncoderImplDX11::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

void ezGALCommandEncoderImplDX11::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

void ezGALCommandEncoderImplDX11::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplDX11::BeginRender(const ezGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    const ezGALRenderTargetView* pRenderTargetViews[EZ_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    const ezGALRenderTargetView* pDepthStencilView = nullptr;

    ezUInt32 uiRenderTargetCount = 0;

    bool bFlushNeeded = false;

    if (m_RenderTargetSetup.HasRenderTargets())
    {
      for (ezUInt8 uiIndex = 0; uiIndex <= m_RenderTargetSetup.GetMaxRenderTargetIndex(); ++uiIndex)
      {
        const ezGALRenderTargetView* pRenderTargetView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));
        if (pRenderTargetView != nullptr)
        {
          const ezGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

          bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
          bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
        }

        pRenderTargetViews[uiIndex] = pRenderTargetView;
      }

      uiRenderTargetCount = m_RenderTargetSetup.GetMaxRenderTargetIndex() + 1;
    }

    pDepthStencilView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget());
    if (pDepthStencilView != nullptr)
    {
      const ezGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();

      bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
      bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
    }

    if (bFlushNeeded)
    {
      FlushPlatform();
    }

    for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
    }
    m_pBoundDepthStencilTarget = nullptr;

    if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
    {
      for (ezUInt32 i = 0; i < uiRenderTargetCount; i++)
      {
        if (pRenderTargetViews[i] != nullptr)
        {
          m_pBoundRenderTargets[i] = static_cast<const ezGALRenderTargetViewDX11*>(pRenderTargetViews[i])->GetRenderTargetView();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<const ezGALRenderTargetViewDX11*>(pDepthStencilView)->GetDepthStencilView();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pDXContext->OMSetRenderTargets(ezMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
      m_uiBoundRenderTargetCount = 0;
    }
  }

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

// Draw functions

void ezGALCommandEncoderImplDX11::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  for (ezUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], ClearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    ezUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, fDepthClear, uiStencilClear);
  }
}

void ezGALCommandEncoderImplDX11::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
}

void ezGALCommandEncoderImplDX11::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);

  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    if (m_GALDeviceDX11.m_pDebug != nullptr)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_GALDeviceDX11.m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError = FALSE;
        static BOOL bBreakOnWarning = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }
  }
#else
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
#endif
}

void ezGALCommandEncoderImplDX11::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void ezGALCommandEncoderImplDX11::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void ezGALCommandEncoderImplDX11::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void ezGALCommandEncoderImplDX11::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstancedIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void ezGALCommandEncoderImplDX11::DrawAutoPlatform()
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawAuto();
}

void ezGALCommandEncoderImplDX11::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALCommandEncoderImplDX11::EndStreamOutPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALCommandEncoderImplDX11::SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer)
{
  if (pIndexBuffer != nullptr)
  {
    const ezGALBufferDX11* pDX11Buffer = static_cast<const ezGALBufferDX11*>(pIndexBuffer);
    m_pDXContext->IASetIndexBuffer(pDX11Buffer->GetDXBuffer(), pDX11Buffer->GetIndexFormat(), 0 /* \todo: Expose */);
  }
  else
  {
    m_pDXContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
  }
}

void ezGALCommandEncoderImplDX11::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const ezGALBufferDX11*>(pVertexBuffer)->GetDXBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void ezGALCommandEncoderImplDX11::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration)
{
  m_pDXContext->IASetInputLayout(
    pVertexDeclaration != nullptr ? static_cast<const ezGALVertexDeclarationDX11*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}

static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[ezGALPrimitiveTopology::ENUM_COUNT] = {
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
};

void ezGALCommandEncoderImplDX11::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[Topology]);
}

void ezGALCommandEncoderImplDX11::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pDXContext->OMSetBlendState(
    pBlendState != nullptr ? static_cast<const ezGALBlendStateDX11*>(pBlendState)->GetDXBlendState() : nullptr, BlendFactors, uiSampleMask);
}

void ezGALCommandEncoderImplDX11::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  m_pDXContext->OMSetDepthStencilState(
    pDepthStencilState != nullptr ? static_cast<const ezGALDepthStencilStateDX11*>(pDepthStencilState)->GetDXDepthStencilState() : nullptr,
    uiStencilRefValue);
}

void ezGALCommandEncoderImplDX11::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(pRasterizerState != nullptr ? static_cast<const ezGALRasterizerStateDX11*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void ezGALCommandEncoderImplDX11::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width = rect.width;
  Viewport.Height = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void ezGALCommandEncoderImplDX11::SetScissorRectPlatform(const ezRectU32& rect)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left = rect.x;
  ScissorRect.top = rect.y;
  ScissorRect.right = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

void ezGALCommandEncoderImplDX11::SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplDX11::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALCommandEncoderImplDX11::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DispatchIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

//////////////////////////////////////////////////////////////////////////

static void SetShaderResources(ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots,
  ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(
  ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots, ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(
  ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots, ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

// Some state changes are deferred so they can be updated faster
void ezGALCommandEncoderImplDX11::FlushDeferredStateChanges()
{
  if (m_BoundVertexBuffersRange.IsValid())
  {
    const ezUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot, m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_pBoundShaders[stage] != nullptr && m_BoundConstantBuffersRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundConstantBuffersRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundConstantBuffersRange[stage].GetCount();

      SetConstantBuffers((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[stage].Reset();
    }
  }

  // Do UAV bindings before SRV since UAV are outputs which need to be unbound before they are potentially rebound as SRV again.
  if (m_pBoundUnoderedAccessViewsRange.IsValid())
  {
    const ezUInt32 uiStartSlot = m_pBoundUnoderedAccessViewsRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_pBoundUnoderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_pBoundUnoderedAccessViews.GetData() + uiStartSlot, nullptr); // Todo: Count reset.

    m_pBoundUnoderedAccessViewsRange.Reset();
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[stage].Reset();
    }

    // Don't need to unset sampler stages for unbound shader stages.
    if (m_pBoundShaders[stage] == nullptr)
      continue;

    if (m_BoundSamplerStatesRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundSamplerStatesRange[stage].GetCount();

      SetSamplers((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

      m_BoundSamplerStatesRange[stage].Reset();
    }
  }
}
