
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

#include <d3d11_1.h>

template <typename Base>
ezGALCommandEncoderDX11<Base>::ezGALCommandEncoderDX11(ezGALDevice& device)
  : Base(device)
{
  m_pDXContext = static_cast<ezGALDeviceDX11&>(device).GetDXImmediateContext();

  if (FAILED(m_pDXContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_pDXAnnotation)))
  {
    ezLog::Warning("Failed to get annotation interface. GALContext marker will not work");
  }
}

template <typename Base>
ezGALCommandEncoderDX11<Base>::~ezGALCommandEncoderDX11()
{
  EZ_GAL_DX11_RELEASE(m_pDXAnnotation);
}

// State setting functions

template <typename Base>
void ezGALCommandEncoderDX11<Base>::SetShaderPlatform(const ezGALShader* pShader)
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

template <typename Base>
void ezGALCommandEncoderDX11<Base>::SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const ezGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<const ezGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<const ezGALResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<const ezGALUnorderedAccessViewDX11*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);
}

// Fence & Query functions

template <typename Base>
void ezGALCommandEncoderDX11<Base>::InsertFencePlatform(const ezGALFence* pFence)
{
  m_pDXContext->End(static_cast<const ezGALFenceDX11*>(pFence)->GetDXFence());
}

template <typename Base>
bool ezGALCommandEncoderDX11<Base>::IsFenceReachedPlatform(const ezGALFence* pFence)
{
  BOOL data = FALSE;
  if (m_pDXContext->GetData(static_cast<const ezGALFenceDX11*>(pFence)->GetDXFence(), &data, sizeof(data), 0) == S_OK)
  {
    EZ_ASSERT_DEV(data == TRUE, "Implementation error");
    return true;
  }

  return false;
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::WaitForFencePlatform(const ezGALFence* pFence)
{
  BOOL data = FALSE;
  while (m_pDXContext->GetData(static_cast<const ezGALFenceDX11*>(pFence)->GetDXFence(), &data, sizeof(data), 0) != S_OK)
  {
    ezThreadUtils::YieldTimeSlice();
  }

  EZ_ASSERT_DEV(data == TRUE, "Implementation error");
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::BeginQueryPlatform(const ezGALQuery* pQuery)
{
  m_pDXContext->Begin(static_cast<const ezGALQueryDX11*>(pQuery)->GetDXQuery());
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::EndQueryPlatform(const ezGALQuery* pQuery)
{
  m_pDXContext->End(static_cast<const ezGALQueryDX11*>(pQuery)->GetDXQuery());
}

template <typename Base>
ezResult ezGALCommandEncoderDX11<Base>::GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult)
{
  return m_pDXContext->GetData(
           static_cast<const ezGALQueryDX11*>(pQuery)->GetDXQuery(), &uiQueryResult, sizeof(ezUInt64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_FALSE
           ? EZ_FAILURE
           : EZ_SUCCESS;
}

// Timestamp functions

template <typename Base>
void ezGALCommandEncoderDX11<Base>::InsertTimestampPlatform(ezGALTimestampHandle hTimestamp)
{
  ID3D11Query* pDXQuery = static_cast<ezGALDeviceDX11&>(this->GetDevice()).GetTimestamp(hTimestamp);

  m_pDXContext->End(pDXQuery);
}

// Resource update functions

template <typename Base>
void ezGALCommandEncoderDX11<Base>::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues)
{
  const ezGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewFloat(pUnorderedAccessViewDX11->GetDXResourceView(), &clearValues.x);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues)
{
  const ezGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewUint(pUnorderedAccessViewDX11->GetDXResourceView(), &clearValues.x);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const ezGALBufferDX11*>(pSource)->GetDXBuffer();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::CopyBufferRegionPlatform(  const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const ezGALBufferDX11*>(pSource)->GetDXBuffer();

  D3D11_BOX srcBox = {uiSourceOffset, 0, 0, uiSourceOffset + uiByteCount, 1, 1};
  m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXSource, 0, &srcBox);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::UpdateBufferPlatform(  const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode)
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
      if (ID3D11Resource* pDXTempBuffer = static_cast<ezGALDeviceDX11&>(this->GetDevice()).FindTempBuffer(pSourceData.GetCount()))
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

template <typename Base>
void ezGALCommandEncoderDX11<Base>::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureDX11*>(pSource)->GetDXTexture();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
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

template <typename Base>
void ezGALCommandEncoderDX11<Base>::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();

  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ID3D11Resource* pDXTempTexture = static_cast<ezGALDeviceDX11&>(this->GetDevice()).FindTempTexture(uiWidth, uiHeight, uiDepth, format))
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

    ezUInt32 dstSubResource = D3D11CalcSubresource(
      DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(
      pDXDestination, dstSubResource, DestinationBox.m_vMin.x, DestinationBox.m_vMin.y, DestinationBox.m_vMin.z, pDXTempTexture, 0, &srcBox);
  }
  else
  {
    EZ_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureDX11*>(pSource)->GetDXTexture();

  ezUInt32 dstSubResource = D3D11CalcSubresource(
    DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  ezUInt32 srcSubResource =
    D3D11CalcSubresource(SourceSubResource.m_uiMipLevel, SourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat =
    static_cast<ezGALDeviceDX11&>(this->GetDevice()).GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::ReadbackTexturePlatform(const ezGALTexture* pTexture)
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

template <typename Base>
void ezGALCommandEncoderDX11<Base>::CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
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

template <typename Base>
void ezGALCommandEncoderDX11<Base>::GenerateMipMapsPlatform(const ezGALResourceView* pResourceView)
{
  const ezGALResourceViewDX11* pDXResourceView = static_cast<const ezGALResourceViewDX11*>(pResourceView);

  m_pDXContext->GenerateMips(pDXResourceView->GetDXResourceView());
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

template <typename Base>
void ezGALCommandEncoderDX11<Base>::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

template <typename Base>
void ezGALCommandEncoderDX11<Base>::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}

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
template <typename Base>
void ezGALCommandEncoderDX11<Base>::FlushDeferredStateChanges()
{
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
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_pBoundUnoderedAccessViews.GetData() + uiStartSlot,
      nullptr); // Todo: Count reset.

    m_pBoundUnoderedAccessViewsRange.Reset();
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources(
        (ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

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
