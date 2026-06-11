
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>

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

  vk::PipelineVertexInputStateCreateInfo m_CreateInfo;
  ezHybridArray<vk::VertexInputAttributeDescription, EZ_GAL_MAX_VERTEX_ATTRIBUTE_COUNT> m_Attributes;
  ezHybridArray<vk::VertexInputBindingDescription, EZ_GAL_MAX_VERTEX_BUFFER_COUNT> m_Bindings;
};

#include <RendererVulkan/Shader/Implementation/VertexDeclarationVulkan_inl.h>
