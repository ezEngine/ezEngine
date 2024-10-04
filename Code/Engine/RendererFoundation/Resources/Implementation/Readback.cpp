#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/Readback.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

ezEnum<ezGALAsyncResult> ezGALReadback::GetReadbackResult(ezTime timeout) const
{
  if (m_hFence == 0 || m_pDevice == nullptr)
    return ezGALAsyncResult::Expired;

  return m_pDevice->GetFenceResult(m_hFence, timeout);
}

ezGALReadbackBuffer::~ezGALReadbackBuffer()
{
  if (!m_hReadbackBuffer.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hReadbackBuffer);
  }
}

ezGALFenceHandle ezGALReadbackBuffer::ReadbackBuffer(ezGALCommandEncoder& encoder, ezGALBufferHandle hBuffer)
{
  EZ_ASSERT_DEV(!encoder.IsInRenderingScope(), "Readback is only supported outside rendering scope");
  m_pDevice = &encoder.GetDevice();
  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle passed in for readback");
  const ezGALBufferCreationDescription& desc = pBuffer->GetDescription();

  if (!m_hReadbackBuffer.IsInvalidated())
  {
    const ezGALBuffer* pReadbackBuffer = m_pDevice->GetBuffer(m_hReadbackBuffer);
    const ezGALBufferCreationDescription& readbackDesc = pReadbackBuffer->GetDescription();
    if (desc.m_uiTotalSize != readbackDesc.m_uiTotalSize)
    {
      m_pDevice->DestroyBuffer(m_hReadbackBuffer);
      m_hReadbackBuffer = {};
    }
  }

  if (m_hReadbackBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription readbackDesc;
    readbackDesc.m_uiTotalSize = desc.m_uiTotalSize;
    readbackDesc.m_ResourceAccess.m_MemoryUsage = ezGALMemoryUsage::Readback;
    readbackDesc.m_ResourceAccess.m_bImmutable = false;
    m_hReadbackBuffer = m_pDevice->CreateBuffer(readbackDesc);
  }
  encoder.CopyBuffer(m_hReadbackBuffer, hBuffer);
  m_hFence = encoder.InsertFence();
  return m_hFence;
}

ezResult ezGALReadbackBuffer::LockBuffer(ezArrayPtr<const ezUInt8>& out_Memory)
{
  if (m_hReadbackBuffer.IsInvalidated())
    return EZ_FAILURE;
  const ezGALBuffer* pReadbackBuffer = m_pDevice->GetBuffer(m_hReadbackBuffer);
  if (pReadbackBuffer == nullptr)
    return EZ_FAILURE;

  return m_pDevice->LockBufferPlatform(pReadbackBuffer, out_Memory);
}

ezResult ezGALReadbackBuffer::UnlockBuffer()
{
  if (m_hReadbackBuffer.IsInvalidated())
    return EZ_FAILURE;
  const ezGALBuffer* pReadbackBuffer = m_pDevice->GetBuffer(m_hReadbackBuffer);
  if (pReadbackBuffer == nullptr)
    return EZ_FAILURE;

  return m_pDevice->UnlockBufferPlatform(pReadbackBuffer);
}

ezGALReadbackTexture::~ezGALReadbackTexture()
{
  if (!m_hReadbackTexture.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hReadbackTexture);
  }
}

ezGALFenceHandle ezGALReadbackTexture::ReadbackTexture(ezGALCommandEncoder& encoder, ezGALTextureHandle hTexture)
{
  EZ_ASSERT_DEV(!encoder.IsInRenderingScope(), "Readback is only supported outside rendering scope");
  m_pDevice = &encoder.GetDevice();
  const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  EZ_ASSERT_DEV(pTexture != nullptr, "Invalid texture handle passed in for readback");
  const ezGALTextureCreationDescription& desc = pTexture->GetDescription();

  if (!m_hReadbackTexture.IsInvalidated())
  {
    const ezGALTexture* pReadbackTexture = m_pDevice->GetTexture(m_hReadbackTexture);
    const ezGALTextureCreationDescription& readbackDesc = pReadbackTexture->GetDescription();
    if (desc.m_uiWidth != readbackDesc.m_uiWidth || desc.m_uiHeight != readbackDesc.m_uiHeight || desc.m_uiDepth != readbackDesc.m_uiDepth || desc.m_uiMipLevelCount != readbackDesc.m_uiMipLevelCount || desc.m_uiArraySize != readbackDesc.m_uiArraySize || desc.m_Format != readbackDesc.m_Format || desc.m_Type != readbackDesc.m_Type || desc.m_Format != readbackDesc.m_Format || desc.m_SampleCount != readbackDesc.m_SampleCount)
    {
      m_pDevice->DestroyTexture(m_hReadbackTexture);
      m_hReadbackTexture = {};
    }
  }

  if (m_hReadbackTexture.IsInvalidated())
  {
    EZ_ASSERT_DEV(desc.m_SampleCount == ezGALMSAASampleCount::None, "Readback of Multi-sampled images is unsupported");

    ezGALTextureCreationDescription readbackDesc;
    readbackDesc.m_uiWidth = desc.m_uiWidth;
    readbackDesc.m_uiHeight = desc.m_uiHeight;
    readbackDesc.m_uiDepth = desc.m_uiDepth;
    readbackDesc.m_uiMipLevelCount = desc.m_uiMipLevelCount;
    readbackDesc.m_uiArraySize = desc.m_uiArraySize;
    readbackDesc.m_Format = desc.m_Format;
    readbackDesc.m_SampleCount = desc.m_SampleCount;
    readbackDesc.m_Type = desc.m_Type;
    readbackDesc.m_ResourceAccess.m_MemoryUsage = ezGALMemoryUsage::Readback;
    readbackDesc.m_ResourceAccess.m_bImmutable = false;
    m_hReadbackTexture = m_pDevice->CreateTexture(readbackDesc);
  }
  encoder.ReadbackTexture(m_hReadbackTexture, hTexture);
  m_hFence = encoder.InsertFence();
  return m_hFence;
}

ezResult ezGALReadbackTexture::LockTexture(const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory)
{
  out_Memory.Clear();
  if (m_hReadbackTexture.IsInvalidated())
    return EZ_FAILURE;
  const ezGALTexture* pReadbackTexture = m_pDevice->GetTexture(m_hReadbackTexture);
  if (pReadbackTexture == nullptr)
    return EZ_FAILURE;

  return m_pDevice->LockTexturePlatform(pReadbackTexture, subResources, out_Memory);
}

ezResult ezGALReadbackTexture::UnlockTexture(const ezArrayPtr<const ezGALTextureSubresource>& subResources)
{
  if (m_hReadbackTexture.IsInvalidated())
    return EZ_FAILURE;
  const ezGALTexture* pReadbackTexture = m_pDevice->GetTexture(m_hReadbackTexture);
  if (pReadbackTexture == nullptr)
    return EZ_FAILURE;

  return m_pDevice->UnlockTexturePlatform(pReadbackTexture, subResources);
}
