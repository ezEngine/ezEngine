
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

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

  /// \brief Remaps high level resource binding to the descriptor layout used by this shader.
  struct BindingMapping
  {
    enum Type : ezUInt8
    {
      ConstantBuffer,
      ResourceView,
      UAV,
      Sampler,
    };
    vk::DescriptorType m_descriptorType = vk::DescriptorType::eSampler;
    Type m_type = Type::ConstantBuffer;                            ///< Source resource type in the high level binding model.
    ezGALShaderStage::Enum m_stage = ezGALShaderStage::ENUM_COUNT; ///< Source stage in the high level resource binding model.
    ezUInt8 m_uiSource = 0;                                        ///< Source binding index in the high level resource binding model.
    ezUInt8 m_uiTarget = 0;                                        ///< Target binding index in the descriptor set layout.
  };

  void SetDebugName(const char* szName) const override;

  EZ_ALWAYS_INLINE vk::ShaderModule GetShader(ezGALShaderStage::Enum stage) const;
  EZ_ALWAYS_INLINE const DescriptorSetLayoutDesc& GetDescriptorSetLayout() const;
  EZ_ALWAYS_INLINE const ezArrayPtr<const BindingMapping> GetBindingMapping() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALShaderVulkan(const ezGALShaderCreationDescription& description);
  virtual ~ezGALShaderVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

private:
  DescriptorSetLayoutDesc m_descriptorSetLayoutDesc;
  ezHybridArray<BindingMapping, 16> m_BindingMapping;
  vk::ShaderModule m_Shaders[ezGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
