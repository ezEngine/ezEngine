
#include <PCH.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
#include <Foundation/Utilities/Stats.h>
#endif


ezGPUResourcePool* ezGPUResourcePool::s_pDefaultInstance = nullptr;


ezGPUResourcePool::ezGPUResourcePool()
  : m_uiMemoryThresholdForGC(256 * 1024 * 1024)
  , m_uiCurrentlyAllocatedMemory(0)
  , m_uiNumAllocationsThresholdForGC(128)
  , m_uiNumAllocationsSinceLastGC(0)
{
  m_pDevice = ezGALDevice::GetDefaultDevice();
}

ezGPUResourcePool::~ezGPUResourcePool()
{
  if (!m_TexturesInUse.IsEmpty())
  {
    ezLog::SeriousWarning("Destructing a GPU resource pool of which textures are still in use!");
  }

  // Free remaining resources
  RunGC();
}


ezGALTextureHandle ezGPUResourcePool::GetRenderTarget(const ezGALTextureCreationDescription& TextureDesc)
{
  EZ_LOCK(m_Lock);

  if (!TextureDesc.m_bCreateRenderTarget)
  {
    ezLog::Error("Texture description for render target usage has not set bCreateRenderTarget!");
    return ezGALTextureHandle();
  }

  const ezUInt32 uiTextureDescHash = TextureDesc.CalculateHash();

  // Check if there is a fitting texture available
  auto it = m_AvailableTextures.Find(uiTextureDescHash);
  if (it.IsValid())
  {
    ezDynamicArray<ezGALTextureHandle>& textures = it.Value();
    if (!textures.IsEmpty())
    {
      ezGALTextureHandle hTexture = textures[0];
      textures.RemoveAtSwap(0);

      EZ_ASSERT_DEV(m_pDevice->GetTexture(hTexture) != nullptr, "Invalid texture in resource pool");

      m_TexturesInUse.Insert(hTexture);

      return hTexture;
    }
  }

  // Since we found no matching texture we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  ezGALTextureHandle hNewTexture = m_pDevice->CreateTexture(TextureDesc);

  if (hNewTexture.IsInvalidated())
  {
    ezLog::Error("GPU resource pool couldn't create new texture for given desc (size: {0} x {1}, format: {2})", TextureDesc.m_uiWidth, TextureDesc.m_uiHeight, TextureDesc.m_Format);
    return ezGALTextureHandle();
  }

  // Also track the new created texture
  m_TexturesInUse.Insert(hNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(TextureDesc);

  UpdateMemoryStats();

  return hNewTexture;
}

ezGALTextureHandle ezGPUResourcePool::GetRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum eFormat, ezGALMSAASampleCount::Enum sampleCount, ezUInt32 uiSliceColunt)
{
  ezGALTextureCreationDescription TextureDesc;
  TextureDesc.m_bCreateRenderTarget = true;
  TextureDesc.m_bAllowShaderResourceView = true;
  TextureDesc.m_Format = eFormat;
  TextureDesc.m_Type = ezGALTextureType::Texture2D;
  TextureDesc.m_uiWidth = uiWidth;
  TextureDesc.m_uiHeight = uiHeight;
  TextureDesc.m_SampleCount = sampleCount;
  TextureDesc.m_uiArraySize = uiSliceColunt;

  return GetRenderTarget(TextureDesc);
}

