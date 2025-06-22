
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

#include <RendererCore/Shader/ShaderStageBinary.h>
#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALShaderVulkan : public ezGALShader
{
public:
  virtual void SetDebugName(ezStringView sName) const override;

  EZ_ALWAYS_INLINE vk::ShaderModule GetShader(ezGALShaderStage::Enum stage) const;
  vk::PipelineLayout GetVkPipelineLayout() const;
  vk::DescriptorSetLayout GetDescriptorSetLayout(ezUInt32 uiSet = 0) const;
  vk::PushConstantRange GetPushConstantRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALShaderVulkan(const ezGALShaderCreationDescription& description);
  virtual ~ezGALShaderVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

private:
  vk::ShaderModule m_Shaders[ezGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
