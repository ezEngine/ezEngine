
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>

class ezDescriptorSetPoolVulkan;

class ezGALBindGroupLayoutVulkan : public ezGALBindGroupLayout
{
public:
  inline const vk::DescriptorSetLayout& GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
  ezDescriptorSetPoolVulkan* GetDescriptorSetPool() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALBindGroupLayoutVulkan(const ezGALBindGroupLayoutCreationDescription& Description);

  virtual ~ezGALBindGroupLayoutVulkan();

private:
  vk::DescriptorSetLayout m_DescriptorSetLayout;
  ezBindGroupLayoutResourceUsageVulkan m_ResourceUsage; // How many resources of each type each descriptor set uses.
  ezSharedPtr<ezDescriptorSetPoolVulkan> m_DescriptorSetPool;
};
