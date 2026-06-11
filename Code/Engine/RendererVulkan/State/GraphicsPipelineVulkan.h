#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/State/GraphicsPipeline.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class ezGALDeviceVulkan;

class EZ_RENDERERVULKAN_DLL ezGALGraphicsPipelineVulkan : public ezGALGraphicsPipeline
{
public:
  ezGALGraphicsPipelineVulkan(const ezGALGraphicsPipelineCreationDescription& description);
  ~ezGALGraphicsPipelineVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  const vk::Pipeline& GetPipeline() const { return m_Pipeline; }
  bool HasStencilTest() const { return m_bStencilTest; }

  virtual void SetDebugName(const char* szName) override;

private:
  vk::Pipeline m_Pipeline;
  bool m_bStencilTest = false;
};