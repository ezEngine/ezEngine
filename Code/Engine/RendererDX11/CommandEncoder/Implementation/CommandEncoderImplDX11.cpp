#include <RendererDX11/RendererDX11PCH.h>

#include <Foundation/Containers/IterateBits.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Pools/FencePoolDX11.h>
#include <RendererDX11/Pools/QueryPoolDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ReadbackBufferDX11.h>
#include <RendererDX11/Resources/ReadbackTextureDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <d3d11_1.h>

ezGALCommandEncoderImplDX11::ezGALCommandEncoderImplDX11(ezGALDeviceDX11& ref_deviceDX11)
  : m_GALDeviceDX11(ref_deviceDX11)
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


void ezGALCommandEncoderImplDX11::EndFrame()
{
  m_AlreadyUpdatedTransientBuffers.Clear();
}

// State setting functions

void ezGALCommandEncoderImplDX11::SetShaderPlatform(const ezGALShader* pShader)
{
  m_uiTessellationPatchControlPoints = 0;
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
    if (pShader)
    {
      m_uiTessellationPatchControlPoints = pShader->GetDescription().m_ByteCodes[ezGALShaderStage::HullShader]->m_uiTessellationPatchControlPoints;
    }
    else
    {
      m_uiTessellationPatchControlPoints = 0;
    }
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

void ezGALCommandEncoderImplDX11::SetConstantBufferPlatform(const ezShaderResourceBinding& binding, const ezGALBuffer* pBuffer)
{
  EZ_ASSERT_RELEASE(binding.m_iSlot < EZ_GAL_MAX_CONSTANT_BUFFER_COUNT, "Constant buffer slot index too big!");

  ID3D11Buffer* pBufferDX11 = pBuffer != nullptr ? static_cast<const ezGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;
  if (m_pBoundConstantBuffers[binding.m_iSlot] == pBufferDX11)
    return;

  m_pBoundConstantBuffers[binding.m_iSlot] = pBufferDX11;
  // The GAL doesn't care about stages for constant buffer, but we need to handle this internally.
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(binding.m_iSlot);
}

void ezGALCommandEncoderImplDX11::SetSamplerStatePlatform(const ezShaderResourceBinding& binding, const ezGALSamplerState* pSamplerState)
{
  EZ_ASSERT_RELEASE(binding.m_iSlot < EZ_GAL_MAX_SAMPLER_COUNT, "Sampler state slot index too big!");

  ID3D11SamplerState* pSamplerStateDX11 = pSamplerState != nullptr ? static_cast<const ezGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;

  for (ezGALShaderStage::Enum stage : ezIterateBitIndices<ezUInt16, ezGALShaderStage::Enum>(binding.m_Stages.GetValue()))
  {
    if (m_pBoundSamplerStates[stage][binding.m_iSlot] != pSamplerStateDX11)
    {
      m_pBoundSamplerStates[stage][binding.m_iSlot] = pSamplerStateDX11;
      m_BoundSamplerStatesRange[stage].SetToIncludeValue(binding.m_iSlot);
    }
  }
}

void ezGALCommandEncoderImplDX11::SetResourceViewPlatform(const ezShaderResourceBinding& binding, const ezGALTextureResourceView* pResourceView)
{
  if (pResourceView != nullptr && UnsetUnorderedAccessViews(pResourceView->GetResource()->GetParentResource()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11ShaderResourceView* pResourceViewDX11 = pResourceView != nullptr ? static_cast<const ezGALTextureResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;

  SetResourceView(binding, pResourceView != nullptr ? pResourceView->GetResource()->GetParentResource() : nullptr, pResourceViewDX11);
}

void ezGALCommandEncoderImplDX11::SetResourceViewPlatform(const ezShaderResourceBinding& binding, const ezGALBufferResourceView* pResourceView)
{
  if (pResourceView != nullptr && UnsetUnorderedAccessViews(pResourceView->GetResource()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11ShaderResourceView* pResourceViewDX11 = pResourceView != nullptr ? static_cast<const ezGALBufferResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;

  SetResourceView(binding, pResourceView != nullptr ? pResourceView->GetResource() : nullptr, pResourceViewDX11);
}

void ezGALCommandEncoderImplDX11::SetResourceView(const ezShaderResourceBinding& binding, const ezGALResourceBase* pResource, ID3D11ShaderResourceView* pResourceViewDX11)
{
  for (ezGALShaderStage::Enum stage : ezIterateBitIndices<ezUInt16, ezGALShaderStage::Enum>(binding.m_Stages.GetValue()))
  {
    auto& boundShaderResourceViews = m_pBoundShaderResourceViews[stage];
    boundShaderResourceViews.EnsureCount(binding.m_iSlot + 1);
    auto& resourcesForResourceViews = m_ResourcesForResourceViews[stage];
    resourcesForResourceViews.EnsureCount(binding.m_iSlot + 1);
    if (boundShaderResourceViews[binding.m_iSlot] != pResourceViewDX11)
    {
      boundShaderResourceViews[binding.m_iSlot] = pResourceViewDX11;
      resourcesForResourceViews[binding.m_iSlot] = pResource;
      m_BoundShaderResourceViewsRange[stage].SetToIncludeValue(binding.m_iSlot);
    }
  }
}

void ezGALCommandEncoderImplDX11::SetUnorderedAccessViewPlatform(const ezShaderResourceBinding& binding, const ezGALTextureUnorderedAccessView* pUnorderedAccessView)
{
  if (pUnorderedAccessView != nullptr && UnsetResourceViews(pUnorderedAccessView->GetResource()->GetParentResource()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11UnorderedAccessView* pUnorderedAccessViewDX11 = pUnorderedAccessView != nullptr ? static_cast<const ezGALTextureUnorderedAccessViewDX11*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;
  SetUnorderedAccessView(binding, pUnorderedAccessViewDX11, pUnorderedAccessView != nullptr ? pUnorderedAccessView->GetResource()->GetParentResource() : nullptr);
}

void ezGALCommandEncoderImplDX11::SetUnorderedAccessViewPlatform(const ezShaderResourceBinding& binding, const ezGALBufferUnorderedAccessView* pUnorderedAccessView)
{
  if (pUnorderedAccessView != nullptr && UnsetResourceViews(pUnorderedAccessView->GetResource()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11UnorderedAccessView* pUnorderedAccessViewDX11 = pUnorderedAccessView != nullptr ? static_cast<const ezGALBufferUnorderedAccessViewDX11*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;
  SetUnorderedAccessView(binding, pUnorderedAccessViewDX11, pUnorderedAccessView != nullptr ? pUnorderedAccessView->GetResource() : nullptr);
}

void ezGALCommandEncoderImplDX11::SetUnorderedAccessView(const ezShaderResourceBinding& binding, ID3D11UnorderedAccessView* pUnorderedAccessViewDX11, const ezGALResourceBase* pResource)
{
  m_BoundUnorderedAccessViews.EnsureCount(binding.m_iSlot + 1);
  m_ResourcesForUnorderedAccessViews.EnsureCount(binding.m_iSlot + 1);
  if (m_BoundUnorderedAccessViews[binding.m_iSlot] != pUnorderedAccessViewDX11)
  {
    m_BoundUnorderedAccessViews[binding.m_iSlot] = pUnorderedAccessViewDX11;
    m_ResourcesForUnorderedAccessViews[binding.m_iSlot] = pResource;
    m_BoundUnorderedAccessViewsRange.SetToIncludeValue(binding.m_iSlot);
  }
}

void ezGALCommandEncoderImplDX11::SetPushConstantsPlatform(ezArrayPtr<const ezUInt8> data)
{
  EZ_IGNORE_UNUSED(data);
  EZ_REPORT_FAILURE("DX11 does not support push constants, this function should not have been called.");
}

// Query functions

ezGALTimestampHandle ezGALCommandEncoderImplDX11::InsertTimestampPlatform()
{
  return m_GALDeviceDX11.GetQueryPool().InsertTimestamp();
}

ezGALOcclusionHandle ezGALCommandEncoderImplDX11::BeginOcclusionQueryPlatform(ezEnum<ezGALQueryType> type)
{
  return m_GALDeviceDX11.GetQueryPool().BeginOcclusionQuery(type);
}

void ezGALCommandEncoderImplDX11::EndOcclusionQueryPlatform(ezGALOcclusionHandle hOcclusion)
{
  m_GALDeviceDX11.GetQueryPool().EndOcclusionQuery(hOcclusion);
}

ezGALFenceHandle ezGALCommandEncoderImplDX11::InsertFencePlatform()
{
  return m_GALDeviceDX11.GetFenceQueue().GetCurrentFenceHandle();
}

// Resource update functions

void ezGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const ezGALTextureUnorderedAccessView* pUnorderedAccessView, ezVec4 vClearValues)
{
  const ezGALTextureUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALTextureUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewFloat(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
}

void ezGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const ezGALBufferUnorderedAccessView* pUnorderedAccessView, ezVec4 vClearValues)
{
  const ezGALBufferUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALBufferUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewFloat(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
}

void ezGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const ezGALTextureUnorderedAccessView* pUnorderedAccessView, ezVec4U32 vClearValues)
{
  const ezGALTextureUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALTextureUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewUint(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
}

void ezGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const ezGALBufferUnorderedAccessView* pUnorderedAccessView, ezVec4U32 vClearValues)
{
  const ezGALBufferUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const ezGALBufferUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewUint(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
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

void ezGALCommandEncoderImplDX11::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode)
{
  EZ_CHECK_ALIGNMENT(sourceData.GetPtr(), 16);

  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferDX11*>(pDestination)->GetDXBuffer();

  // On DX11 we can treat non-transient and transient constant buffers equally.
  if (updateMode == ezGALUpdateMode::TransientConstantBuffer || pDestination->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::ConstantBuffer))
  {
    EZ_ASSERT_DEV(uiDestOffset == 0 && sourceData.GetCount() == pDestination->GetSize(),
      "Constant buffers can't be updated partially (and we don't check for DX11.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult)))
    {
      memcpy(MapResult.pData, sourceData.GetPtr(), sourceData.GetCount());

      m_pDXContext->Unmap(pDXDestination, 0);
    }
  }
  else
  {
    const bool bTransient = pDestination->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::Transient);
    if (updateMode == ezGALUpdateMode::CopyToTempStorage || !bTransient)
    {
      if (ID3D11Resource* pDXTempBuffer = m_GALDeviceDX11.FindTempBuffer(sourceData.GetCount()))
      {
        D3D11_MAPPED_SUBRESOURCE MapResult;
        HRESULT hRes = m_pDXContext->Map(pDXTempBuffer, 0, D3D11_MAP_WRITE, 0, &MapResult);
        EZ_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");
        EZ_IGNORE_UNUSED(hRes);

        memcpy(MapResult.pData, sourceData.GetPtr(), sourceData.GetCount());

        m_pDXContext->Unmap(pDXTempBuffer, 0);

        D3D11_BOX srcBox = {0, 0, 0, sourceData.GetCount(), 1, 1};
        m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXTempBuffer, 0, &srcBox);
      }
      else
      {
        EZ_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      D3D11_MAP mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
      if (!m_AlreadyUpdatedTransientBuffers.Contains(pDestination))
      {
        // If this is the first time we update a transient buffer this frame, we can use DISCARD which will allow us to safely use NO_OVERWRITE on this buffer for this frame afterwards.
        // This is guaranteed by the constraint that the buffer must not be updated twice on the same memory location within one frame.
        m_AlreadyUpdatedTransientBuffers.Insert(pDestination);
        mapType = D3D11_MAP_WRITE_DISCARD;
      }

      D3D11_MAPPED_SUBRESOURCE MapResult;
      if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, mapType, 0, &MapResult)))
      {
        memcpy(ezMemoryUtils::AddByteOffset(MapResult.pData, uiDestOffset), sourceData.GetPtr(), sourceData.GetCount());

        m_pDXContext->Unmap(pDXDestination, 0);
      }
      else
      {
        ezLog::Error("Could not map buffer to update content.");
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

void ezGALCommandEncoderImplDX11::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
  const ezVec3U32& vDestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureDX11*>(pSource)->GetDXTexture();

  ezUInt32 dstSubResource = D3D11CalcSubresource(
    destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  ezUInt32 srcSubResource =
    D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  D3D11_BOX srcBox = {box.m_vMin.x, box.m_vMin.y, box.m_vMin.z, box.m_vMax.x, box.m_vMax.y, box.m_vMax.z};
  m_pDXContext->CopySubresourceRegion(
    pDXDestination, dstSubResource, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z, pDXSource, srcSubResource, &srcBox);
}

void ezGALCommandEncoderImplDX11::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
  const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();

  ezUInt32 uiWidth = ezMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ID3D11Resource* pDXTempTexture = m_GALDeviceDX11.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT hRes = m_pDXContext->Map(pDXTempTexture, 0, D3D11_MAP_WRITE, 0, &MapResult);
    EZ_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");
    EZ_IGNORE_UNUSED(hRes);

    ezUInt32 uiRowPitch = uiWidth * ezGALResourceFormat::GetBitsPerElement(format) / 8;
    ezUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    EZ_ASSERT_DEV(sourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, sourceData.m_uiRowPitch);
    EZ_ASSERT_DEV(sourceData.m_uiSlicePitch == 0 || sourceData.m_uiSlicePitch == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}",
      uiSlicePitch, sourceData.m_uiSlicePitch);

    if (MapResult.RowPitch == uiRowPitch && MapResult.DepthPitch == uiSlicePitch)
    {
      EZ_ASSERT_DEBUG(sourceData.m_pData.GetCount() >= uiSlicePitch * uiDepth, "Not enough data provided to update texture");
      memcpy(MapResult.pData, sourceData.m_pData.GetPtr(), uiSlicePitch * uiDepth);
    }
    else
    {
      // Copy row by row
      for (ezUInt32 z = 0; z < uiDepth; ++z)
      {
        const void* pSource = ezMemoryUtils::AddByteOffset(sourceData.m_pData.GetPtr(), z * uiSlicePitch);
        void* pDest = ezMemoryUtils::AddByteOffset(MapResult.pData, z * MapResult.DepthPitch);

        for (ezUInt32 y = 0; y < uiHeight; ++y)
        {
          memcpy(pDest, pSource, uiRowPitch);

          pSource = ezMemoryUtils::AddByteOffset(pSource, uiRowPitch);
          pDest = ezMemoryUtils::AddByteOffset(pDest, MapResult.RowPitch);
        }
      }
    }

    m_pDXContext->Unmap(pDXTempTexture, 0);

    ezUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, destinationBox.m_vMin.x, destinationBox.m_vMin.y, destinationBox.m_vMin.z, pDXTempTexture, 0, &srcBox);
  }
  else
  {
    EZ_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void ezGALCommandEncoderImplDX11::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureDX11*>(pSource)->GetDXTexture();

  ezUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  ezUInt32 srcSubResource = D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat = m_GALDeviceDX11.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

void ezGALCommandEncoderImplDX11::ReadbackTexturePlatform(const ezGALReadbackTexture* pDestination, const ezGALTexture* pSource)
{
  const ezGALReadbackTextureDX11* pDXDestination = static_cast<const ezGALReadbackTextureDX11*>(pDestination);
  const ezGALTextureDX11* pDXTexture = static_cast<const ezGALTextureDX11*>(pSource);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;
  EZ_IGNORE_UNUSED(bMSAASourceTexture);
  EZ_ASSERT_DEV(!bMSAASourceTexture, "MSAA readback is not supported");
  m_pDXContext->CopyResource(pDXDestination->GetDXTexture(), pDXTexture->GetDXTexture());
}


void ezGALCommandEncoderImplDX11::ReadbackBufferPlatform(const ezGALReadbackBuffer* pDestination, const ezGALBuffer* pSource)
{
  const ezGALReadbackBufferDX11* pDXDestination = static_cast<const ezGALReadbackBufferDX11*>(pDestination);
  const ezGALBufferDX11* pDXBuffer = static_cast<const ezGALBufferDX11*>(pSource);
  m_pDXContext->CopyResource(pDXDestination->GetDXBuffer(), pDXBuffer->GetDXBuffer());
}

ezUInt32 GetMipSize(ezUInt32 uiSize, ezUInt32 uiMipLevel)
{
  for (ezUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return ezMath::Max(1u, uiSize);
}

void ezGALCommandEncoderImplDX11::GenerateMipMapsPlatform(const ezGALTextureResourceView* pResourceView)
{
  const ezGALTextureResourceViewDX11* pDXResourceView = static_cast<const ezGALTextureResourceViewDX11*>(pResourceView);

  m_pDXContext->GenerateMips(pDXResourceView->GetDXResourceView());
}

void ezGALCommandEncoderImplDX11::FlushPlatform()
{
  FlushDeferredStateChanges().IgnoreResult();
  m_GALDeviceDX11.GetFenceQueue().SubmitCurrentFence();
  m_pDXContext->Flush();
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

void ezGALCommandEncoderImplDX11::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    const ezGALRenderTargetView* pRenderTargetViews[EZ_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    const ezGALRenderTargetView* pDepthStencilView = nullptr;

    const ezUInt32 uiRenderTargetCount = m_RenderTargetSetup.GetRenderTargetCount();

    bool bFlushNeeded = false;

    for (ezUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      const ezGALRenderTargetView* pRenderTargetView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));
      if (pRenderTargetView != nullptr)
      {
        const ezGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

        bFlushNeeded |= UnsetResourceViews(pTexture);
        bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
      }

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    pDepthStencilView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget());
    if (pDepthStencilView != nullptr)
    {
      const ezGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();

      bFlushNeeded |= UnsetResourceViews(pTexture);
      bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
    }

    if (bFlushNeeded)
    {
      FlushDeferredStateChanges().IgnoreResult();
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

void ezGALCommandEncoderImplDX11::EndRenderingPlatform()
{
}

void ezGALCommandEncoderImplDX11::BeginComputePlatform()
{
  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = ezGALRenderTargetSetup();
  m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
}
void ezGALCommandEncoderImplDX11::EndComputePlatform()
{
}

// Draw functions

void ezGALCommandEncoderImplDX11::ClearPlatform(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  for (ezUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], clearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    ezUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, fDepthClear, uiStencilClear);
  }
}

ezResult ezGALCommandEncoderImplDX11::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplDX11::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

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
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplDX11::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplDX11::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplDX11::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplDX11::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DrawInstancedIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
  return EZ_SUCCESS;
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

static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[] = {
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
};

static_assert(EZ_ARRAY_SIZE(GALTopologyToDX11) == ezGALPrimitiveTopology::ENUM_COUNT);

void ezGALCommandEncoderImplDX11::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum topology)
{
  m_Topology = topology;
}

void ezGALCommandEncoderImplDX11::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& blendFactor, ezUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {blendFactor.r, blendFactor.g, blendFactor.b, blendFactor.a};

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

//////////////////////////////////////////////////////////////////////////

ezResult ezGALCommandEncoderImplDX11::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplDX11::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pDXContext->DispatchIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
  return EZ_SUCCESS;
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
ezResult ezGALCommandEncoderImplDX11::FlushDeferredStateChanges()
{
  if (m_uiTessellationPatchControlPoints == 0)
  {
    m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[m_Topology.GetValue()]);
  }
  else
  {
    m_pDXContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (m_uiTessellationPatchControlPoints - 1)));
  }

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
  if (m_BoundUnorderedAccessViewsRange.IsValid())
  {
    const ezUInt32 uiStartSlot = m_BoundUnorderedAccessViewsRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_BoundUnorderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_BoundUnorderedAccessViews.GetData() + uiStartSlot, nullptr); // Todo: Count reset.

    m_BoundUnorderedAccessViewsRange.Reset();
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
  return EZ_SUCCESS;
}

bool ezGALCommandEncoderImplDX11::UnsetUnorderedAccessViews(const ezGALResourceBase* pResource)
{
  EZ_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (ezUInt32 uiSlot = 0; uiSlot < m_ResourcesForUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_ResourcesForUnorderedAccessViews[uiSlot] == pResource)
    {
      m_ResourcesForUnorderedAccessViews[uiSlot] = nullptr;
      m_BoundUnorderedAccessViews[uiSlot] = nullptr;
      m_BoundUnorderedAccessViewsRange.SetToIncludeValue(uiSlot);
      bResult = true;
    }
  }

  return bResult;
}
bool ezGALCommandEncoderImplDX11::UnsetResourceViews(const ezGALResourceBase* pResource)
{
  EZ_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (ezUInt32 uiSlot = 0; uiSlot < m_ResourcesForResourceViews[stage].GetCount(); ++uiSlot)
    {
      if (m_ResourcesForResourceViews[stage][uiSlot] == pResource)
      {
        m_ResourcesForResourceViews[stage][uiSlot] = nullptr;
        m_pBoundShaderResourceViews[stage][uiSlot] = nullptr;
        m_BoundShaderResourceViewsRange[stage].SetToIncludeValue(uiSlot);
        bResult = true;
      }
    }
  }

  return bResult;
}
