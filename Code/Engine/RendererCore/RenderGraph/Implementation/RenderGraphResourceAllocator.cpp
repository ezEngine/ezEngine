#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <RendererCore/RenderGraph/RenderGraphResourceAllocator.h>

ezRenderGraphResourceAllocator::ezRenderGraphResourceAllocator(ezRenderGraphResourcePool* pPool)
  : m_pPool(pPool)
{
  EZ_ASSERT_DEBUG(pPool != nullptr, "Pool must not be null");
}

ezRenderGraphResourceAllocator::~ezRenderGraphResourceAllocator()
{
  FreeResources();
}

ezRenderGraphResourceAllocator::ezRenderGraphResourceAllocator(ezRenderGraphResourceAllocator&& rhs) noexcept
  : m_pPool(rhs.m_pPool)
  , m_TextureGroups(std::move(rhs.m_TextureGroups))
  , m_BufferGroups(std::move(rhs.m_BufferGroups))
{
  rhs.m_pPool = nullptr;
}

ezRenderGraphResourceAllocator& ezRenderGraphResourceAllocator::operator=(ezRenderGraphResourceAllocator&& rhs) noexcept
{
  if (this != &rhs)
  {
    FreeResources();
    m_pPool = rhs.m_pPool;
    m_TextureGroups = std::move(rhs.m_TextureGroups);
    m_BufferGroups = std::move(rhs.m_BufferGroups);
    rhs.m_pPool = nullptr;
  }
  return *this;
}

// --- Textures ---

ezGALTextureHandle ezRenderGraphResourceAllocator::AcquireTexture(const ezGALTextureCreationDescription& desc)
{
  const ezUInt64 uiHash = ComputeDescHash(desc);
  TextureGroup& group = m_TextureGroups[uiHash];

  // Try to recycle a previously released texture.
  if (!group.m_Available.IsEmpty())
  {
    ezSharedPtr<ezPooledRenderTexture> pTex = std::move(group.m_Available.PeekBack());
    group.m_Available.PopBack();
    ezGALTextureHandle hTexture = pTex->GetHandle();
    m_HandleToTexture[hTexture] = std::move(pTex);
    return hTexture;
  }

  // Request from the global pool at the next sequential index.
  ezSharedPtr<ezPooledRenderTexture> pTex = m_pPool->AcquireTexture(desc, group.m_uiNextPoolIndex);
  ++group.m_uiNextPoolIndex;
  group.m_All.PushBack(pTex);
  ezGALTextureHandle hTexture = pTex->GetHandle();
  m_HandleToTexture[hTexture] = std::move(pTex);
  return hTexture;
}

void ezRenderGraphResourceAllocator::ReleaseTexture(ezGALTextureHandle hTexture)
{
  ezSharedPtr<ezPooledRenderTexture> pTex;
  if (!m_HandleToTexture.Remove(hTexture, &pTex))
  {
    EZ_ASSERT_DEBUG(false, "ReleaseTexture: handle not found in allocator");
    return;
  }

  const ezUInt64 uiHash = ComputeDescHash(pTex->GetDescription());
  TextureGroup& group = m_TextureGroups[uiHash];
  group.m_Available.PushBack(std::move(pTex));
}

// --- Buffers ---

ezGALBufferHandle ezRenderGraphResourceAllocator::AcquireBuffer(const ezGALBufferCreationDescription& desc)
{
  const ezUInt64 uiHash = ComputeDescHash(desc);
  BufferGroup& group = m_BufferGroups[uiHash];

  if (!group.m_Available.IsEmpty())
  {
    ezSharedPtr<ezPooledRenderBuffer> pBuf = std::move(group.m_Available.PeekBack());
    group.m_Available.PopBack();
    ezGALBufferHandle hBuffer = pBuf->GetHandle();
    m_HandleToBuffer[hBuffer] = std::move(pBuf);
    return hBuffer;
  }

  ezSharedPtr<ezPooledRenderBuffer> pBuf = m_pPool->AcquireBuffer(desc, group.m_uiNextPoolIndex);
  ++group.m_uiNextPoolIndex;
  group.m_All.PushBack(pBuf);
  ezGALBufferHandle hBuffer = pBuf->GetHandle();
  m_HandleToBuffer[hBuffer] = std::move(pBuf);
  return hBuffer;
}

void ezRenderGraphResourceAllocator::ReleaseBuffer(ezGALBufferHandle hBuffer)
{
  ezSharedPtr<ezPooledRenderBuffer> pBuf;
  if (!m_HandleToBuffer.Remove(hBuffer, &pBuf))
  {
    EZ_ASSERT_DEBUG(false, "ReleaseBuffer: handle not found in allocator");
    return;
  }

  const ezUInt64 uiHash = ComputeDescHash(pBuf->GetDescription());
  BufferGroup& group = m_BufferGroups[uiHash];
  group.m_Available.PushBack(std::move(pBuf));
}

// --- Cleanup ---

void ezRenderGraphResourceAllocator::FreeResources()
{
  m_HandleToTexture.Clear();
  m_HandleToBuffer.Clear();
  m_TextureGroups.Clear();
  m_BufferGroups.Clear();
}

// --- Hashing ---

ezUInt64 ezRenderGraphResourceAllocator::ComputeDescHash(const ezGALTextureCreationDescription& desc)
{
  return ezHashingUtils::xxHash64(&desc, sizeof(desc));
}

ezUInt64 ezRenderGraphResourceAllocator::ComputeDescHash(const ezGALBufferCreationDescription& desc)
{
  return ezHashingUtils::xxHash64(&desc, sizeof(desc));
}
