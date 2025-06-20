#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/PipelineLayout.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALPipelineLayoutVulkan : public ezGALPipelineLayout
{
public:
  inline vk::PushConstantRange GetPushConstantRange() const { return m_pushConstants; }
  inline vk::PipelineLayout GetVkPipelineLayout() const { return m_PipelineLayout; }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALPipelineLayoutVulkan(const ezGALPipelineLayoutCreationDescription& Description);

  virtual ~ezGALPipelineLayoutVulkan();

private:
  vk::PushConstantRange m_pushConstants;
  vk::PipelineLayout m_PipelineLayout;
};
