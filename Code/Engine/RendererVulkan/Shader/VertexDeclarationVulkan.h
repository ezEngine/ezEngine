
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALVertexDeclarationVulkan : public ezGALVertexDeclaration
{
public:

  EZ_ALWAYS_INLINE const vk::PipelineVertexInputStateCreateInfo& GetInputLayout() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALVertexDeclarationVulkan(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclarationVulkan();

  vk::PipelineVertexInputStateCreateInfo m_vulkanInputLayout;
};

#include <RendererVulkan/Shader/Implementation/VertexDeclarationVulkan_inl.h>
