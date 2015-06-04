
#include <RendererCore/PCH.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Foundation/Threading/Lock.h>

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
  for (ezGALTextureHandle hCurrentTexture : m_AvailableTextures)
  {
    const ezGALTexture* pTexture = m_pDevice->GetTexture(hCurrentTexture);

    if (pTexture)
    {
      // If the texture description matches, return the texture handle
      if (uiTextureDescHash == pTexture->GetDescription().CalculateHash())
      {
        m_TexturesInUse.PushBack(hCurrentTexture);
        m_AvailableTextures.Remove(hCurrentTexture);

        return hCurrentTexture;
      }
    }
    else
    {
      ezLog::SeriousWarning("Texture in available texture list of GPU resource pool was destroyed!");
    }

  }

  // Since we found no matching texture we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  ezGALTextureHandle hNewTexture = m_pDevice->CreateTexture(TextureDesc);
  
  if (hNewTexture.IsInvalidated())
  {
    ezLog::Error("GPU resource pool couldn't create new texture for given desc (size: %d x %d, format: %d)", TextureDesc.m_uiWidth, TextureDesc.m_uiHeight, TextureDesc.m_Format);
    return ezGALTextureHandle();
  }

  // Also track the new created texture
  m_TexturesInUse.PushBack(hNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(TextureDesc);

  UpdateMemoryStats();

  return hNewTexture;
}

ezGALTextureHandle ezGPUResourcePool::GetRenderTarget(ezUInt32 uiWidth, ezUInt32 uiHeight, ezGALResourceFormat::Enum eFormat)
{
  ezGALTextureCreationDescription TextureDesc;
  TextureDesc.m_bAllowDynamicMipGeneration = true;
  TextureDesc.m_bCreateRenderTarget = true;
  TextureDesc.m_bAllowShaderResourceView = true;
  TextureDesc.m_Format = eFormat;
  TextureDesc.m_Type = ezGALTextureType::Texture2D;
  TextureDesc.m_uiWidth = uiWidth;
  TextureDesc.m_uiHeight = uiHeight;

  return GetRenderTarget(TextureDesc);
}

void ezGPUResourcePool::ReturnRenderTarget(ezGALTextureHandle hRenderTarget)
{
  EZ_LOCK(m_Lock);

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_TexturesInUse.Contains(hRenderTarget))
  {
    ezLog::SeriousWarning("Returning a texture to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

  #endif

  m_TexturesInUse.Remove(hRenderTarget);
  m_AvailableTextures.PushBack(hRenderTarget);
}

void ezGPUResourcePool::RunGC()
{
  EZ_LOCK(m_Lock);

  // Destroy all available textures
  for(ezGALTextureHandle hCurrentTexture : m_AvailableTextures)
  {
    const ezGALTexture* pTexture = m_pDevice->GetTexture(hCurrentTexture);
    
    if (pTexture)
    {
      m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTexture->GetDescription());
    }

    m_pDevice->DestroyTexture(hCurrentTexture);
  }

  m_AvailableTextures.Clear();

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
  sOut.Format("%.4f (Mb)", fMegaBytes);
  ezStats::SetStat("GPU Resource Pool/Memory Consumption", sOut.GetData());

  #endif
}
