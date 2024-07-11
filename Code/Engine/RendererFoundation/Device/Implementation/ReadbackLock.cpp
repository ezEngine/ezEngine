#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ReadbackLock.h>


ezReadbackBufferLock::ezReadbackBufferLock(ezGALDevice* pDevice, const ezGALReadbackBuffer* pBuffer, ezArrayPtr<const ezUInt8>& out_Memory)
  : m_pDevice(pDevice)
  , m_pBuffer(pBuffer)
{
  if (m_pDevice->LockBufferPlatform(m_pBuffer, out_Memory).Failed())
  {
    m_pDevice = nullptr;
    m_pBuffer = nullptr;
  }
}


ezReadbackBufferLock::~ezReadbackBufferLock()
{
  if (m_pDevice)
  {
    m_pDevice->UnlockBufferPlatform(m_pBuffer);
  }
}

void ezReadbackBufferLock::operator=(ezReadbackBufferLock&& rhs)
{
  if (m_pDevice)
  {
    m_pDevice->UnlockBufferPlatform(m_pBuffer);
  }

  m_pDevice = rhs.m_pDevice;
  rhs.m_pDevice = nullptr;
  m_pBuffer = rhs.m_pBuffer;
  rhs.m_pBuffer = nullptr;
}

//////////////////////////////////////////////////////////////////////////

ezReadbackTextureLock::ezReadbackTextureLock(ezGALDevice* pDevice, const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory)
  : m_pDevice(pDevice)
  , m_pTexture(pTexture)
  , m_subResources(subResources)
{
  if (m_pDevice->LockTexturePlatform(m_pTexture, subResources, out_Memory).Failed())
  {
    m_pDevice = nullptr;
    m_pTexture = nullptr;
    m_subResources = {};
  }
}

ezReadbackTextureLock::~ezReadbackTextureLock()
{
  if (m_pDevice)
  {
    m_pDevice->UnlockTexturePlatform(m_pTexture, m_subResources);
  }
}

void ezReadbackTextureLock::operator=(ezReadbackTextureLock&& rhs)
{
  if (m_pDevice)
  {
    m_pDevice->UnlockTexturePlatform(m_pTexture, m_subResources);
  }

  m_pDevice = rhs.m_pDevice;
  rhs.m_pDevice = nullptr;
  m_pTexture = rhs.m_pTexture;
  rhs.m_pTexture = nullptr;
  m_subResources = rhs.m_subResources;
  rhs.m_subResources = {};
}
