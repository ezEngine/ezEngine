#include <RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>

#include <d3d11.h>

ezGALQueryVulkan::ezGALQueryVulkan(const ezGALQueryCreationDescription& Description)
    : ezGALQuery(Description)
    , m_pDXQuery(nullptr)
{
}

ezGALQueryVulkan::~ezGALQueryVulkan() {}

ezResult ezGALQueryVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDXDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  D3D11_QUERY_DESC desc;
  if (m_Description.m_type == ezGALQueryType::AnySamplesPassed)
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

ezResult ezGALQueryVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_Vulkan_RELEASE(m_pDXQuery);
  return EZ_SUCCESS;
}

void ezGALQueryVulkan::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_pDXQuery != nullptr)
  {
    m_pDXQuery->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_QueryVulkan);
