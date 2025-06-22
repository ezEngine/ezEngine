#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/SharedTextureDX11.h>

#include <d3d11.h>

//////////////////////////////////////////////////////////////////////////
// ezGALSharedTextureDX11
//////////////////////////////////////////////////////////////////////////

ezGALSharedTextureDX11::ezGALSharedTextureDX11(const ezGALTextureCreationDescription& Description, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle hSharedHandle)
  : ezGALTextureDX11(Description)
  , m_SharedType(sharedType)
  , m_hSharedHandle(hSharedHandle)
{
}

ezGALSharedTextureDX11::~ezGALSharedTextureDX11() = default;

ezResult ezGALSharedTextureDX11::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  m_pDevice = pDXDevice;

  EZ_ASSERT_DEBUG(m_SharedType != ezGALSharedTextureType::None, "Shared texture must either be exported or imported");
  EZ_ASSERT_DEBUG(m_Description.m_Type == ezGALTextureType::Texture2DShared, "Shared texture must be of type ezGALTextureType::Texture2DShared");

  if (m_SharedType == ezGALSharedTextureType::Imported)
  {
    IDXGIResource* d3d11ResPtr = NULL;
    HRESULT hr = pDXDevice->GetDXDevice()->OpenSharedResource((HANDLE)m_hSharedHandle.m_hSharedTexture, __uuidof(ID3D11Resource), (void**)(&d3d11ResPtr));
    if (FAILED(hr))
    {
      ezLog::Error("Failed to open shared texture: {}", ezArgErrorCode(hr));
      return EZ_FAILURE;
    }
    EZ_SCOPE_EXIT(d3d11ResPtr->Release());

    hr = d3d11ResPtr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pDXTexture));
    if (FAILED(hr))
    {
      ezLog::Error("Failed to query shared texture interface: {}", ezArgErrorCode(hr));
      return EZ_FAILURE;
    }

    hr = d3d11ResPtr->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_pKeyedMutex);
    if (FAILED(hr))
    {
      ezLog::Error("Failed to query keyed mutex interface: {}", ezArgErrorCode(hr));
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  D3D11_TEXTURE2D_DESC Tex2DDesc = {};
  EZ_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pDXDevice, Tex2DDesc));

  if (m_SharedType == ezGALSharedTextureType::Exported)
    Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

  ezHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
  ConvertInitialData(m_Description, pInitialData, InitialData);

  if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
  {
    return EZ_FAILURE;
  }
  else if (m_SharedType == ezGALSharedTextureType::Exported)
  {
    IDXGIResource* pDXGIResource;
    HRESULT hr = m_pDXTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
    if (FAILED(hr))
    {
      ezLog::Error("Failed to get shared texture resource interface: {}", ezArgErrorCode(hr));
      return EZ_FAILURE;
    }
    EZ_SCOPE_EXIT(pDXGIResource->Release());
    HANDLE hTexture = 0;
    hr = pDXGIResource->GetSharedHandle(&hTexture);
    if (FAILED(hr))
    {
      ezLog::Error("Failed to get shared handle: {}", ezArgErrorCode(hr));
      return EZ_FAILURE;
    }
    hr = pDXGIResource->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_pKeyedMutex);
    if (FAILED(hr))
    {
      ezLog::Error("Failed to query keyed mutex interface: {}", ezArgErrorCode(hr));
      return EZ_FAILURE;
    }
    m_hSharedHandle.m_hSharedTexture = (ezUInt64)hTexture;
  }

  return EZ_SUCCESS;
}


ezResult ezGALSharedTextureDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pKeyedMutex);
  return SUPER::DeInitPlatform(pDevice);
}

ezGALPlatformSharedHandle ezGALSharedTextureDX11::GetSharedHandle() const
{
  return m_hSharedHandle;
}

void ezGALSharedTextureDX11::WaitSemaphoreGPU(ezUInt64 uiValue) const
{
  m_pKeyedMutex->AcquireSync(uiValue, INFINITE);
}

void ezGALSharedTextureDX11::SignalSemaphoreGPU(ezUInt64 uiValue) const
{
  m_pKeyedMutex->ReleaseSync(uiValue);
}
