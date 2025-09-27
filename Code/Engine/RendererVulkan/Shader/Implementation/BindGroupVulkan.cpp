#include <RendererVulkan/Shader/BindGroupVulkan.h>

#include <RendererVulkan/Pools/DescriptorWritePoolVulkan.h>

ezGALBindGroupVulkan::ezGALBindGroupVulkan(const ezGALBindGroupCreationDescription& Description)
  : ezGALBindGroup(Description)
{
}

ezGALBindGroupVulkan::~ezGALBindGroupVulkan() = default;

ezResult ezGALBindGroupVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);
  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(pDeviceVulkan->GetBindGroupLayout(m_Description.m_hBindGroupLayout));
  if (pLayout == nullptr)
  {
    ezLog::Error("Invalid bind group layout handle passed into bind group");
    return EZ_FAILURE;
  }

  m_DescriptorSet = pLayout->GetDescriptorSetPool()->CreateDescriptorSet(m_Description.m_hBindGroupLayout, m_Allocation);
  pDeviceVulkan->GetDescriptorWritePool().WriteDescriptor(m_DescriptorSet, m_Description, m_Offsets);
  return EZ_SUCCESS;
}

ezResult ezGALBindGroupVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  Invalidate(pDevice);
  return EZ_SUCCESS;
}

void ezGALBindGroupVulkan::Invalidate(ezGALDevice* pDevice)
{
  if (m_DescriptorSet != nullptr)
  {
    ezGALDeviceVulkan* pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);
    const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(pDeviceVulkan->GetBindGroupLayout(m_Description.m_hBindGroupLayout));
    pDeviceVulkan->ReclaimLater(m_DescriptorSet, pLayout->GetDescriptorSetPool(), m_Allocation.m_uiPoolIndex);
    m_DescriptorSet = nullptr;
    m_Allocation = {};
  }
}

bool ezGALBindGroupVulkan::IsInvalidated() const
{
  return m_DescriptorSet == nullptr;
}

void ezGALBindGroupVulkan::SetDebugNamePlatform(const char* szName) const
{
}
