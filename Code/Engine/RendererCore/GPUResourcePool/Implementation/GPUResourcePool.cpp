#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

ezGPUResourcePool* ezGPUResourcePool::s_pDefaultInstance = nullptr;

ezGPUResourcePool::ezGPUResourcePool()
{
  m_pDevice = ezGALDevice::GetDefaultDevice();

  m_GALDeviceEventSubscriptionID = m_pDevice->s_Events.AddEventHandler(ezMakeDelegate(&ezGPUResourcePool::GALDeviceEventHandler, this));
}

ezGPUResourcePool::~ezGPUResourcePool()
{
  m_pDevice->s_Events.RemoveEventHandler(m_GALDeviceEventSubscriptionID);
  if (!m_TexturesInUse.IsEmpty())
  {
    ezLog::SeriousWarning("Destructing a GPU resource pool of which textures are still in use!");
  }

  // Free remaining resources
  RunGC(0);
}

ezGALTextureHandle ezGPUResourcePool::GetRenderTarget(const ezGALTextureCreationDescription& textureDesc)
{
  EZ_LOCK(m_Lock);

  if (!textureDesc.m_bCreateRenderTarget)
  {
    ezLog::Error("Texture description for render target usage has not set bCreateRenderTarget!");
    return ezGALTextureHandle();
  }

  const ezUInt32 uiTextureDescHash = textureDesc.CalculateHash();

  // Check if there is a fitting texture available
  auto it = m_AvailableTextures.Find(uiTextureDescHash);
  if (it.IsValid())
  {
    ezDynamicArray<TextureHandleWithAge>& textures = it.Value();
    if (!textures.IsEmpty())
    {
      ezGALTextureHandle hTexture = textures.PeekBack().m_hTexture;
      textures.PopBack();

      EZ_ASSERT_DEV(m_pDevice->GetTexture(hTexture) != nullptr, "Invalid texture in resource pool");

      m_TexturesInUse.Insert(hTexture);

      return hTexture;
    }
  }

  // Since we found no matching texture we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  ezGALTextureHandle hNewTexture = m_pDevice->CreateTexture(textureDesc);

  if (hNewTexture.IsInvalidated())
  {
    ezLog::Error("GPU resource pool couldn't create new texture for given desc (size: {0} x {1}, format: {2})", textureDesc.m_uiWidth,
      textureDesc.m_uiHeight, textureDesc.m_Format);
    return ezGALTextureHandle();
  }

  // Also track the new created texture
  m_TexturesInUse.Insert(hNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(textureDesc);

  UpdateMemoryStats();

  return hNewTexture;
}

ezGALTextureHandle ezGPUResourcePool::GetRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum format, ezGALMSAASampleCount::Enum sampleCount, ezUInt32 uiSliceCount, ezGALTextureType::Enum textureType)
{
  ezGALTextureCreationDescription TextureDesc;
  TextureDesc.m_bCreateRenderTarget = true;
  TextureDesc.m_bAllowShaderResourceView = true;
  TextureDesc.m_Format = format;
  TextureDesc.m_Type = textureType;
  TextureDesc.m_uiWidth = uiWidth;
  TextureDesc.m_uiHeight = uiHeight;
  TextureDesc.m_SampleCount = sampleCount;
  TextureDesc.m_uiArraySize = uiSliceCount;

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
      it = m_AvailableTextures.Insert(uiTextureDescHash, ezDynamicArray<TextureHandleWithAge>());
    }

    it.Value().PushBack({hRenderTarget, ezRenderWorld::GetFrameCounter()});
  }
}

ezGALBufferHandle ezGPUResourcePool::GetBuffer(const ezGALBufferCreationDescription& bufferDesc)
{
  EZ_LOCK(m_Lock);

  const ezUInt32 uiBufferDescHash = bufferDesc.CalculateHash();

  // Check if there is a fitting buffer available
  auto it = m_AvailableBuffers.Find(uiBufferDescHash);
  if (it.IsValid())
  {
    ezDynamicArray<BufferHandleWithAge>& buffers = it.Value();
    if (!buffers.IsEmpty())
    {
      ezGALBufferHandle hBuffer = buffers.PeekBack().m_hBuffer;
      buffers.PopBack();

      EZ_ASSERT_DEV(m_pDevice->GetBuffer(hBuffer) != nullptr, "Invalid buffer in resource pool");

      m_BuffersInUse.Insert(hBuffer);

      return hBuffer;
    }
  }

  // Since we found no matching buffer we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  ezGALBufferHandle hNewBuffer = m_pDevice->CreateBuffer(bufferDesc);

  if (hNewBuffer.IsInvalidated())
  {
    ezLog::Error("GPU resource pool couldn't create new buffer for given desc (size: {0})", bufferDesc.m_uiTotalSize);
    return ezGALBufferHandle();
  }

  // Also track the new created buffer
  m_BuffersInUse.Insert(hNewBuffer);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(bufferDesc);

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
      it = m_AvailableBuffers.Insert(uiBufferDescHash, ezDynamicArray<BufferHandleWithAge>());
    }

    it.Value().PushBack({hBuffer, ezRenderWorld::GetFrameCounter()});
  }
}

void ezGPUResourcePool::RunGC(ezUInt32 uiMinimumAge)
{
  EZ_LOCK(m_Lock);

  EZ_PROFILE_SCOPE("RunGC");
  ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();
  // Destroy all available textures older than uiMinimumAge frames
  {
    for (auto it = m_AvailableTextures.GetIterator(); it.IsValid();)
    {
      auto& textures = it.Value();
      for (ezInt32 i = (ezInt32)textures.GetCount() - 1; i >= 0; i--)
      {
        TextureHandleWithAge& texture = textures[i];
        if (texture.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const ezGALTexture* pTexture = m_pDevice->GetTexture(texture.m_hTexture))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTexture->GetDescription());
          }

          m_pDevice->DestroyTexture(texture.m_hTexture);
          textures.RemoveAtAndCopy(i);
        }
        else
        {
          // The available textures are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (textures.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableTextures.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  // Destroy all available buffers older than uiMinimumAge frames
  {
    for (auto it = m_AvailableBuffers.GetIterator(); it.IsValid();)
    {
      auto& buffers = it.Value();
      for (ezInt32 i = (ezInt32)buffers.GetCount() - 1; i >= 0; i--)
      {
        BufferHandleWithAge& buffer = buffers[i];
        if (buffer.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(buffer.m_hBuffer))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuffer->GetDescription());
          }

          m_pDevice->DestroyBuffer(buffer.m_hBuffer);
          buffers.RemoveAtAndCopy(i);
        }
        else
        {
          // The available buffers are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (buffers.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableBuffers.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
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
    // Only try to collect resources unused for 3 or more frames. Using a smaller number will result in constant memory thrashing.
    RunGC(3);
  }
}

void ezGPUResourcePool::UpdateMemoryStats() const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);
  ezStats::SetStat("GPU Resource Pool/Memory Consumption (MB)", fMegaBytes);
#endif
}

void ezGPUResourcePool::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  if (e.m_Type == ezGALDeviceEvent::AfterEndFrame)
  {
    ++m_uiFramesSinceLastGC;
    if (m_uiFramesSinceLastGC >= m_uiFramesThresholdSinceLastGC)
    {
      m_uiFramesSinceLastGC = 0;
      RunGC(10);
    }
  }
}
