#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

namespace
{
  ezUInt64 ComputeDescHash(const ezGALTextureCreationDescription& desc)
  {
    return ezHashingUtils::xxHash64(&desc, sizeof(desc));
  }

  ezUInt64 ComputeDescHash(const ezGALBufferCreationDescription& desc)
  {
    return ezHashingUtils::xxHash64(&desc, sizeof(desc));
  }
} // namespace

ezRenderGraphResourcePool::ezRenderGraphResourcePool(ezGALDevice* pDevice)
  : m_pDevice(pDevice)
{
}

ezRenderGraphResourcePool::~ezRenderGraphResourcePool()
{
  Clear();
}

// --- Private: called by ezRenderGraphResourceAllocator ---

ezSharedPtr<ezPooledRenderTexture> ezRenderGraphResourcePool::AcquireTexture(const ezGALTextureCreationDescription& desc, ezUInt32 uiIndex)
{
  EZ_PROFILE_SCOPE("AcquireTexture");
  EZ_LOCK(m_Mutex);

  const ezUInt64 uiHash = ComputeDescHash(desc);
  auto& slots = m_Textures[uiHash]; // inserts empty array if not present

  // Grow the slot array if needed.
  while (slots.GetCount() <= uiIndex)
  {
    slots.PushBack(nullptr);
  }

  ezSharedPtr<ezPooledRenderTexture>& pSlot = slots[uiIndex];
  if (pSlot == nullptr)
  {
    // Create the GPU resource.
    ezGALTextureHandle hTexture = m_pDevice->CreateTexture(desc);
    EZ_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create transient texture ({}x{}, format {})",
      desc.m_uiWidth, desc.m_uiHeight, (int)desc.m_Format.GetValue());

    pSlot = EZ_DEFAULT_NEW(ezPooledRenderTexture);
    pSlot->m_hTexture = hTexture;
    pSlot->m_Desc = desc;

    m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(desc);
    UpdateMemoryStats();
  }

  pSlot->m_uiUsedCounter = 0;
  return pSlot;
}

ezSharedPtr<ezPooledRenderBuffer> ezRenderGraphResourcePool::AcquireBuffer(const ezGALBufferCreationDescription& desc, ezUInt32 uiIndex)
{
  EZ_PROFILE_SCOPE("AcquireBuffer");
  EZ_LOCK(m_Mutex);

  const ezUInt64 uiHash = ComputeDescHash(desc);
  auto& slots = m_Buffers[uiHash];

  while (slots.GetCount() <= uiIndex)
  {
    slots.PushBack(nullptr);
  }

  ezSharedPtr<ezPooledRenderBuffer>& pSlot = slots[uiIndex];
  if (pSlot == nullptr)
  {
    ezGALBufferHandle hBuffer = m_pDevice->CreateBuffer(desc);
    EZ_ASSERT_DEV(!hBuffer.IsInvalidated(), "Failed to create transient buffer (size {})", desc.m_uiTotalSize);

    pSlot = EZ_DEFAULT_NEW(ezPooledRenderBuffer);
    pSlot->m_hBuffer = hBuffer;
    pSlot->m_Desc = desc;

    m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(desc);
    UpdateMemoryStats();
  }

  pSlot->m_uiUsedCounter = 0;
  return pSlot;
}

// --- GC & Cleanup ---

void ezRenderGraphResourcePool::RunGC(ezUInt32 uiMinimumAge)
{
  EZ_PROFILE_SCOPE("RunGC");
  EZ_LOCK(m_Mutex);

  for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
  {
    auto& slots = it.Value();
    for (ezInt32 i = (ezInt32)slots.GetCount() - 1; i >= 0; --i)
    {
      ezSharedPtr<ezPooledRenderTexture>& pTex = slots[i];
      if (pTex == nullptr)
        continue;

      // Only destroy if the pool is the sole owner and it's old enough.
      if (pTex->GetRefCount() > 1)
        continue;
      if (pTex->m_uiUsedCounter < uiMinimumAge)
      {
        pTex->m_uiUsedCounter++;
        continue;
      }
      m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTex->m_Desc);
      m_pDevice->DestroyTexture(pTex->m_hTexture);
      pTex = nullptr;
    }

    // Trim trailing nullptrs from the slot array.
    while (!slots.IsEmpty() && slots.PeekBack() == nullptr)
    {
      slots.PopBack();
    }
  }

  for (auto it = m_Buffers.GetIterator(); it.IsValid(); ++it)
  {
    auto& slots = it.Value();
    for (ezInt32 i = (ezInt32)slots.GetCount() - 1; i >= 0; --i)
    {
      ezSharedPtr<ezPooledRenderBuffer>& pBuf = slots[i];
      if (pBuf == nullptr)
        continue;

      if (pBuf->GetRefCount() > 1)
        continue;
      if (pBuf->m_uiUsedCounter < uiMinimumAge)
      {
        pBuf->m_uiUsedCounter++;
        continue;
      }

      m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuf->m_Desc);
      m_pDevice->DestroyBuffer(pBuf->m_hBuffer);
      pBuf = nullptr;
    }

    while (!slots.IsEmpty() && slots.PeekBack() == nullptr)
    {
      slots.PopBack();
    }
  }

  UpdateMemoryStats();
}

void ezRenderGraphResourcePool::EndFrame()
{
  ++m_uiFrameCounter;
  ++m_uiFramesSinceLastGC;
  if (m_uiFramesSinceLastGC >= s_uiFramesThresholdForGC)
  {
    m_uiFramesSinceLastGC = 0;
    RunGC(s_uiMinimumAgeForGC);
  }
}

void ezRenderGraphResourcePool::Clear()
{
  EZ_LOCK(m_Mutex);

  for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pTex : it.Value())
    {
      if (pTex != nullptr)
      {
        m_pDevice->DestroyTexture(pTex->m_hTexture);
        pTex = nullptr;
      }
    }
  }

  for (auto it = m_Buffers.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pBuf : it.Value())
    {
      if (pBuf != nullptr)
      {
        m_pDevice->DestroyBuffer(pBuf->m_hBuffer);
        pBuf = nullptr;
      }
    }
  }

  m_Textures.Clear();
  m_Buffers.Clear();
  m_uiCurrentlyAllocatedMemory = 0;
  UpdateMemoryStats();
}

void ezRenderGraphResourcePool::UpdateMemoryStats() const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);
  ezStats::SetStat("RenderGraph Resource Pool/Memory (MB)", fMegaBytes);

  ezUInt32 uiTextures = 0;
  for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pTex : it.Value())
    {
      if (pTex != nullptr)
        ++uiTextures;
    }
  }

  ezUInt32 uiBuffers = 0;
  for (auto it = m_Buffers.GetIterator(); it.IsValid(); ++it)
  {
    for (auto& pBuf : it.Value())
    {
      if (pBuf != nullptr)
        ++uiBuffers;
    }
  }

  ezStats::SetStat("RenderGraph Resource Pool/Textures", (double)uiTextures);
  ezStats::SetStat("RenderGraph Resource Pool/Buffers", (double)uiBuffers);
#endif
}
