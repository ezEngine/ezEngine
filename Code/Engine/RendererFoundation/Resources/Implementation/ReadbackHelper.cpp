#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ReadbackBuffer.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Resources/ReadbackTexture.h>
#include <RendererFoundation/Resources/Texture.h>

ezEnum<ezGALAsyncResult> ezGALReadbackHelper::GetReadbackResult(ezTime timeout) const
{
  if (m_hFence == 0 || m_pDevice == nullptr)
    return ezGALAsyncResult::Expired;

  return m_pDevice->GetFenceResult(m_hFence, timeout);
}

ezGALReadbackBufferHelper::~ezGALReadbackBufferHelper()
{
  Reset();
}

void ezGALReadbackBufferHelper::Reset()
{
  if (!m_hReadbackBuffer.IsInvalidated())
  {
    m_pDevice->DestroyReadbackBuffer(m_hReadbackBuffer);
    m_hReadbackBuffer.Invalidate();
  }
  m_hFence = 0;
  m_pDevice = nullptr;
}

ezGALFenceHandle ezGALReadbackBufferHelper::ReadbackBuffer(ezGALCommandEncoder& encoder, ezGALBufferHandle hBuffer)
{
  EZ_ASSERT_DEV(!encoder.IsInRenderingScope(), "Readback is only supported outside rendering scope");
  m_pDevice = &encoder.GetDevice();
  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle passed in for readback");
  const ezGALBufferCreationDescription& desc = pBuffer->GetDescription();

  if (!m_hReadbackBuffer.IsInvalidated())
  {
    const ezGALReadbackBuffer* pReadbackBuffer = m_pDevice->GetReadbackBuffer(m_hReadbackBuffer);
    const ezGALBufferCreationDescription& readbackDesc = pReadbackBuffer->GetDescription();
    if (desc.m_uiTotalSize != readbackDesc.m_uiTotalSize)
    {
      m_pDevice->DestroyReadbackBuffer(m_hReadbackBuffer);
      m_hReadbackBuffer = {};
    }
  }

  if (m_hReadbackBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription readbackDesc;
    readbackDesc.m_uiTotalSize = desc.m_uiTotalSize;
    readbackDesc.m_ResourceAccess.m_bImmutable = false;
    m_hReadbackBuffer = m_pDevice->CreateReadbackBuffer(readbackDesc);
  }

  encoder.ReadbackBuffer(m_hReadbackBuffer, hBuffer);
  m_hFence = encoder.InsertFence();
  return m_hFence;
}

ezReadbackBufferLock ezGALReadbackBufferHelper::LockBuffer(ezArrayPtr<const ezUInt8>& out_Memory)
{
  if (m_pDevice == nullptr || m_hFence == 0 || m_pDevice->GetFenceResult(m_hFence) != ezGALAsyncResult::Ready)
    return {};

  return m_pDevice->LockBuffer(m_hReadbackBuffer, out_Memory);
}

//////////////////////////////////////////////////////////////////////////

ezGALReadbackTextureHelper::~ezGALReadbackTextureHelper()
{
  Reset();
}

void ezGALReadbackTextureHelper::Reset()
{
  if (!m_hReadbackTexture.IsInvalidated())
  {
    m_pDevice->DestroyReadbackTexture(m_hReadbackTexture);
    m_hReadbackTexture.Invalidate();
  }
  m_hFence = 0;
  m_pDevice = nullptr;
}

ezGALFenceHandle ezGALReadbackTextureHelper::ReadbackTexture(ezGALCommandEncoder& encoder, ezGALTextureHandle hTexture)
{
  EZ_ASSERT_DEV(!encoder.IsInRenderingScope(), "Readback is only supported outside rendering scope");
  m_pDevice = &encoder.GetDevice();
  const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  EZ_ASSERT_DEV(pTexture != nullptr, "Invalid texture handle passed in for readback");
  ezGALTextureCreationDescription desc = pTexture->GetDescription();
  // Reset properties that have no influence on the readback texture.
  desc.m_pExisitingNativeObject = nullptr;
  desc.m_ResourceAccess.m_bImmutable = false;
  desc.m_bAllowShaderResourceView = false;
  desc.m_bAllowUAV = false;
  desc.m_bCreateRenderTarget = false;
  desc.m_bAllowDynamicMipGeneration = false;


  if (!m_hReadbackTexture.IsInvalidated())
  {
    const ezGALReadbackTexture* pReadbackTexture = m_pDevice->GetReadbackTexture(m_hReadbackTexture);
    const ezGALTextureCreationDescription& readbackDesc = pReadbackTexture->GetDescription();
    if (desc.CalculateHash() != readbackDesc.CalculateHash())
    {
      m_pDevice->DestroyReadbackTexture(m_hReadbackTexture);
      m_hReadbackTexture = {};
    }
  }

  if (m_hReadbackTexture.IsInvalidated())
  {
    EZ_ASSERT_DEV(desc.m_SampleCount == ezGALMSAASampleCount::None, "Readback of Multi-sampled images is unsupported");
    m_hReadbackTexture = m_pDevice->CreateReadbackTexture(desc);
  }
  encoder.ReadbackTexture(m_hReadbackTexture, hTexture);
  m_hFence = encoder.InsertFence();
  return m_hFence;
}

ezReadbackTextureLock ezGALReadbackTextureHelper::LockTexture(const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory)
{
  if (m_pDevice == nullptr || m_hFence == 0 || m_pDevice->GetFenceResult(m_hFence) != ezGALAsyncResult::Ready)
    return {};

  return m_pDevice->LockTexture(m_hReadbackTexture, subResources, out_Memory);
}
