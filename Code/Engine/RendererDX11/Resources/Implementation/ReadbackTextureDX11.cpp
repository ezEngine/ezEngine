#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/ReadbackTextureDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

ezGALReadbackTextureDX11::ezGALReadbackTextureDX11(const ezGALTextureCreationDescription& Description)
  : ezGALReadbackTexture(Description)
{
}

ezGALReadbackTextureDX11::~ezGALReadbackTextureDX11() = default;

ezResult ezGALReadbackTextureDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DArray:
    case ezGALTextureType::TextureCube:
    case ezGALTextureType::TextureCubeArray:
    {
      D3D11_TEXTURE2D_DESC Tex2DDesc = {};
      EZ_SUCCEED_OR_RETURN(ezGALTextureDX11::Create2DDesc(m_Description, pDXDevice, Tex2DDesc));
      Tex2DDesc.BindFlags = 0;
      Tex2DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Tex2DDesc.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
      Tex2DDesc.Usage = D3D11_USAGE_STAGING;

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
      {
        return EZ_FAILURE;
      }
    }
    break;

    case ezGALTextureType::Texture3D:
    {
      D3D11_TEXTURE3D_DESC Tex3DDesc = {};
      EZ_SUCCEED_OR_RETURN(ezGALTextureDX11::Create3DDesc(m_Description, pDXDevice, Tex3DDesc));
      Tex3DDesc.BindFlags = 0;
      Tex3DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Tex3DDesc.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
      Tex3DDesc.Usage = D3D11_USAGE_STAGING;

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture3D(&Tex3DDesc, nullptr, reinterpret_cast<ID3D11Texture3D**>(&m_pDXTexture))))
      {
        return EZ_FAILURE;
      }
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezGALReadbackTextureDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_GAL_DX11_RELEASE(m_pDXTexture);
  return EZ_SUCCESS;
}

void ezGALReadbackTextureDX11::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_pDXTexture != nullptr)
  {
    m_pDXTexture->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}
