#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <vulkan/vulkan.hpp>

class ezGALBindGroupVulkan : public ezGALBindGroup
{
public:
  inline vk::DescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
  inline ezArrayPtr<const ezUInt32> GetOffsets() const { return m_Offsets.GetArrayPtr(); }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void Invalidate(ezGALDevice* pDevice) override;
  virtual bool IsInvalidated() const override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezGALBindGroupVulkan(const ezGALBindGroupCreationDescription& Description);

  virtual ~ezGALBindGroupVulkan();

private:
  vk::DescriptorSet m_DescriptorSet;
  ezHybridArray<ezUInt32, 1> m_Offsets;
  ezDescriptorSetPoolVulkan::Allocation m_Allocation;
};
