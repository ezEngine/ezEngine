
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

#include <RendererCore/Shader/ShaderStageBinary.h>
#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALShaderVulkan : public ezGALShader
{
public:
  /// \brief Used as input to ezResourceCacheVulkan::RequestDescriptorSetLayout to create a vk::DescriptorSetLayout.
  struct DescriptorSetLayoutDesc
  {
    mutable ezUInt32 m_uiHash = 0;
    ezHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
    void ComputeHash();
  };

  virtual void SetDebugName(ezStringView sName) const override;

  EZ_ALWAYS_INLINE vk::ShaderModule GetShader(ezGALShaderStage::Enum stage) const;
  EZ_ALWAYS_INLINE vk::PipelineLayout GetPipelineLayout() const;
  EZ_ALWAYS_INLINE ezUInt32 GetSetCount() const;
  EZ_ALWAYS_INLINE vk::DescriptorSetLayout GetDescriptorSetLayout(ezUInt32 uiSet = 0) const;
  EZ_ALWAYS_INLINE ezArrayPtr<const ezShaderResourceBinding> GetBindings(ezUInt32 uiSet = 0) const;
  EZ_ALWAYS_INLINE vk::PushConstantRange GetPushConstantRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALShaderVulkan(const ezGALShaderCreationDescription& description);
  virtual ~ezGALShaderVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

private:
  vk::PushConstantRange m_pushConstants;
  ezHybridArray<vk::DescriptorSetLayout, 4> m_descriptorSetLayout;
  ezHybridArray<ezHybridArray<ezShaderResourceBinding, 16>, 4> m_SetBindings;
  vk::ShaderModule m_Shaders[ezGALShaderStage::ENUM_COUNT];
  vk::PipelineLayout m_PipelineLayout;
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
