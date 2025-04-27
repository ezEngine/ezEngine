#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/State/ComputePipeline.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;

class EZ_RENDERERVULKAN_DLL ezGALComputePipelineVulkan : public ezGALComputePipeline
{
public:
  ezGALComputePipelineVulkan(const ezGALComputePipelineCreationDescription& description);
  ~ezGALComputePipelineVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  const vk::Pipeline& GetPipeline() const { return m_pipeline; }

  virtual void SetDebugName(const char* szName) override;

private:
  vk::Pipeline m_pipeline;
};