void ezGPUResourcePool::ReturnRenderTarget(ezGALTextureHandle hRenderTarget)
{
  EZ_LOCK(m_Lock);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_TexturesInUse.Contains(hRenderTarget))
  {
    ezLog::Error("Returning a texture to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_TexturesInUse.Remove(hRenderTarget);

  if (const ezGALTexture* pTexture = m_pDevice->GetTexture(hRenderTarget))
  {
    const ezUInt32 uiTextureDescHash = pTexture->GetDescription().CalculateHash();

    auto it = m_AvailableTextures.Find(uiTextureDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableTextures.Insert(uiTextureDescHash, ezDynamicArray<ezGALTextureHandle>());
    }

    it.Value().PushBack(hRenderTarget);
  }
}

ezGALBufferHandle ezGPUResourcePool::GetBuffer(const ezGALBufferCreationDescription& BufferDesc)
{
  EZ_LOCK(m_Lock);

  const ezUInt32 uiBufferDescHash = BufferDesc.CalculateHash();

  // Check if there is a fitting buffer available
  auto it = m_AvailableBuffers.Find(uiBufferDescHash);
  if (it.IsValid())
  {
    ezDynamicArray<ezGALBufferHandle>& buffers = it.Value();
    if (!buffers.IsEmpty())
    {
      ezGALBufferHandle hBuffer = buffers[0];
      buffers.RemoveAtSwap(0);

      EZ_ASSERT_DEV(m_pDevice->GetBuffer(hBuffer) != nullptr, "Invalid buffer in resource pool");

      m_BuffersInUse.Insert(hBuffer);

      return hBuffer;
    }
  }

  // Since we found no matching buffer we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  ezGALBufferHandle hNewBuffer = m_pDevice->CreateBuffer(BufferDesc);

  if (hNewBuffer.IsInvalidated())
  {
    ezLog::Error("GPU resource pool couldn't create new buffer for given desc (size: {0})", BufferDesc.m_uiTotalSize);
    return ezGALBufferHandle();
  }

  // Also track the new created buffer
  m_BuffersInUse.Insert(hNewBuffer);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(BufferDesc);

  UpdateMemoryStats();

  return hNewBuffer;
}

void ezGPUResourcePool::ReturnBuffer(ezGALBufferHandle hBuffer)
{
  EZ_LOCK(m_Lock);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_BuffersInUse.Contains(hBuffer))
  {
    ezLog::Error("Returning a buffer to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_BuffersInUse.Remove(hBuffer);

  if (const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer))
  {
    const ezUInt32 uiBufferDescHash = pBuffer->GetDescription().CalculateHash();

    auto it = m_AvailableBuffers.Find(uiBufferDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableBuffers.Insert(uiBufferDescHash, ezDynamicArray<ezGALBufferHandle>());
    }

    it.Value().PushBack(hBuffer);
  }
}

void ezGPUResourcePool::RunGC()
{
  EZ_LOCK(m_Lock);

  // Destroy all available textures
  {
    for (auto it = m_AvailableTextures.GetIterator(); it.IsValid(); ++it)
    {
      auto& textures = it.Value();
      for (auto hCurrentTexture : textures)
      {
        if (const ezGALTexture* pTexture = m_pDevice->GetTexture(hCurrentTexture))
        {
          m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTexture->GetDescription());
        }

        m_pDevice->DestroyTexture(hCurrentTexture);
      }
    }

    m_AvailableTextures.Clear();
  }

  // Destroy all available buffers
  {
    for (auto it = m_AvailableBuffers.GetIterator(); it.IsValid(); ++it)
    {
      auto& buffers = it.Value();
      for (auto hCurrentBuffer : buffers)
      {
        if (const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hCurrentBuffer))
        {
          m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuffer->GetDescription());
        }

        m_pDevice->DestroyBuffer(hCurrentBuffer);
      }
    }

    m_AvailableBuffers.Clear();
  }

  m_uiNumAllocationsSinceLastGC = 0;

  UpdateMemoryStats();
}



ezGPUResourcePool* ezGPUResourcePool::GetDefaultInstance()
{
  return s_pDefaultInstance;
}

void ezGPUResourcePool::SetDefaultInstance(ezGPUResourcePool* pDefaultInstance)
{
  EZ_DEFAULT_DELETE(s_pDefaultInstance);
  s_pDefaultInstance = pDefaultInstance;
}


void ezGPUResourcePool::CheckAndPotentiallyRunGC()
{
  if ((m_uiNumAllocationsSinceLastGC >= m_uiNumAllocationsThresholdForGC) || (m_uiCurrentlyAllocatedMemory >= m_uiMemoryThresholdForGC))
  {
    RunGC();
  }
}

void ezGPUResourcePool::UpdateMemoryStats() const
{
  #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);

  ezStringBuilder sOut;
  sOut.Format("{0} (Mb)", ezArgF(fMegaBytes, 4));
  ezStats::SetStat("GPU Resource Pool/Memory Consumption", sOut.GetData());

  #endif
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_GPUResourcePool_Implementation_GPUResourcePool);

