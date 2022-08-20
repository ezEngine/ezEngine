#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALTextureVulkan;
class ezGALRenderTargetViewVulkan;
class ezGALResourceViewVulkan;
class ezGALUnorderedAccessViewVulkan;

/// \brief
class EZ_RENDERERVULKAN_DLL ezImageCopyVulkan
{
public:
  ezImageCopyVulkan(ezGALDeviceVulkan& GALDeviceVulkan);
  ~ezImageCopyVulkan();
  void Init(const ezGALTextureVulkan* pSource, const ezGALTextureVulkan* pTarget, bool useReadbackTarget, ezShaderUtils::ezBuiltinShaderType type);

  void Copy(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends);

private:
  void RenderInternal(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends);

private:
  ezGALDeviceVulkan& m_GALDeviceVulkan;

  // Init input
  const ezGALTextureVulkan* m_pSource = nullptr;
  const ezGALTextureVulkan* m_pTarget = nullptr;
  bool m_bUseReadbackTarget = false;
  ezShaderUtils::ezBuiltinShaderType m_type = ezShaderUtils::ezBuiltinShaderType::CopyImage;

  // Render target settings
  bool m_bRequiresProxy = false;
  vk::Image m_targetImage;
  vk::Format m_targetFormat;

  // Init derived Vulkan objects
  vk::RenderPass m_renderPass;
  ezShaderUtils::ezBuiltinShader m_shader;
  ezGALVertexDeclarationHandle m_hVertexDecl;
  ezResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  ezResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  vk::Pipeline m_pipeline;

  // Proxy render target in case we can't render to a linear RT
  vk::Image m_proxyImage;
  ezVulkanAllocation m_proxyAlloc = nullptr;
  ezVulkanAllocationInfo m_proxyAllocInfo;
};
