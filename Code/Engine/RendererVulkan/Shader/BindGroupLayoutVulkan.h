
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALBindGroupLayoutVulkan : public ezGALBindGroupLayout
{
public:
  inline vk::DescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALBindGroupLayoutVulkan(const ezGALBindGroupLayoutCreationDescription& Description);

  virtual ~ezGALBindGroupLayoutVulkan();

private:
  vk::DescriptorSetLayout m_DescriptorSetLayout;
};
