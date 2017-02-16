
#include <PCH.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>

#include <d3d11.h>

ezGALQueryDX11::ezGALQueryDX11(const ezGALQueryCreationDescription& Description)
  : ezGALQuery(Description), m_pDXQuery(nullptr)
{
}

ezGALQueryDX11::~ezGALQueryDX11()
{
}

void ezGALQueryDX11::SetDebugName(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_pDXQuery != nullptr)
  {
    m_pDXQuery->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

ezResult ezGALQueryDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  D3D11_QUERY_DESC desc;
  if(m_Description.m_type == ezGALQueryType::AnySamplesPassed)
    desc.MiscFlags = m_Description.m_bDrawIfUnknown ? D3D11_QUERY_MISC_PREDICATEHINT : 0;
  else
    desc.MiscFlags = 0;

  switch (m_Description.m_type)
  {
  case ezGALQueryType::NumSamplesPassed:
    desc.Query = D3D11_QUERY_OCCLUSION;
    break;
  case ezGALQueryType::AnySamplesPassed:
    desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
    break;
  case ezGALQueryType::Timestamp:
    desc.Query = D3D11_QUERY_TIMESTAMP;
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateQuery(&desc, &m_pDXQuery)))
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Creation of native DirectX query failed!");
    return EZ_FAILURE;
  }
}

ezResult ezGALQueryDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXQuery);
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_QueryDX11);
