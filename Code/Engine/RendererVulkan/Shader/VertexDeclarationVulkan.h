
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALVertexDeclarationVulkan : public ezGALVertexDeclaration
{
public:
  EZ_ALWAYS_INLINE const vk::PipelineVertexInputStateCreateInfo& GetCreateInfo() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALVertexDeclarationVulkan(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclarationVulkan();

  vk::PipelineVertexInputStateCreateInfo m_createInfo;
  ezHybridArray<vk::VertexInputAttributeDescription, EZ_GAL_MAX_VERTEX_ATTRIBUTE_COUNT> m_attributes;
  ezHybridArray<vk::VertexInputBindingDescription, EZ_GAL_MAX_VERTEX_BUFFER_COUNT> m_bindings;
};

#include <RendererVulkan/Shader/Implementation/VertexDeclarationVulkan_inl.h>
